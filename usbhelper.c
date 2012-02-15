#include <stdio.h>
#include <string.h>
#include <libusb-1.0/libusb.h>

#define CONTROL_TIMEOUT 5000
#define READ_TIMEOUT 5000

int open_and_callback(libusb_device *device, int (action)(libusb_device_handle *));

void error(char *info, int retcode)
{
	fprintf(stderr, "usbhelper: %s %d\n", info, retcode);
	fflush(stderr);
}

int usb_return(int retcode, char *info)
{
	if (retcode < 0) {
		error(info, retcode);
	}
	return retcode;
}

void iterate_usb(int (is_interesting)(libusb_device *),
	int (do_process)(libusb_device_handle *))
{
	// discover devices
	libusb_context *ctx = NULL;
	libusb_device **list;

	int r;

	r = usb_return(libusb_init(&ctx), "libusb_init");
	if (r < 0) {
		return;
	}
	
	#ifdef DEBUG
	usb_return(libusb_set_debug(ctx, 3));
	#endif
	
	ssize_t cnt = usb_return(libusb_get_device_list(ctx, &list), "libusb_get_device_list");
	ssize_t i = 0;

	for (i = 0; i < cnt; i++) {
			libusb_device *device = list[i];
			if (is_interesting(device)) {
				open_and_callback(device, do_process);
			}
	}
	
	libusb_free_device_list(list, 1);
	libusb_exit(ctx);
}

int open_and_callback(libusb_device *device, int(action)(libusb_device_handle *))
{
	libusb_device_handle *handle;
	int err = 0;
	
	err = usb_return(libusb_open(device, &handle), "libusb_open");
	if (!err) {
		if (action)
			action(handle);
		libusb_close(handle);
	}
	return (0);
}
	
int device_vendor_product_is(libusb_device *device, uint16_t vendor, uint16_t product)
{
	int r;
	struct libusb_device_descriptor devdesc;

	r = usb_return(libusb_get_device_descriptor(device, &devdesc), "device_vendor_product_is: libusb_get_device_descriptor");
	if (r == 0) {
		r = (devdesc.idVendor == vendor && devdesc.idProduct == product);
	}
	return r;
}

int driver_to_be_reattached = 0;

int detach_driver(libusb_device_handle *handle, int interface_number)
{
	int r;
	if (usb_return(libusb_kernel_driver_active(handle, interface_number), "detach_driver: libusb_kernel_driver_active")) {
		r = usb_return(libusb_detach_kernel_driver(handle, interface_number), "detach_driver: libusb_detach_kernel_driver");
		driver_to_be_reattached = 1;
	}
	return r;
}

int set_configuration(libusb_device_handle *handle, int configuration)
{
	int r;
	r = usb_return(libusb_set_configuration(handle, configuration), "libusb_set_configuration");
	return r;
}

int claim_interface(libusb_device_handle *handle, int interface_number)
{
	int r;
	r = usb_return(libusb_claim_interface(handle, interface_number), "libusb_claim_interface");
	return r;
}

int release_interface(libusb_device_handle *handle, int interface_number)
{
	int r;
	r = usb_return(libusb_release_interface(handle, interface_number), "libusb_release_interface");
	return 1;
}

int restore_driver(libusb_device_handle *handle, int interface_number)
{
	int r = 0;
	if (driver_to_be_reattached) {
		r = usb_return(libusb_attach_kernel_driver(handle, interface_number), "libusb_attach_kernel_driver");
	}
	return r;
}

#define CTRL_REQ_TYPE 0x21
#define CTRL_REQ 0x09
#define CTRL_VALUE 0x0200

int control_message(libusb_device_handle *handle, uint16_t index, const unsigned char *pquestion, int qlength) 
{
	int r;
	unsigned char question[qlength];
    
	memcpy(question, pquestion, qlength);

	r = usb_return(libusb_control_transfer(handle, CTRL_REQ_TYPE, CTRL_REQ, CTRL_VALUE, index, 
			(unsigned char *) question, qlength, CONTROL_TIMEOUT),
			"libusb_control_transfer");
	return r;
}

#define READ_ENDPOINT 0x82

int interrupt_read(libusb_device_handle *handle, unsigned char *data, int datalength)
{
	int r, total_transferred = 0, transferred = 0;

	do {
		r = usb_return(libusb_interrupt_transfer(handle, READ_ENDPOINT, data, datalength, &transferred, READ_TIMEOUT),
			"libusb_interrupt_transfer");
		total_transferred += transferred;
//		fprintf(stdout, "tx: %d, %d\n", datalength, transferred);
	}
	while (r == 0 && total_transferred < datalength);
	
	return r;
}
