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

