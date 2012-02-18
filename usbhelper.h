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

#include <usb.h>

#ifndef FALSE
#define FALSE (0)
#define TRUE (!(FALSE))
#endif

void initialise_usb();
void iterate_usb(int (is_interesting)(struct usb_device *), 
	int (do_open)(struct usb_dev_handle *),
	int (do_process)(struct usb_dev_handle *),
	int (do_close)(struct usb_dev_handle *)
	);

int device_vendor_product_is(struct usb_device *device, u_int16_t vendor, u_int16_t product);
int device_bus_address(struct usb_dev_handle *handle, u_int8_t *bus_id, u_int8_t *device_id);

int detach_driver(usb_dev_handle *handle, int interface_number);
int set_configuration(usb_dev_handle *handle, int configuration);
int claim_interface(usb_dev_handle *handle, int interface_number);
int control_message(struct usb_dev_handle *handle, int requesttype, int request, int value, 
	int index, const char *pquestion, int qlength);
int interrupt_read(usb_dev_handle *handle, int ep, char *data, int datalength);
int release_interface(usb_dev_handle *handle, int interface_number);
int restore_driver(usb_dev_handle *handle, int interface_number);

typedef struct device_handle {
	struct usb_device *device;
	struct usb_dev_handle *handle;
	struct device_handle *next;
} device_handle;

