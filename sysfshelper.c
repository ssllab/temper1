/*
 * sysfshelper.c by Andrew Mannering (c) 2012 (andrewm@sledgehammersolutions.co.uk)
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

static int file_exists(const char *filepath) 
{
	struct stat filestat;
	return (((stat(filepath, &filestat)) == 0));
}

int sysfs_find_usb_device_name(const int busnum, const int devnum, char * const usbname) 
{
	const char root_sys_bus_usb_devices[] = "/sys/bus/usb/devices";

	struct dirent *entry;
	char busnum_part[5];
	char devnum_path[FILENAME_MAX - 1];
	DIR *dp;
	int r = -1;
 
	dp = opendir(root_sys_bus_usb_devices);
	if (dp == NULL) {
		perror("opendir");
		return -1;
	}
 
	while((entry = readdir(dp))) 
	{
		if ((strncmp(entry->d_name, ".", 1) == 0) || (strncmp(entry->d_name, "..", 2) == 0))
			continue;
		sprintf(busnum_part, "%i", busnum);
		sprintf(devnum_path, "%s/%s/devnum", root_sys_bus_usb_devices, entry->d_name);
		if ((strncmp(entry->d_name, busnum_part, strlen(busnum_part)) == 0) && 
				file_exists(devnum_path)) {
			
			char devnum_string[5];
			FILE *fpdn = fopen(devnum_path, "r");
			if (fpdn == NULL) {
				perror("Failed to open devnum file");
				return 2;
			}
			fgets(devnum_string, 5, fpdn);
			fclose(fpdn);
			
			if (atoi(devnum_string) == devnum) {
				r = devnum;
				strncpy(usbname, entry->d_name, strlen(entry->d_name));
			}
		
		}
	}
	
	closedir(dp);
	return r;
}
