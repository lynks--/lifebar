#include "lifebar.h"

int is_output_key_label(char *c) {
	if(strcmp(c, "name") == 0) return 1;
	if(strcmp(c, "active") == 0) return 1;
	if(strcmp(c, "x") == 0) return 1;
	if(strcmp(c, "y") == 0) return 1;
	if(strcmp(c, "width") == 0) return 1;
	if(strcmp(c, "height") == 0) return 1;
	return 0;
}

void handle_output_value_label(struct i3_output *o, char *key, char *value) {
	if(strlen(key) > 0) {
		if(strcmp(key, "name") == 0) strcpy(o->name, value);
		else if(strcmp(key, "active") == 0) strcpy(o->active, value);
		else if(strcmp(key, "x") == 0) o->x = atoi(value);
		else if(strcmp(key, "y") == 0) o->y = atoi(value);
		else if(strcmp(key, "width") == 0) o->width = atoi(value);
		else if(strcmp(key, "height") == 0) o->height = atoi(value);
	}
}

int is_workspace_key_label(char *c) {
	if(strcmp(c, "name") == 0) return 1;
	if(strcmp(c, "visible") == 0) return 1;
	if(strcmp(c, "focused") == 0) return 1;
	if(strcmp(c, "urgent") == 0) return 1;
	if(strcmp(c, "output") == 0) return 1;
	return 0;
}

void handle_workspace_value_label(struct i3_workspace *o,
								  char *key, char *value) {
	if(strlen(key) > 0) {
		if(strcmp(key, "name") == 0) strcpy(o->name, value);
		else if(strcmp(key, "visible") == 0) strcpy(o->visible, value);
		else if(strcmp(key, "focused") == 0) strcpy(o->focused, value);
		else if(strcmp(key, "urgent") == 0) strcpy(o->urgent, value);
		else if(strcmp(key, "output") == 0) strcpy(o->output, value);
	}
}

void debug_i3_output(struct i3_output *head) {
	while(head != NULL) {
		printf("=========\nname=%s\nactive=%s\nx=%d\ny=%d\nwidth=%d\nheight=\
%d\n",
				head->name, head->active, head->x, head->y, head->width,
				head->height);
		head = head->next;
	}
}

void debug_i3_workspace(struct i3_workspace *head) {
	while(head != NULL) {
		printf("=========\nname=%s\nvisible=%s\noutput=%s\n", head->name,
			   head->visible, head->output);
		head = head->next;
	}
}

void i3_ipc_send(char **ret, int i3_sock, int type, char *payload) {
	//we pack the buffer and send it
	int i;
	int plen = strlen(payload);
	char buf[plen + 14];
	strcpy(buf, "i3-ipc");
	*((int *)(buf +  6)) = plen;
	*((int *)(buf + 10)) = type;
	strcpy(buf + 14, payload);
	write(i3_sock, buf, plen + 14);

	//and read the response
	char *recv_buf = malloc(4096);
	ssize_t t = recv(i3_sock, recv_buf, 4096, 0);
	if(t == -1) {
		perror("recv");
		recv_buf[0] = '\0';
	}
	else recv_buf[t] = '\0';
	*ret = recv_buf + 14; //skip over the header
}

void free_ipc_result(char *c) {
	//this function exists because we have to free the actual malloced
	//result, which included the 14 byte header we skipped over
	free(c - 14);
}

struct i3_output *get_i3_outputs(int i3_sock) {
	//query i3 for the current outputs and build a linked list
	//note: this function returns a list in reverse order to the json array
	struct i3_output *head = NULL;

	char *output_json;
	i3_ipc_send(&output_json, i3_sock, GET_OUTPUTS, "");

	char key[64];
	char label[64];
	char *labelptr = label;

	int inside = 0;
	int i;
	for(i = 0; i < strlen(output_json); i++) {
		char c = *(output_json + i);
		char d = *(output_json + i + 1);
		if(c == '\"') {
			if(inside == 0) inside = 1;
			else {
				inside = 0;
				*labelptr = '\0';
				if(strcmp(label, "name") == 0) {
					//we are entering a new output block
					//we create a new struct pointing to the current head
					struct i3_output *o = malloc(sizeof *o);
					o->next = head;
					head = o;
				}

				if(is_output_key_label(label)) strcpy(key, label);
				else {
					handle_output_value_label(head, key, label);
					key[0] = '\0';
				}

				labelptr = label; //reset the label pointer
			}
		}
		else if(c == ':' && d != '\"' && d != '{')
			inside = 2; //inside a non-"-delimited value
		else if((c == ',' || c == '}') && inside == 2) {
			inside = 0;
			*labelptr = '\0';
			handle_output_value_label(head, key, label);
			key[0] = '\0';
			labelptr = label;
		}
		else {
			if(inside != 0) {
				*labelptr = c;
				labelptr++;
			}
		}
	}
	free_ipc_result(output_json);
	return head;
}

struct i3_workspace *get_i3_workspaces(int i3_sock) {
	struct i3_workspace *head = NULL;
	struct i3_workspace *tail = NULL;

	char *workspace_json;
	i3_ipc_send(&workspace_json, i3_sock, GET_WORKSPACES, "");

	char key[64];
	char label[64];
	char *labelptr = label;

	int inside = 0;
	int i;
	for(i = 0; i < strlen(workspace_json); i++) {
		char c = *(workspace_json + i);
		char d = *(workspace_json + i + 1);
		if(c == '\"') {
			if(inside == 0) inside = 1;
			else {
				inside = 0;
				*labelptr = '\0';
				if(strcmp(label, "name") == 0) {
					//we are entering a new workspace block
					//we create a new struct pointing to the current head
					struct i3_workspace *o = malloc(sizeof *o);
					if(tail != NULL) tail->next = o;
					else head = o;
					o->next = NULL;
					tail = o;
				}

				if(is_workspace_key_label(label)) strcpy(key, label);
				else {
					handle_workspace_value_label(tail, key, label);
					key[0] = '\0';
				}

				labelptr = label; //reset the label pointer
			}
		}
		else if(c == ':' && d != '\"' && d != '{')
			inside = 2; //inside a non-"-delimited value
		else if((c == ',' || c == '}') && inside == 2) {
			inside = 0;
			*labelptr = '\0';
			handle_workspace_value_label(tail, key, label);
			key[0] = '\0';
			labelptr = label;
		}
		else {
			if(inside != 0) {
				*labelptr = c;
				labelptr++;
			}
		}
	}
	free_ipc_result(workspace_json);
	return head;
}

void get_i3_sockpath(char **ret) {
	int pid, err;
	int pipefd[2];
	FILE *output;
	char buf[512];

	//set up the pipe
	if(pipe(pipefd) < 0) {
		perror("pipe");
		exit(1);
	}

	if((pid = fork()) == 0) {
		//we are in the child

		//duplicate writing end of pipe to child stdout
		dup2(pipefd[1], STDOUT_FILENO);

		//close both of my ends of the pipe?
		close(pipefd[0]);
		close(pipefd[1]);

		execl("/usr/bin/i3", "i3", "--get-socketpath", NULL);
		perror("execl");
	}

	//we are in the parent
	
	//close writing end of pipe
	close(pipefd[1]);

	//open the reading end of the pipe
	output = fdopen(pipefd[0], "r");

	fgets(buf, sizeof buf, output);
	int len = strlen(buf);
	//remove the trailing newline
	buf[len - 1] = '\0';
	*ret = malloc(len);
	strcpy(*ret, buf);
}
