/*
 * usbhelper.c by Andrew Mannering (c) 2012 (andrewm@sledgehammersolutions.co.uk)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include "usbhelper.h"

#define CONTROL_TIMEOUT 5000
#define READ_TIMEOUT 5000

static struct usb_dev_handle *open_handle_for_device(struct usb_device *, 
	int (do_open)(struct usb_dev_handle *));
static void close_handle(struct usb_dev_handle *handle, int (do_open)(struct usb_dev_handle *));

static void error(char *info, int retcode)
{
	fprintf(stderr, "usbhelper: %s %d\n", info, retcode);
	fflush(stderr);
}

static int usb_return(int retcode, char *info)
{
	if (retcode < 0) {
		error(info, retcode);
	}
	return retcode;
}

void initialise_usb()
{
	usb_init();
	
	#ifdef DEBUG
	usb_set_debug(2);
	#endif
}

void iterate_usb(int (is_interesting)(struct usb_device *), 
	int (do_open)(struct usb_dev_handle *),
	int (do_process)(struct usb_dev_handle *),
	int (do_close)(struct usb_dev_handle *)
	)
{
	usb_find_busses();
	usb_find_devices();

	struct usb_bus *bus;
	struct usb_device *dev;
 
	for (bus = usb_busses; bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next) {
			if (is_interesting(dev)) {
				struct usb_dev_handle *handle = open_handle_for_device(dev, do_open);

				if (do_process) 
					do_process(handle);
				
				if (do_close)
					close_handle(handle, do_close);
			}
		}
	}

}

device_handle *device_handles = NULL;

static device_handle *add_device_handle(struct usb_device *dev, struct usb_dev_handle *handle)
{
	device_handle *dh = device_handles;
	device_handle *dc = NULL;

	while(dh != NULL && (dh = (device_handle *)dh->next));
	
	dc = (device_handle *)malloc(sizeof(device_handle));
	dc->device = dev;
	dc->handle = handle;
	dc->next = NULL;

	if (dh == NULL) device_handles = dh = dc;
	else dh->next = (device_handle *)dc;
	
	printf("Added: %p (%p, %p)\n", dc, dc->device, dc->handle);
	return dc;
}

static device_handle *get_device_handle_by_device(struct usb_device *dev)
{
	device_handle *dh = device_handles;
	if (dh == NULL)
		printf("Head not found\n");
	else
		printf("Head : %p (%p, %p)\n", dh, dh->device, dh->handle);

	while(dh != NULL) {
		printf("Test : %p (%p, %p)\n", dh, dh->device, dh->handle);
		if (dh->device == dev) {
			break;
		}
		dh = (device_handle *)dh->next;
	}

	if (dh == NULL) {
		printf("Not found for %p\n", dev);
	}
	else
		printf("Found: %p (%p, %p)\n", dh, dh->device, dh->handle);
	return dh;
}

static struct usb_dev_handle *open_handle_for_device(struct usb_device *dev, 
	int (do_open)(struct usb_dev_handle *))
{
	struct device_handle *dh = get_device_handle_by_device(dev);
	struct usb_dev_handle *handle;

	if (!dh)	{
		handle = usb_open(dev);
		if (do_open) 
			do_open(handle);
		// and stash new handle
		dh = add_device_handle(dev, handle);
	}
	return dh->handle;
}

static void close_handle(struct usb_dev_handle *handle, 
	int (do_close)(struct usb_dev_handle *))
{
	if (do_close) {
		do_close(handle);
		usb_close(handle);
	}
}

int device_vendor_product_is(struct usb_device *device, u_int16_t vendor, u_int16_t product)
{
	return (device->descriptor.idVendor == vendor && device->descriptor.idProduct == product);
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
	r = usb_return(usb_set_configuration(handle, configuration), "usb_set_configuration");
	return r;
}

int claim_interface(struct usb_dev_handle *handle, int interface_number)
{
	int r;
	r = usb_return(usb_claim_interface(handle, interface_number), "usb_claim_interface");
	return r;
}

int release_interface(struct usb_dev_handle *handle, int interface_number)
{
	int r;
	r = usb_return(usb_release_interface(handle, interface_number), "usb_release_interface");
	return 1;
}

int restore_driver(struct usb_dev_handle *handle, int interface_number)
{
	return 0;
}

int control_message(struct usb_dev_handle *handle, int requesttype, int request, int value, 
	int index, const char *pquestion, int qlength) 
{
	int r;
	unsigned char question[qlength];
    
	memcpy(question, pquestion, qlength);

	r = usb_return(usb_control_msg(handle, requesttype, request, value, index, 
			(char *) question, qlength, CONTROL_TIMEOUT),
			"usb_control_msg");
	return r;
}

int interrupt_read(struct usb_dev_handle *handle, int ep, char *data, int datalength)
{
	int r;

	r = usb_return(usb_interrupt_read(handle, ep, data, datalength, READ_TIMEOUT),
			"usb_interrupt_read");
	
	return r;
}

int handle_bus_address(struct usb_dev_handle *handle, u_int8_t *bus_id, u_int8_t *device_id)
{
	struct usb_device *device = usb_device(handle);
	*bus_id = atoi(device->bus->dirname);
	*device_id = device->devnum;
	
	return (bus_id > 0 && device_id > 0);
}

static const char root_sys_class_usb_device[] = "/sys/class/usb_device";
int handle_bus_port(struct usb_dev_handle *handle, char *bus_port)
{
	// Use sysfs (but beware of older versions) to get port number
	
	char devicepath[FILENAME_MAX - 1], testpath[FILENAME_MAX - 1];
	u_int8_t bus_id, device_id;
	handle_bus_address(handle, &bus_id, &device_id);
	
	sprintf(devicepath, "%s/usbdev%d.%d/device", root_sys_class_usb_device, bus_id, device_id);

	sprintf(testpath, "%s/busnum", devicepath);
	FILE *fbn = fopen(testpath,"r");
	sprintf(testpath, "%s/devpath", devicepath);
	FILE *fdp = fopen(testpath,"r");
	if( fbn && fdp ) {
		printf("device_bus_port: mode 1 - modern sysfs\n");
		// Ahh, good. Modern sysfs implementation, so do this properly.
		char sysfs_busnum[4] = {};
		fgets(sysfs_busnum, 4, fbn);
		if (fbn) fclose(fbn);

		char sysfs_devpath[32] = {};
		fgets(sysfs_devpath, 32, fdp);
		if (fdp) fclose(fdp);
		
		sprintf(bus_port, "%s-%s", sysfs_busnum, sysfs_devpath);
	} 
	else {
		// Crud. Probably lame old sysfs implementation, try the bodge method
		char linkpath[FILENAME_MAX - 1];
		if (readlink(devicepath, linkpath, FILENAME_MAX) > 0) {
			printf("device_bus_port: mode 2 - elderly sysfs\n");
			// take the last bit off the link target
			char *bodge = basename(linkpath);
			strncpy(bus_port, bodge, strlen(bodge));
		}
		else {
			// No way to get the values. Return the bus-address instead
			printf("device_bus_port: mode 99 - no sysfs - lie\n");		
			sprintf(bus_port, "%d-%d", bus_id, device_id);
		}
	}
	
	printf("device_bus_port: (%d, %d) => (%s) %p\n", bus_id, device_id, bus_port, bus_port);
	return (1);
}
