#include "lifebar.h"

#define PS_PATH "/sys/class/power_supply"

int count_acpi_batteries() {
	DIR *d;
	struct dirent *de;
	int count = 0;

	d = opendir(PS_PATH);
	if(d == NULL) return 0;
	while((de = readdir(d)) != NULL) {
		if(strstr(de->d_name, "BAT") == de->d_name) {
			//name starts with BAT
			count++;
		}
	}
	closedir(d);

	return count;
}

void read_acpi_battery(int b, struct batt_info *bi) {
	//we assume the battery index to exist as filename BAT<index>
}
