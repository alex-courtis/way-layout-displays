#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>

#include "convert.h"
#include "ipc.h"
#include "process.h"
#include "sockets.h"

void execute(enum IpcCommand command, char *request) {
	int fd;

	if ((fd = create_socket_client()) == -1) {
		exit(1);
	}

	log_debug("========%s request=================\n%s", ipc_command_name(command), request);
	if (socket_write(fd, request, strlen(request)) == -1) {
		exit(1);
	}

	for (;;) {
		char *response = NULL;
		if (!(response = socket_read(fd))) { // yup, that's a memory leak
			exit(1);
		}
		log_debug("========%s response================\n%s", ipc_command_name(command), response);
		if (strstr(response, "DONE: TRUE")) {
			break;
		}

	}

	close(fd);
}

void get(void) {
	char *request = "\
OP: GET"
;

	execute(GET, request);
}

void cfg_set(void) {
	char *request = "\
OP: CFG_SET\n\
CFG: \n\
  ORDER:\n\
    - Monitor One\n\
    - Mon Two\n\
    - Three"
	;

	execute(CFG_SET, request);
}

void example_usage(void) {
	fprintf(stderr, "Usage: example_client <GET | CFG_SET>\n");
	exit(1);
}

int
main(int argc, char **argv) {
	log_set_threshold(DEBUG, true);

	if (argc != 2) {
		example_usage();
	}

	void (*fn)(void);
	if (strcmp(argv[1], ipc_command_name(GET)) == 0) {
		fn = get;
	} else if (strcmp(argv[1], ipc_command_name(CFG_SET)) == 0) {
		fn = cfg_set;
	} else {
		example_usage();
	}

	if (!getenv("WAYLAND_DISPLAY")) {
		log_error("environment variable $WAYLAND_DISPLAY missing");
		exit(1);
	}
	log_debug("WAYLAND_DISPLAY=%s", getenv("WAYLAND_DISPLAY") ? getenv("WAYLAND_DISPLAY") : "");
	log_debug("XDG_VTNR=%s", getenv("XDG_VTNR") ? getenv("XDG_VTNR") : "");

	char *path = pid_path();
	log_debug("Server PID file %s", path);
	free(path);

	pid_t pid = pid_active_server();
	if (pid == 0) {
		log_error("way-displays not running");
		exit(1);
	}
	log_debug("Server PID %d", pid);

	struct sockaddr_un addr;
	socket_path(&addr);
	log_debug("Server socket %s", addr.sun_path);

	fn();

	return(0);
}

