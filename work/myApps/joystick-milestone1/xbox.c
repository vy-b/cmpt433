#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>

#define VENDOR_ID 0x045e
#define PRODUCT_ID 0x028e
#define INTERFACE_NUMBER 0
#define ENDPOINT 0x81

int main(int argc, char **argv)
{
    libusb_context *ctx;
    libusb_device_handle *dev_handle;
    int error;
    ssize_t cnt;

    error = libusb_init(&ctx);
    if (error != LIBUSB_SUCCESS) {
        fprintf(stderr, "libusb_init() failed: %s\n", libusb_strerror(error));
        return 1;
    }

    dev_handle = libusb_open_device_with_vid_pid(ctx, VENDOR_ID, PRODUCT_ID);
    printf("%d, %d",VENDOR_ID,PRODUCT_ID);
    if (dev_handle == NULL) {
        fprintf(stderr, "libusb_open_device_with_vid_pid() failed: device not found\n");
        libusb_exit(ctx);
        return 1;
    }
    if (dev_handle != NULL)
    {
        libusb_detach_kernel_driver(dev_handle, INTERFACE_NUMBER);
        {
            error = libusb_claim_interface(dev_handle, 0);
            if (error != LIBUSB_SUCCESS) {
                fprintf(stderr, "libusb_claim_interface() failed: %s\n", libusb_strerror(error));
                libusb_close(dev_handle);
                libusb_exit(ctx);
                return 1;
            }
        }
    }

    printf("Waiting for controller input...\n");

    unsigned char data[20];

    while (1) {
        error = libusb_interrupt_transfer(dev_handle, ENDPOINT, data, sizeof(data), &cnt, 0);
        if (error == LIBUSB_ERROR_INTERRUPTED) {
            break;
        }
        if (error != LIBUSB_SUCCESS) {
            fprintf(stderr, "libusb_interrupt_transfer() failed: %s\n", libusb_strerror(error));
            break;
        }
        // printf("Received controller input: ");
        // for (int i = 0; i < cnt; i++) {
        //     printf("%x ", data[i]);
        // }
        // printf("\n");
        if (data[8]==0xff && data [9]==0x7f){
            printf("up\n");
        }
        if (data[7] == 0x80){
            printf("left\n");
        }
        if (data[6]==0xff&&data[7]==0x7f){
            printf("right\n");
        }
        if (data[9]==0x80){
            printf("down\n");
        }
    }

    libusb_release_interface(dev_handle, 0);
    libusb_close(dev_handle);
    libusb_exit(ctx);

    return 0;
}
