/*
 * temper1.c by Andrew Mannering (c) 2012 (andrewm@sledgehammersolutions.co.uk)
 * based on pcsensor.c by Michitaka Ohno (c) 2011 (elpeo@mars.dti.ne.jp)
 * based on pcsensor.c by Juan Carlos Perez (c) 2011 (cray@isp-sl.com)
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
#include <string.h>
#include <time.h>

#include "usbhelper.h"
#include "strreplace.h"

#define VERSION "0.1"

#define VENDOR_ID 0x0c45
#define PRODUCT_ID 0x7401
#define INTERFACE0 0
#define INTERFACE1 1

// Forward declarations
static void load_configuration();
static void load_calibrations();
static int is_device_temper1(struct usb_device *device);
static int initialise_temper1(struct usb_dev_handle *handle);
static int use_temper1(struct usb_dev_handle *handle);
static int read_temper1(struct usb_dev_handle *handle, char *data, int datalen);
static int close_temper1(struct usb_dev_handle *handle);

static void parse_units(char *arg);
static int decode_raw_data(char *data);
static float raw_to_c(char *busport, int rawtemp);
static float c_to_u(float deg_c, char unit);

typedef struct options {
	int verbose;
	int daemon;
	char output_file[FILENAME_MAX];
	char config_file[FILENAME_MAX];
	char units;
	char dt_format[100];
	char only_device[40];
} options;

options opts;

// Main...
int main(int argc, char *argv[])
{
	opts.verbose = FALSE;
	opts.daemon = FALSE;
	bzero(opts.output_file, FILENAME_MAX);
	strcpy(opts.config_file, "temper1.conf");
	opts.units = 'C';
	strcpy(opts.dt_format, "%d-%b-%Y %H:%M");
	bzero(opts.only_device, 40);
	
	static struct option long_options[] =
	 {
	   {"help", no_argument,          0, 'h'},
	   {"version", no_argument,       0, 'V'},
	   {"verbose", no_argument,       0, 'v'},
	   {"daemon",  required_argument, 0, 'D'},
	   {"config",  required_argument, 0, 'C'},
	   {"output",  required_argument, 0, 'o'},
	   {"units",   required_argument, 0, 'u'},
	   {"device",  required_argument, 0, 'd'},
	   {0, 0, 0, 0}
	 };
	int options_index = 0, c = 0, proceed = TRUE;
	
	while ((c = getopt_long(argc, argv, "hVvD:C:o:u:d:", long_options, &options_index)) != -1) 
	{
		switch (c) {
			case 'C':
				strcpy(opts.config_file, optarg);
				break;
		}
	}
	load_configuration();
	
	optind = 1;
	while ((c = getopt_long(argc, argv, "hVvD:C:o:u:d:", long_options, &options_index)) != -1) 
	{
		switch (c) {
			case 'h':
				fprintf(stdout, "usage: temper1 [--version|-V] [--help|-h] [--daemon|-D [seconds]]\n");
				fprintf(stdout, "               [--verbose|-v] [--config|-C [file]] [--output|-o [file]]\n");
				fprintf(stdout, "               [--units|-u [C|F|K]] [--device|-d [bus_no-port_no]]\n");
				proceed = FALSE;
				break;
			case 'V':
				proceed = FALSE;
				fprintf(stdout, "temper1 version %s\n", VERSION);
				break;
			case 'v':
				opts.verbose = TRUE;
				break;
			case 'D':
				opts.daemon = TRUE;
				break;
			case 'd':
				strcpy(opts.only_device, optarg);
				break;
			case 'o':
				strcpy(opts.output_file, optarg);
				break;
			case 'u': 
				parse_units(optarg);
				break;
			case 'C':
				// Just ignore as we've done this already
				break;
			case '?':
				proceed = FALSE;
				break;
			default:
				break;
		}
	}
	
	if (proceed) {	
		initialise_usb(opts.verbose);
		load_calibrations();
		
		if (!opts.daemon) {
			// This is the one shot read
			iterate_usb(is_device_temper1, 
						initialise_temper1, use_temper1, close_temper1);
		}
		else {
			// These separate iterations allow for the use_temper1 to do loops over all devices (daemon mode)
			// by simply using a different use_temper1 method pointer.
			int r = iterate_usb(is_device_temper1, initialise_temper1, NULL, NULL);
			if (r >= 0) r = iterate_usb(is_device_temper1, NULL, use_temper1, NULL);
			// TODO install signal handler to run this
			if (r >= 0) r = iterate_usb(is_device_temper1, NULL, NULL, close_temper1);
		}
	}
	
	return (!proceed);
}

static void parse_units(char *arg)
{
	int u;
	char unit = toupper(arg[0]);
	char units[] = { 'C', 'F', 'K' };
	for (u = 0; u < sizeof(units); u++) {
		if (units[u] == unit) {
			opts.units = units[u];
			break;
		}
	}
}

// Worker methods
static void output_data(char *busport, char *data)
{
	struct tm *utc;
	time_t tm;
	tm = time(NULL);
	utc = gmtime(&tm);
	
	char dt[80];
	strftime(dt, 80, opts.dt_format, utc);

	float t = c_to_u(raw_to_c(busport, decode_raw_data(data)), opts.units);

	FILE *fp = stdout;
	if (strlen(opts.output_file) > 0) {
		if (!(fp = fopen(opts.output_file, "w+"))) {
			fprintf(stderr, "Unable to open %s for appending\n", opts.output_file);
			fp = stdout;
		}
	}

// Uncomment this line to use the opts.dt_format output 
//	fprintf(fp, "%s,%s,%f\n", dt, busport, t);
// Otherwise we default to outputing a timestamp in seconds as this is easier
// to use in JQuery/Javascript and Oracle 
	fprintf(fp, "%ld,%f,%s\n", tm, t, busport) ;

	fflush(fp);
	if (fp != stdout) 
		fclose(fp);
}

static int is_device_temper1(struct usb_device *device)
{
	return device_vendor_product_is(device, VENDOR_ID, PRODUCT_ID);
}

static int use_temper1(struct usb_dev_handle *handle)
{
	int r, do_read = TRUE;
	char data[8];
	char busport[100] = {};
		
	r = handle_bus_port(handle, busport);

	if (strlen(opts.only_device) > 0) {
		do_read = (strcmp(busport, opts.only_device) == 0);
	}
	
	if (do_read) {
		bzero(data, 8);
		r = read_temper1(handle, data, 8);
		if (r >= 0) {
			if (decode_raw_data(data) > 0) { 
				output_data(busport, data);
			}
			else {
				if (opts.verbose) fprintf(stderr, "Read returned 0 value (r = %i)\n", r);
				r = -1;
			}
		}
		else {
			if (opts.verbose) fprintf(stderr, "use_temper1: read_temper1 returned (r = %i)\n", r);
		}
	}
		
	return r;
}

static void load_configuration()
{
	FILE *fp = fopen(opts.config_file, "r");
	if (fp) {
		char line[100];
		while (fgets(line, sizeof(line), fp) != NULL)
		{
			if (strstr(line, "OUTPUT\t") == line) {
			}
			else if (strstr(line, "FORMAT\t") == line) {
			}
			else if (strstr(line, "DATETIME\t") == line) {
			}
		}
		fclose(fp);
	}	
}

typedef struct temper1_calibration {
	char *port_descriptor;
	float scale;
	float offset;
} temper1_calibration;
static temper1_calibration temper1_calibrations[127] = {};

static void load_calibrations() 
{
	FILE *fp = fopen(opts.config_file, "r");
	if (fp) {
		char line[100];
		while (fgets(line, sizeof(line), fp) != NULL)
		{
			if (strstr(line, "CALIBRATION\t") == line) {
				char port[40] = {};
				float scale, offset;
				int s;
				if ((s = sscanf(line, "CALIBRATION\t%s\t%f\t%f", port, &scale, &offset))) {
					temper1_calibration cal;
					
					cal.port_descriptor = (char *)malloc(strlen(port));
				
					strncpy(cal.port_descriptor, port, strlen(port));
					cal.scale = scale;
					cal.offset = offset;	
					
					int i;
					for (i = 0; temper1_calibrations[i].port_descriptor; i++);
					temper1_calibrations[i] = cal;
					
					if (opts.verbose) fprintf(stderr, "Loaded calibration (%s): scale: %f; offset %f\n", port, cal.scale, cal.offset);
					
				}
			}
		}
		fclose(fp);
	}	
}

static temper1_calibration get_calibration(char *busport)
{
	int i = 0;
	for (i = 0; i < 128; i++) {
		if (temper1_calibrations[i].port_descriptor != NULL && 
			strcmp(temper1_calibrations[i].port_descriptor, busport) == 0) {
			break;
		}
	} 
		
	return temper1_calibrations[i];
} 

static int decode_raw_data(char *data)
{
	unsigned int rawtemp = (data[3] & 0xFF) + (data[2] << 8);
	if (opts.verbose) fprintf(stderr, 
		"Raw temp: %d (%04X) [%02X %02X (%02X >> %02X)+(%02X >> %02X) %02X %02X %02X %02X]\n", rawtemp, rawtemp,
		(unsigned)data[0] & 0xFF, (unsigned)data[1] & 0xFF, 
		(unsigned)data[2] & 0xFF, (unsigned)(data[2] << 8) ,
		(unsigned)data[3] & 0xFF, (unsigned)(data[3] & 0xFF), 
		(unsigned)data[4] & 0xFF, (unsigned)data[5] & 0xFF, (unsigned)data[6] & 0xFF, (unsigned)data[7] & 0xFF);
    
	/* msb means the temperature is negative -- less than 0 Celsius -- and in 2'complement form.
 	 * We can't be sure that the host uses 2's complement to store negative numbers
 	 * so if the temperature is negative, we 'manually' get its magnitude
 	 * by explicity getting it's 2's complement and then we return the negative of that.
 	 */
    
	if ((data[2] & 0x80) != 0) {
		/* return the negative of magnitude of the temperature */
		rawtemp = -((rawtemp ^ 0xffff) + 1);
	}
    
	return rawtemp;
}

