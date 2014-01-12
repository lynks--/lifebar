#include "lifebar.h"

#define PS_PATH "/sys/class/power_supply"
#define TH_PATH "/sys/class/thermal"
#define NET_PATH "/sys/class/net"

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

int count_acpi_thermal() {
	DIR *d;
	struct dirent *de;
	int count = 0;

	d = opendir(TH_PATH);
	if(d == NULL) return 0;
	while((de = readdir(d)) != NULL) {
		if(strstr(de->d_name, "thermal") == de->d_name) {
			//name starts with thermal
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

void read_acpi_thermal(int t, struct thermal_info *therm) {
	//save the index 
	therm->index = t;

	//we assume the thermal index to exist as filename thermal_zone<index>
	char path[128];
	FILE *f;

	//temp
	char temp_s[32];
	long int temp = 0;
	sprintf(path, "%s/thermal_zone%d/temp", TH_PATH, t);
	f = fopen(path, "r");
	if(f == NULL || fgets(temp_s, 32, f) == NULL) {
		fprintf(stderr, "%scould not read thermal status: '%s'\n",
				BAD_MSG, path);
	}
	else temp = strtol(temp_s, NULL, 10);
	fclose(f);

	therm->temp_c = temp / 1000;
}

void read_net_speed(char *ifname, struct net_speed_info *net) {
	char path[128];
	FILE *f;

	//download
	char rxb_s[32];
	sprintf(path, "%s/%s/statistics/rx_bytes", NET_PATH, ifname);
	f = fopen(path, "r");
	if(f == NULL || fgets(rxb_s, 32, f) == NULL) {
		fprintf(stderr, "%scould not read interface speed: '%s'\n",
				BAD_MSG, path);
	}
	else net->down_bytes = strtol(rxb_s, NULL, 10);
	fclose(f);

	//upload
	char txb_s[32];
	sprintf(path, "%s/%s/statistics/tx_bytes", NET_PATH, ifname);
	f = fopen(path, "r");
	if(f == NULL || fgets(txb_s, 32, f) == NULL) {
		fprintf(stderr, "%scould not read interface speed: '%s'\n",
				BAD_MSG, path);
	}
	else net->up_bytes = strtol(txb_s, NULL, 10);
	fclose(f);
}
