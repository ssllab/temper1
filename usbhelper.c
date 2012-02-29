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
#include "sysfshelper.h"

#define CONTROL_TIMEOUT 5000
#define READ_TIMEOUT 5000

static struct usb_dev_handle *open_handle_for_device(struct usb_device *, 
	int (do_open)(struct usb_dev_handle *));
static int close_handle(struct usb_dev_handle *handle, int (do_open)(struct usb_dev_handle *));
static int debug = FALSE;

static void error(char *info, int retcode)
{
	if (debug) fprintf(stderr, "usbhelper: %s %d\n", info, retcode);
	fflush(stderr);
}

static int usb_return(int retcode, char *info)
{
	if (retcode < 0) {
		error(info, retcode);
	}
	return retcode;
}

void initialise_usb(int verbose)
{
	usb_init();
	
	if (verbose) {
		fprintf(stderr, "Verbose mode enabled\n");
		usb_set_debug(2);
		debug = verbose;
	}
}

int iterate_usb(int (is_interesting)(struct usb_device *), 
	int (do_open)(struct usb_dev_handle *),
	int (do_process)(struct usb_dev_handle *),
	int (do_close)(struct usb_dev_handle *)
	)
{
	usb_find_busses();
	usb_find_devices();

	int result = 0;
	struct usb_bus *bus;
	struct usb_device *dev;
 
	for (bus = usb_busses; bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next) {
			if (is_interesting(dev)) {
				struct usb_dev_handle *handle = open_handle_for_device(dev, do_open);
				if (handle) {
					if (do_process) 
						result += do_process(handle);
					
					if (do_close)
						result += close_handle(handle, do_close);
				}
				else {
					result += 1;
				}
			}
		}
	}
	return result;
}

device_handle *device_handles = NULL;

static device_handle *add_device_handle(struct usb_device *dev, struct usb_dev_handle *handle)
{
	device_handle *dh = device_handles;
	device_handle *dc = NULL;

	if (dh) {
		while (dh->next)
			dh = (device_handle *)dh->next;
	}
	
	dc = (device_handle *)malloc(sizeof(device_handle));
	dc->device = dev;
	dc->handle = handle;
	dc->next = NULL;

	if (dh == NULL) device_handles = dh = dc;
	else dh->next = (device_handle *)dc;
	return dc;
}

static device_handle *get_device_handle_by_device(struct usb_device *dev)
{
	device_handle *dh = device_handles;
	while(dh != NULL) {
		if (dh->device == dev) {
			break;
		}
		dh = (device_handle *)dh->next;
	}

//fprintf(stderr, "Returning handle struct %p by device %p\n", dh, dev); fflush(stderr);
	return dh;
}

static struct usb_dev_handle *open_handle_for_device(struct usb_device *dev, 
	int (do_open)(struct usb_dev_handle *))
{
	struct device_handle *dh = get_device_handle_by_device(dev);
	struct usb_dev_handle *handle;
	int r;

	if (!dh) {
		handle = usb_open(dev);
		if (do_open) 
			r = do_open(handle);
		// and stash new handle
		if (r >= 0)
			dh = add_device_handle(dev, handle);
	}
	return dh->handle;
}

static int close_handle(struct usb_dev_handle *handle, 
	int (do_close)(struct usb_dev_handle *))
{
	int r = 0;
	if (do_close) {
		r = do_close(handle);
		usb_close(handle);
	}
	return r;
}

int device_vendor_product_is(struct usb_device *device, u_int16_t vendor, u_int16_t product)
{
	int r = (device->descriptor.idVendor == vendor && device->descriptor.idProduct == product);
	return r;
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

int handle_bus_port(struct usb_dev_handle *handle, char *bus_port)
{
	// Use sysfs (but beware of older versions) to get port number
	
	u_int8_t bus_id, device_id;
	handle_bus_address(handle, &bus_id, &device_id);
	
	sysfs_find_usb_device_name(bus_id, device_id, bus_port);
	
	if (debug) fprintf(stderr, "device_bus_port: (%d, %d) => (%s) %p\n", bus_id, device_id, bus_port, bus_port);
	return (1);
}
