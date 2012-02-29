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

int sysfs_find_device_id(const int busnum, const int devnum, char * const usbname) 
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
