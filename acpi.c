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
	//save the index 
	bi->index = b;

	//we assume the battery index to exist as filename BAT<index>
	char path[128];
	FILE *f;

	//status
	char status[2];
	sprintf(path, "%s/BAT%d/status", PS_PATH, b);
	f = fopen(path, "r");
	if(f == NULL || fgets(status, 2, f) == NULL) {
		fprintf(stderr, "%scould not read battery status: '%s'\n",
				BAD_MSG, path);
		bi->status = UNKNOWN;
	}
	else {
		switch(status[0]) {
			case 'C': bi->status = CHARGING; break;
			case 'D': bi->status = DISCHARGING; break;
			case 'F': bi->status = FULL; break;
			default: bi->status = UNKNOWN;
		}
	}
	fclose(f);

	//energy when full
	char energy_full_s[32];
	long int energy_full = 0;
	sprintf(path, "%s/BAT%d/energy_full", PS_PATH, b);
	f = fopen(path, "r");
	if(f == NULL || fgets(energy_full_s, 32, f) == NULL) {
		fprintf(stderr, "%scould not read battery energy max: '%s'\n",
				BAD_MSG, path);
	}
	else energy_full = strtol(energy_full_s, NULL, 10);
	fclose(f);

	//energy now
	char energy_now_s[32];
	long int energy_now = 0;
	sprintf(path, "%s/BAT%d/energy_now", PS_PATH, b);
	f = fopen(path, "r");
	if(f == NULL || fgets(energy_now_s, 32, f) == NULL) {
		fprintf(stderr, "%scould not read battery energy now: '%s'\n",
				BAD_MSG, path);
	}
	else energy_now = strtol(energy_now_s, NULL, 10);
	fclose(f);

	bi->percent = (int)(energy_now * 100 / (double)energy_full);
}
