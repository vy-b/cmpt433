/*
// driver for flashing led in morse code
// adapted from sample code by Dr. Brian Fraser
// used code from echo.c, demo_miscdriver
*/

#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/kfifo.h>
#include <linux/leds.h>
#define MY_DEVICE_FILE "morse-code"
#define FIFO_SIZE 256
#define LED_DOT_TIME 200
DEFINE_LED_TRIGGER(ledtrig);
static DECLARE_KFIFO(echo_fifo, char, FIFO_SIZE);
static unsigned short morsecode_codes[] = {
	0xB800, // A 1011 1
	0xEA80, // B 1110 1010 1
	0xEBA0, // C 1110 1011 101
	0xEA00, // D 1110 101
	0x8000, // E 1
	0xAE80, // F 1010 1110 1
	0xEE80, // G 1110 1110 1
	0xAA00, // H 1010 101
	0xA000, // I 101
	0xBBB8, // J 1011 1011 1011 1
	0xEB80, // K 1110 1011 1
	0xBA80, // L 1011 1010 1
	0xEE00, // M 1110 111
	0xE800, // N 1110 1
	0xEEE0, // O 1110 1110 111
	0xBBA0, // P 1011 1011 101
	0xEEB8, // Q 1110 1110 1011 1
	0xBA00, // R 1011 101
	0xA800, // S 1010 1
	0xE000, // T 111
	0xAE00, // U 1010 111
	0xAB80, // V 1010 1011 1
	0xBB80, // W 1011 1011 1
	0xEAE0, // X 1110 1010 111
	0xEBB8, // Y 1110 1011 1011 1
	0xEEA0	// Z 1110 1110 101
};


// led_register from Dr. Brian's code demo_ledtrig.c
static void led_register(void)
{
	// Setup the trigger's name:
	led_trigger_register_simple("morse-code", &ledtrig);
}

static void led_unregister(void)
{
	// Cleanup
	led_trigger_unregister_simple(ledtrig);
}

static unsigned short charToMorse(char c)
{
	if (c >= 'A' && c <= 'Z'){
		printk(KERN_INFO "Morse code for %c (#%d) is %x.\n", c, c, morsecode_codes[c - 'A']);
		return morsecode_codes[c - 'A'];
	}
	if (c >= 'a' && c <= 'z'){
		printk(KERN_INFO "Morse code for %c (#%d) is %x.\n", c, c, morsecode_codes[c - 'a']);
		return morsecode_codes[c - 'a'];
	}

	if (c == ' '){
		printk(KERN_INFO "space");
		return 1;
	}
	
	return 0;
}

static ssize_t flash_leds(unsigned short morse)
{
	// use bit masking to get dots and dashes
	// shift after each time a dot or dash is recorded
	while (morse)
	{
		// identify dash
		if ((morse & 0xE000) == 0xE000)
		{
			morse = morse << 3; // shift the dash away
			led_trigger_event(ledtrig, LED_FULL);
			msleep(LED_DOT_TIME*3);

			if (!kfifo_put(&echo_fifo, '-'))
				return -EFAULT;
		}
		// identify dot
		else if ((morse & 0x8000) == 0x8000)
		{
			morse = morse << 1; // shift the dot away
			led_trigger_event(ledtrig, LED_FULL);
			msleep(LED_DOT_TIME);

			if (!kfifo_put(&echo_fifo, '.'))
				return -EFAULT;
		}
		
		else // if there is still more, wait 1 dot time before next dot/dash
		{
			morse = morse << 1;
			led_trigger_event(ledtrig, LED_OFF);
			msleep(LED_DOT_TIME);
		}
	}
	return 0;
}

static ssize_t my_read(struct file *file,
					   char *buff, size_t count, loff_t *ppos)
{
	int bytes_read = 0;

	if (kfifo_to_user(&echo_fifo, buff, count, &bytes_read))
	{
		printk(KERN_ERR "Unable to write to buffer.");
		return -EFAULT;
	}
	return bytes_read; // # bytes actually read.
}
static int isSpace(char c)
{
	return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}
