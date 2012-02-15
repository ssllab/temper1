#include <stdio.h>
#include <string.h>
#include <usb.h>

#define CONTROL_TIMEOUT 5000
#define READ_TIMEOUT 5000

int open_and_callback(struct usb_device *device, int (action)(struct usb_dev_handle *));

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

void iterate_usb(int (is_interesting)(struct usb_device *),
	int (do_process)(struct usb_dev_handle *))
{
	usb_init();
	usb_find_busses();
	usb_find_devices();
	
	#ifdef DEBUG
	usb_set_debug(2);
	#endif
	
	struct usb_bus *bus;
	struct usb_device *dev;
 
	for (bus = usb_busses; bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next) {
			if (is_interesting(dev)) {
				open_and_callback(dev, do_process);
			}
		}
	}

}

int open_and_callback(struct usb_device *device, int(action)(struct usb_dev_handle *))
{
	struct usb_dev_handle *handle;
	
	handle = usb_open(device);
	if (handle) {
		if (action)
			action(handle);
		usb_close(handle);
	}
	return (0);
}
	
int device_vendor_product_is(struct usb_device *device, u_int16_t vendor, u_int16_t product)
{
	int r;

	r = (device->descriptor.idVendor == vendor && device->descriptor.idProduct == product);

	return r;
}

int device_bus_address(struct usb_dev_handle *handle, u_int8_t *bus_id, u_int8_t *device_id)
{
	struct usb_device *device = usb_device(handle);
	*bus_id = device->bus->location;
	*device_id = device->devnum;
	
	return (bus_id > 0 && device_id > 0);
}

int detach_driver(struct usb_dev_handle *handle, int interface_number)
{
	int r;
	r = usb_detach_kernel_driver_np(handle, interface_number);
	r = usb_return(((r == -61)? 0 : r), "detach_driver: usb_detach_kernel_driver_np");
	
	return r;
}

int set_configuration(struct usb_dev_handle *handle, int configuration)
{
	int r;
	r = usb_return(usb_set_configuration(handle, configuration), "libusb_set_configuration");
	return r;
}

int claim_interface(struct usb_dev_handle *handle, int interface_number)
{
	int r;
	r = usb_return(usb_claim_interface(handle, interface_number), "libusb_claim_interface");
	return r;
}

int release_interface(struct usb_dev_handle *handle, int interface_number)
{
	int r;
	r = usb_return(usb_release_interface(handle, interface_number), "libusb_release_interface");
	return 1;
}

int restore_driver(struct usb_dev_handle *handle, int interface_number)
{
	return 0;
}

#define CTRL_REQ_TYPE 0x21
#define CTRL_REQ 0x09
#define CTRL_VALUE 0x0200

int control_message(struct usb_dev_handle *handle, u_int16_t index, const char *pquestion, int qlength) 
{
	int r;
	unsigned char question[qlength];
    
	memcpy(question, pquestion, qlength);

	r = usb_return(usb_control_msg(handle, CTRL_REQ_TYPE, CTRL_REQ, CTRL_VALUE, index, 
			(char *) question, qlength, CONTROL_TIMEOUT),
			"libusb_control_transfer");
	return r;
}

#define READ_ENDPOINT 0x82

int interrupt_read(struct usb_dev_handle *handle, char *data, int datalength)
{
	int r;

	r = usb_return(usb_interrupt_read(handle, READ_ENDPOINT, data, datalength, READ_TIMEOUT),
			"libusb_interrupt_transfer");
	
	return r;
}
