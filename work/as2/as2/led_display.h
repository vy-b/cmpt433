#ifndef _LED_DISPLAY_H_
#define _LED_DISPLAY_H_

#define I2C_DEVICE_ADDRESS 0x20
#define REG_DIRA 0x00 // Zen Red uses: 0x02
#define REG_DIRB 0x01 // Zen Red uses: 0x03
#define REG_OUTA 0x14 // Zen Red uses: 0x00
#define REG_OUTB 0x15 // Zen Red uses: 0x01
#define LEFT_DIRECTION "/sys/class/gpio/gpio61/direction"
#define RIGHT_DIRECTION "/sys/class/gpio/gpio44/direction"
#define RIGHT_VALUE "/sys/class/gpio/gpio44/value"
#define LEFT_VALUE "/sys/class/gpio/gpio61/value"
#define I2CDRV_LINUX_BUS0 "/dev/i2c-0"
#define I2CDRV_LINUX_BUS1 "/dev/i2c-1"
#define I2CDRV_LINUX_BUS2 "/dev/i2c-2"
void start_ledThread(void);
void stop_ledThread(void);
void led_shutdownRoutine();
#endif