static int isAlpha(char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}
static ssize_t trim_spaces(char c, const char *buff, int *curr, int *end)
{
	int curr_idx = *curr;
	int end_idx = *end;
	while (isSpace(c) || !isAlpha(c))
	{
		curr_idx++;
		if (copy_from_user(&c, &buff[curr_idx], sizeof(c)))
			return -EFAULT;
	}
	if (copy_from_user(&c, &buff[end_idx - 1], sizeof(c)))
		return -EFAULT;
	while (isSpace(c) || !isAlpha(c))
	{
		end_idx--;
		if (copy_from_user(&c, &buff[end_idx - 1], sizeof(c)))
			return -EFAULT;
	}
	*curr = curr_idx;
	*end = end_idx;
	return 0;
}
static ssize_t my_write(struct file *file,
						const char *buff, size_t count, loff_t *ppos)
{
	unsigned short morseValue = 0;
	int curr = 0;
	int start = 0;
	int end = count;
	char c;
	int spaceFlag = 0;
	if (copy_from_user(&c, &buff[curr], sizeof(c)))
		return -EFAULT;
	trim_spaces(c, buff, &curr, &end);
	start = curr;
	// iterate through characters in buffer
	for (curr = start; curr < end; curr++)
	{
		if (copy_from_user(&c, &buff[curr], sizeof(c)))
			return -EFAULT;
		morseValue = charToMorse(c);
		// all non-alphas are ignored, if a space is detected then this is flagged to wait 7 dot times later
		while (morseValue == 1 || morseValue == 0){
			if (morseValue == 1){
				spaceFlag = 1;
			}
			curr++;
			if (copy_from_user(&c, &buff[curr], sizeof(c)))
				return -EFAULT;
			morseValue = charToMorse(c);
		}
		// wait 3 dot times before flashing the current character, unless this is the first char
		// but if the current is a space or nonAlpha, then skip to next if
		if (curr != start && morseValue!=1 && morseValue!=0)
		{
			// if there was a space beforehand, that means it is flagged to wait 7 dot times in the next if, don't wait more.
			if (!spaceFlag)
			{
				// wait 3 dot times between each character
				led_trigger_event(ledtrig, LED_OFF);
				msleep(LED_DOT_TIME*3);
				if (!kfifo_put(&echo_fifo, ' '))
					return -EFAULT;
			}
		}
		if (spaceFlag) // is a space char
		{
			spaceFlag = 0;
			// wait 7 dot times for a space
			led_trigger_event(ledtrig, LED_OFF);
			msleep(LED_DOT_TIME*7);

			if (!kfifo_put(&echo_fifo, ' '))
				return -EFAULT;
			if (!kfifo_put(&echo_fifo, ' '))
				return -EFAULT;
			if (!kfifo_put(&echo_fifo, ' '))
				return -EFAULT;
		}
		flash_leds(morseValue);
	}
	led_trigger_event(ledtrig, LED_OFF);
	if (!kfifo_put(&echo_fifo, '\n'))
		return -EFAULT;

	// Return # bytes actually written.
	*ppos += count;
	return count;
}

/******************************************************
 * Misc support
 ******************************************************/
// Callbacks:  (structure defined in /linux/fs.h)
struct file_operations my_fops = {
	.owner = THIS_MODULE,
	.read = my_read,
	.write = my_write,
};

// Character Device info for the Kernel:
static struct miscdevice my_miscdevice = {
	.minor = MISC_DYNAMIC_MINOR, // Let the system assign one.
	.name = MY_DEVICE_FILE,		 // /dev/.... file.
	.fops = &my_fops			 // Callback functions.
};

static int __init morsecode_init(void)
{
	int ret;
	printk(KERN_INFO "----> My morse driver init()\n");
	INIT_KFIFO(echo_fifo);
	ret = misc_register(&my_miscdevice);
	led_register();

	return ret;
}

static void __exit morsecode_exit(void)
{
	printk(KERN_INFO "<---- My morse driver exit().\n");

	// unregister
	misc_deregister(&my_miscdevice);
	led_unregister();
}

// Link our init/exit functions into the kernel's code.
module_init(morsecode_init);
module_exit(morsecode_exit);

// Information about this module:
MODULE_AUTHOR("Vy!");
MODULE_DESCRIPTION("Morsecode driver test run space");
MODULE_LICENSE("GPL"); // Important to leave as GPL.