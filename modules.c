#include "lifebar.h"

// checks if the given modules does not contain invalid values.
// Returns 0 if everything is ok or 1 if an error occured while
// parsing the modules
void check_module_list(struct module *modules) {
	struct module *ptr;
	ptr = modules;
	while(ptr != NULL) {
		if(!valid_module_suffix(ptr->name, valid_suffix) 
										&& !valid_module_static(ptr->name, valid_static)) {
			fprintf(stderr, "%sbad value for config key 'modules' -> '%s'\n",
					BAD_MSG, ptr->name);
		} 
		ptr = ptr->next;
	}
}

// checks if the specified module is one of the static valid entries
// Returns 1 if the value is valid or 0 otherwise.
int valid_module_static(char *module, char **valid_entries) {
	size_t i;

	for(i = 0; valid_entries[i] != NULL; i++) {
		if(strcmp(module, valid_entries[i]) == 0)
			return 1;
	}
	return 0;
}

// Checks if the specified module is one of the valid suffix based values like
// bat0 or therm1. Returns 1 if the value is valid, 0 otherwise.
int valid_module_suffix(char *module, char **valid_entries) {
	size_t i;
	int suffix = 0;

	suffix = get_module_suffix(module, valid_entries);
	if(suffix >= 0) 
		return 1;
	return 0;
}

// Returns the module suffix. Example for 
// bat0 => 0
// therm1 => 1
//
// Returns the suffix as an integer or -1 if no suffix could be determined.
int get_module_suffix(char *module, char **valid_entries) {
	size_t i;
	int suffix = -1;

	for(i = 0; valid_entries[i] != NULL; i++) {
		char *prefix = valid_entries[i];
		if(strncmp(prefix, module, strlen(prefix)) == 0) {
			suffix = atoi(module+strlen(prefix));
			return suffix;
		}
	}
	return -1;
}
