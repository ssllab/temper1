#include <stdio.h>
#include <time.h>
#include <libusb-1.0/libusb.h>

#include "usbhelper.h"

#define VENDOR_ID 0x0c45
#define PRODUCT_ID 0x7401
#define INTERFACE0 0
#define INTERFACE1 1

int is_device_temper1(libusb_device *device);
int use_device_temper1(libusb_device_handle *handle);
int initialise_temper1(libusb_device_handle *handle);
int read_temper1(libusb_device_handle *handle, unsigned char *data);
int close_temper1(libusb_device_handle *handle);

float raw_to_c(uint8_t bus_id, uint8_t device_id, unsigned char *data);

int main ()
{
	iterate_usb(is_device_temper1, use_device_temper1);
	return 0;
}

void output_data(uint8_t bus_id, uint8_t device_id, unsigned char *data)
{
	struct tm *utc;
	time_t t;
	t = time(NULL);
	utc = gmtime(&t);
	
	char dt[80];
	strftime(dt, 80, "%d-%b-%Y %H:%M", utc);

	float c = raw_to_c(bus_id, device_id, data);

//	fprintf(stdout, "%s,%d,%d,%f\n", dt, bus_id, device_id, c);
	fprintf(stdout, "%s,%f\n", dt, c);
	fflush(stdout);
}

int is_device_temper1(libusb_device *device)
{
	return device_vendor_product_is(device, VENDOR_ID, PRODUCT_ID);
}

int use_device_temper1(libusb_device_handle *handle)
{
	int r;
	uint8_t bus_id, device_id;
	unsigned char data[8];
	
	r = device_bus_address(libusb_get_device(handle), &bus_id, &device_id);
	
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
static float scale = 1.0287;
static float offset = -0.85;

float raw_to_c(uint8_t bus_id, uint8_t device_id, unsigned char *data)
{
	unsigned int rawtemp = (data[3] & 0xFF) + (data[2] << 8);
	float temp_c = rawtemp * (125.0 / 32000.0);
	temp_c = (temp_c * scale) + offset;
	return temp_c;
}

const static unsigned char cq_initialise[] = { 0x01, 0x01 };

int initialise_temper1(libusb_device_handle *handle)
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

const static unsigned char cq_temperature[] = { 0x01, 0x80, 0x33, 0x01, 0x00, 0x00, 0x00, 0x00 };

int read_temper1(libusb_device_handle *handle, unsigned char *data)
{
	int r = 
		control_message(handle, 1, cq_temperature, sizeof(cq_temperature));
	if (r >= 0) r = 
		interrupt_read(handle, data, sizeof(data));
	
	return r;
}

int close_temper1(libusb_device_handle *handle)
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
