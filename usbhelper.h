void iterate_usb(int (is_interesting)(libusb_device *),
	int (do_process)(libusb_device_handle *));
int device_vendor_product_is(libusb_device *device, uint16_t vendor, uint16_t product);
int detach_driver(libusb_device_handle *handle, int interface_number);
int set_configuration(libusb_device_handle *handle, int configuration);
int claim_interface(libusb_device_handle *handle, int interface_number);
int control_message(libusb_device_handle *handle, uint16_t index, const unsigned char *pquestion, int qlength);
int interrupt_read(libusb_device_handle *handle, unsigned char *data, int datalength);
int release_interface(libusb_device_handle *handle, int interface_number);
int restore_driver(libusb_device_handle *handle, int interface_number);

