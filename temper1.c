/*
 * temper1.c by Andrew Mannering (c) 2012 (andrewm@sledgehammersolutions.co.uk)
 * based on pcsensor.c by Michitaka Ohno (c) 2011 (elpeo@mars.dti.ne.jp)
 * based oc pcsensor.c by Juan Carlos Perez (c) 2011 (cray@isp-sl.com)
 * based on Temper.c by Robert Kavaler (c) 2009 (relavak.com)
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
#include <time.h>

#include "usbhelper.h"

#define VENDOR_ID 0x0c45
#define PRODUCT_ID 0x7401
#define INTERFACE0 0
#define INTERFACE1 1

// Forward declarations
static int is_device_temper1(struct usb_device *device);
static int use_device_temper1(struct usb_dev_handle *handle);
static int initialise_temper1(struct usb_dev_handle *handle);
static int read_temper1(struct usb_dev_handle *handle, char *data);
static int close_temper1(struct usb_dev_handle *handle);

static float raw_to_c(u_int8_t bus_id, u_int8_t device_id, char *data);

// Main...
int main(int argc, char *argv[])
{
	iterate_usb(is_device_temper1, use_device_temper1);
	return 0;
}

// Worker methods
static void output_data(u_int8_t bus_id, u_int8_t device_id, char *data)
{
	struct tm *utc;
	time_t t;
	t = time(NULL);
	utc = gmtime(&t);
	
	char dt[80];
	strftime(dt, 80, "%d-%b-%Y %H:%M", utc);

	float c = raw_to_c(bus_id, device_id, data);

	fprintf(stdout, "%s,%d,%d,%f\n", dt, bus_id, device_id, c);
//	fprintf(stdout, "%s,%f\n", dt, c);
	fflush(stdout);
}

static int is_device_temper1(struct usb_device *device)
{
	return device_vendor_product_is(device, VENDOR_ID, PRODUCT_ID);
}

static int use_device_temper1(struct usb_dev_handle *handle)
{
	int r;
	u_int8_t bus_id, device_id;
	char data[8];
	
	r = device_bus_address(handle, &bus_id, &device_id);
	
	r = initialise_temper1(handle);
	if (r >= 0)
		r = read_temper1(handle, data);
		
	if (r >= 0) 
		output_data(bus_id, device_id, data);
	
	r = close_temper1(handle);
			
	return r;
}

// Todo: find way to identify device, and then apply specific calibration variables
// Poss: use udev to catch change of device, and write virtual address (which we can get at in libusb)
//       to a file indexed by physical address from udev. Can then use a file to map physical
//	     port to calibration values.
/* Calibration adjustments */
/* See http://www.pitt-pladdy.com/blog/_20110824-191017_0100_TEMPer_under_Linux_perl_with_Cacti/ */
static float scale = 1.0287;
static float offset = -0.85;

static float raw_to_c(u_int8_t bus_id, u_int8_t device_id, char *data)
{
	unsigned int rawtemp = (data[3] & 0xFF) + (data[2] << 8);
	float temp_c = rawtemp * (125.0 / 32000.0);
	temp_c = (temp_c * scale) + offset;
	return temp_c;
}

const static char cq_initialise[] = { 0x01, 0x01 };

static int initialise_temper1(struct usb_dev_handle *handle)
{
	int r = detach_driver(handle, INTERFACE0);
	if (r >= 0) r = 
		detach_driver(handle, INTERFACE1);
	if (r >= 0) r = 
		set_configuration(handle, 0x01);
	if (r >= 0) r = 
		claim_interface(handle, INTERFACE0);
	if (r >= 0) r = 
		claim_interface(handle, INTERFACE1);
	if (r >= 0) r = 
		control_message(handle, 0, cq_initialise, sizeof(cq_initialise));
		
	return r;
}

const static char cq_temperature[] = { 0x01, 0x80, 0x33, 0x01, 0x00, 0x00, 0x00, 0x00 };

static int read_temper1(struct usb_dev_handle *handle, char *data)
{
	int r = 
		control_message(handle, 1, cq_temperature, sizeof(cq_temperature));
	if (r >= 0) r = 
		interrupt_read(handle, data, sizeof(data));
	
	return r;
}

static int close_temper1(struct usb_dev_handle *handle)
{
	int r = 
		release_interface(handle, INTERFACE0);	
	if (r >= 0) r = 
		release_interface(handle, INTERFACE1);
	if (r >= 0) r = 
		restore_driver(handle, INTERFACE0);
	if (r >= 0) r = 
		restore_driver(handle, INTERFACE1);
		
	return r;
}