/* Calibration adjustments */
/* See http://www.pitt-pladdy.com/blog/_20110824-191017_0100_TEMPer_under_Linux_perl_with_Cacti/ */
static float raw_to_c(char *busport, int rawtemp)
{
	float temp_c = rawtemp * (125.0 / 32000.0);
	
	temper1_calibration cal = get_calibration(busport);
	temp_c = (temp_c * (cal.scale == 0 ? 1.0 : cal.scale)) + cal.offset;
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
/*
	// In light of discovery that the HID implementation has a keyboard emulation
	// and a vendor specific mode, don't touch anything else. No init needed?
	if (r >= 0) r = 
		control_message(handle, CTRL_REQ_TYPE, CTRL_REQ, CTRL_VALUE, 
			0, cq_initialise, sizeof(cq_initialise));
		
*/
	return r;
}

#define READ_ENDPOINT 0x82
const static char cq_temperature[] = { 0x01, 0x80, 0x33, 0x01, 0x00, 0x00, 0x00, 0x00 };

static int read_temper1(struct usb_dev_handle *handle, char *data, int datalen)
{
	int r = control_message(handle, CTRL_REQ_TYPE, CTRL_REQ, CTRL_VALUE, 
			1, cq_temperature, sizeof(cq_temperature));
	if (r >= 0) r = 
		interrupt_read(handle, READ_ENDPOINT, data, datalen);
	
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
