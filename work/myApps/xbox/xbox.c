#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <string.h>



int main()
{
    libusb_init(NULL);
    libusb_device_handle *handle = NULL;
    handle = libusb_open_device_with_vid_pid(NULL, 0x045e, 0x028e);
    if (handle == NULL) {
        printf("Xbox controller not found\n");
        return 1;
    }
    libusb_claim_interface(handle,0);
    
    unsigned char buffer[8];
    int length = 0;
    int result = libusb_interrupt_transfer(handle, 0x81, buffer, sizeof(buffer), &length, 0);
    if (result ==0 && length == sizeof(buffer)){
    	for (int i = 0; i < 8; i++){
    	    printf("%c",buffer[i]);
    	}
    }
    libusb_release_interface(handle, 0);
    libusb_close(handle);
    libusb_exit(NULL);
    return 0;
}
