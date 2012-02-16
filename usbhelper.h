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

void iterate_usb(int (is_interesting)(struct usb_device *), int (do_process)(struct usb_dev_handle *));

int device_vendor_product_is(struct usb_device *device, u_int16_t vendor, u_int16_t product);
int device_bus_address(struct usb_dev_handle *handle, u_int8_t *bus_id, u_int8_t *device_id);

int detach_driver(usb_dev_handle *handle, int interface_number);
int set_configuration(usb_dev_handle *handle, int configuration);
int claim_interface(usb_dev_handle *handle, int interface_number);
int control_message(usb_dev_handle *handle, u_int16_t index, const char *pquestion, int qlength);
int interrupt_read(usb_dev_handle *handle, char *data, int datalength);
int release_interface(usb_dev_handle *handle, int interface_number);
int restore_driver(usb_dev_handle *handle, int interface_number);

