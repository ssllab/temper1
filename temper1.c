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
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>

#include "usbhelper.h"

#define VERSION "0.1"

#define VENDOR_ID 0x0c45
#define PRODUCT_ID 0x7401
#define INTERFACE0 0
#define INTERFACE1 1

// Forward declarations
static void load_calibrations();
static int is_device_temper1(struct usb_device *device);
static int initialise_temper1(struct usb_dev_handle *handle);
static int use_temper1(struct usb_dev_handle *handle);
static int read_temper1(struct usb_dev_handle *handle, char *data);
static int close_temper1(struct usb_dev_handle *handle);

static float raw_to_c(u_int8_t bus_id, u_int8_t device_id, char *data);
static float c_to_u(float deg_c, char unit);

typedef struct options {
	int verbose;
	int daemon;
	char *output_file;
	char *config_file;
	char units;
	char *format;
} options;

options opts;

// Main...
int main(int argc, char *argv[])
{
	opts.verbose = FALSE;
	opts.daemon = FALSE;
	opts.output_file = "stdout";
	opts.config_file = "temper1.conf";
	opts.units = 'C';
	opts.format = "%d-%b-%Y %H:%M";
	
	static struct option long_options[] =
	 {
	   {"help", no_argument,          0, 'h'},
	   {"version", no_argument,       0, 'V'},
	   {"verbose", no_argument,       0, 'v'},
	   {"daemon",  required_argument, 0, 'd'},
	   {"output",  required_argument, 0, 'o'},
	   {"config",  required_argument, 0, 'c'},
	   {"units",   required_argument, 0, 'u'},
	   {"timestamp",  required_argument, 0, 't'},
	   {0, 0, 0, 0}
	 };
	int options_index = 0, c = 0, proceed = TRUE;
	
	while ((c = getopt_long(argc, argv, "hVvd:o:c:u:t:", long_options, &options_index)) != -1) 
	{
		switch (c) {
			case 'h':
				// TODO
				proceed = FALSE;
				break;
			case 'V':
				proceed = FALSE;
				fprintf(stdout, "temper1 version %s\n", VERSION);
				break;
			case 'v':
				opts.verbose = TRUE;
				break;
			case 'd':
				opts.daemon = TRUE;
				break;
			case 'o':
				break;
			case 'c':
				break;
			case 'u': {
					int u;
					char unit = toupper(optarg[0]);
					char units[] = { 'C', 'F', 'K' };
					for (u = 0; u < sizeof(units); u++) {
						if (units[u] == unit) {
							opts.units = units[u];
							break;
						}
					}
					break;
				}
			case 't':
				break;
			case '?':
				proceed = FALSE;
				break;
			default:
				break;
		}
	}
	
	if (proceed) {
		initialise_usb();
		load_calibrations();
		
	/* // This is the one shot read
		iterate_usb(is_device_temper1, 
					initialise_temper1, use_temper1, close_temper1);
	*/
	
		// These separate iterations allow for the use_temper1 to do loops over all devices (daemon mode)
		iterate_usb(is_device_temper1, initialise_temper1, NULL, NULL);
		iterate_usb(is_device_temper1, NULL, use_temper1, NULL);
		iterate_usb(is_device_temper1, NULL, NULL, close_temper1);
	}
	
	return (!proceed);
}

// Worker methods
static void output_data(u_int8_t bus_id, u_int8_t device_id, char *data)
{
	struct tm *utc;
	time_t tm;
	tm = time(NULL);
	utc = gmtime(&tm);
	
	char dt[80];
	strftime(dt, 80, "%d-%b-%Y %H:%M", utc);

	float t = c_to_u(raw_to_c(bus_id, device_id, data), opts.units);

	fprintf(stdout, "(%ld) %s,%d,%d,%f\n", tm, dt, bus_id, device_id, t);
//	fprintf(stdout, "%s,%f\n", dt, t);
	fflush(stdout);
}

static int is_device_temper1(struct usb_device *device)
{
	return device_vendor_product_is(device, VENDOR_ID, PRODUCT_ID);
}

static int use_temper1(struct usb_dev_handle *handle)
{
	int r;
	u_int8_t bus_id, device_id;
	char data[8];
	
	r = device_bus_address(handle, &bus_id, &device_id);
	
	if (r >= 0)
		r = read_temper1(handle, data);
		
	if (r >= 0) 
		output_data(bus_id, device_id, data);
	
	return r;
}


typedef struct temper1_calibration {
	char *port_descriptor;
	float scale;
	float offset;
} temper1_calibration;
temper1_calibration temper1_calibrations[127];

static void load_calibrations() 
{
	// TODO
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

static float c_to_u(float deg_c, char unit)
{
	if (unit == 'F')
		return (deg_c * 1.8) + 32.0;
	else if (unit == 'K')
		return (deg_c + 273.15);
	else
		return deg_c;
}

#define CTRL_REQ_TYPE 0x21
#define CTRL_REQ 0x09
#define CTRL_VALUE 0x0200
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
		control_message(handle, CTRL_REQ_TYPE, CTRL_REQ, CTRL_VALUE, 
			0, cq_initialise, sizeof(cq_initialise));
		
	return r;
}

#define READ_ENDPOINT 0x82
const static char cq_temperature[] = { 0x01, 0x80, 0x33, 0x01, 0x00, 0x00, 0x00, 0x00 };

static int read_temper1(struct usb_dev_handle *handle, char *data)
{
	int r = 
		control_message(handle, CTRL_REQ_TYPE, CTRL_REQ, CTRL_VALUE, 
			1, cq_temperature, sizeof(cq_temperature));
	if (r >= 0) r = 
		interrupt_read(handle, READ_ENDPOINT, data, sizeof(data));
	
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
