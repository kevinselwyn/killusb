#include <stdio.h>
#include <string.h>
#include <usb.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <linux/reboot.h>
#include "killusb.h"

/* Verbose printf() Wrapper */
static void printv(const char *format, ...) {
	if (!verbose) {
		return;
	}

	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

/* Logging printf() Wrapper */
static void printl(const char *format, ...) {
	time_t t = time(NULL);
	char message[256], *date = NULL;
	FILE *log_file = NULL;

	va_list args;
	va_start(args, format);

	vsprintf(message, format, args);

	log_file = fopen(log, "a");

	if (!log_file) {
		goto cleanup;
	}

	date = ctime(&t);
	date[24] = '\0';

	fprintf(log_file, "%s %s", date, message);

cleanup:
	if (log_file) {
		fclose(log_file);
	}

	va_end(args);
}

/* Check For Root Access */
static boolean is_root() {
	uid_t uid = getuid(), euid = geteuid();

	if (uid != 0 || uid != euid) {
		return FALSE;
	}

	return TRUE;
}

/* Shutdown */
static void shutdown() {
	reboot(LINUX_REBOOT_CMD_POWER_OFF);
}

/* Execute Trigger */
static void execute() {
	FILE *script_file = NULL;

	if (script == NULL) {
		shutdown();
	} else {
		printv("Executing %s...\n", script);
		script_file = popen(script, "r");

		if (pclose(script_file) != 0) {
			shutdown();
		}
	}
}

/* Crtl-C Handler */
static void sig_handler(int signo) {
	printv("\n");
	printl("Killing %s...\n", exec);

	if (force == TRUE) {
		execute();
	}

	exit(signo);
}

/* Get USB Device Name */
static int get_name(char **name, usb_dev_handle *hnd, struct usb_device *dev) {
	int rc = 0, i = 0, l = 0;
	int langid = 0, index = 0, type = 0, request = 0, size = 0;
	char buffer[256], output[256];

	size = 256;
	langid = 0x0409;
	index = dev->descriptor.iProduct + (USB_DT_STRING << 8);
	type = USB_TYPE_STANDARD | USB_RECIP_DEVICE | USB_ENDPOINT_IN;
	request = USB_REQ_GET_DESCRIPTOR;

	rc = usb_control_msg(hnd, type, request, index, langid, buffer, size, 1000);

	if (rc < 0) {
		return rc;
	}

	if ((unsigned char)buffer[0] < rc) {
		rc = (unsigned char)buffer[0];
	}

	if (buffer[1] != USB_DT_STRING) {
		return 0;
	}

	rc /= 2;

	for (i = 1, l = rc / 2; i < l && i < size; i++) {
		output[i - 1] = (buffer[2 * i + 1] == 0) ? buffer[2 * i] : '?';
	}

	output[i - 1] = '\0';

	strcpy(*name, output);

	return i - 1;
}

/* Whitelist Test */
static boolean is_whitelisted(char *name) {
	int i = 0, l = 0;
	size_t name_length = 0, whitelist_length = 0;

	name_length = strlen(name);
	whitelist_length = strlen(whitelist);

	for (i = 0, l = (int)whitelist_length; i < l; i += 1) {
		if (i == 0 || whitelist[i] == ',') {
			if (strncmp(name, whitelist + i + 1, name_length) == 0) {
				return TRUE;
			}
		}
	}

	return FALSE;
}

/* Listen For USB Device Changes */
static int listen() {
	int rc = 0, i = 0, l = 0;
	int total = 0;
	char *product = NULL;
	char old_var, new_var;
	boolean test = FALSE;
	struct count bus_count = { 0, 0 };
	struct count dev_count = { 0, 0 };
	struct device devices;
	struct usb_bus *bus;
	struct usb_device *dev;
	usb_dev_handle *handle = NULL;

	printv("Initializing USB...\n");
	usb_init();

	product = (char *)malloc(sizeof(char) * 256);

	while (bus_count.old == bus_count.new && dev_count.old == dev_count.new) {
		if (started == TRUE) {
			printv("Waiting %d second%s..\n", delay, delay == 1 ? "." : "s.");

			if (verbose == TRUE) {
				for (i = 0, l = delay; i < l; i++) {
					printv("%d...\n", delay - i);
					sleep(1);
				}
			} else {
				sleep(delay);
			}
		}

		printv("Finding USB devices...\n");
		usb_find_busses();
		usb_find_devices();

		bus_count.new = 0;
		dev_count.new = 0;
		total = 0;

		for (bus = usb_busses; bus; bus = bus->next) {
			bus_count.new++;

			for (dev = bus->devices; dev; dev = dev->next) {
				if (whitelist != NULL) {
					if (!(handle = usb_open(dev))) {
						continue;
					}

					if (get_name(&product, handle, dev) < 0) {
						continue;
					}

					if (is_whitelisted(product)) {
						continue;
					}
				}

				dev_count.new++;

				if (dev_count.new > EVICES) {
					break;
				}

				devices.device_ids_new[total] = dev->descriptor.idVendor;
				devices.device_ids_new[total + 1] = dev->descriptor.idProduct;

				if (started == TRUE) {
					new_var = devices.device_ids_new[total];
					old_var = devices.device_ids_old[total];

					if (new_var != old_var) {
						rc = 1;
						goto trigger;
					}

					new_var = devices.device_ids_new[total + 1];
					old_var = devices.device_ids_old[total + 1];

					if (new_var != old_var) {
						rc = 1;
						goto trigger;
					}
				}

				total += 2;
			}
		}

		printv("Found %d USB devices...\n", dev_count.new);

		if (started == FALSE) {
			bus_count.old = bus_count.new;
			dev_count.old = dev_count.new;

			memcpy(devices.device_ids_old, devices.device_ids_new, DEVICES_IDS);

			started = TRUE;
		}
	}

	if (bus_count.old != bus_count.new) {
		test = bus_count.old > bus_count.new ? TRUE : FALSE;
		printv("USB bus %s...\n", test == TRUE ? "removed" : "added");
	}

	if (dev_count.old != dev_count.new) {
		test = dev_count.old > dev_count.new ? TRUE : FALSE;
		printv("USB device %s...\n", test == TRUE ? "removed" : "added");
	}

trigger:
	printl("%s triggered...\n", exec);

	if (product) {
		free(product);
	}

	return rc;
}

/* Print Program Usage */
static void usage() {
	printf(" _    _ _ _           _     \n");
	printf("| |  (_) | |         | |    \n");
	printf("| | ___| | |_   _ ___| |__  \n");
	printf("| |/ / | | | | | / __| '_ \\ \n");
	printf("|   <| | | | |_| \\__ \\ |_) |\n");
	printf("|_|\\_\\_|_|_|\\__,_|___/_.__/ \n");
	printf("             Version %s\n\n", VERSION);
	printf("Usage: %s [options]\n\n", exec);
	printf("Options:\n");
	printf(" -h, --help                  Show help\n");
	printf(" -v, --verbose               Log events to screen\n");
	printf(" -f, --force                 Trigger execution on Ctrl-C\n");
	printf(" -d, --delay <seconds>       Polling delay in seconds\n");
	printf(" -s, --script <path>         Script to run on USB change\n");
	printf(" -w, --whitelist <name,...>  Comma separated USB device names\n");
	printf(" -l, --log <path>            Log file path\n");
}

/* Main Function */
int main(int argc, char *argv[]) {
	int rc = 0, i = 0;
	char *action = NULL;

	exec = argv[0];

	if (is_root() == FALSE) {
		printf("%s must be run as root\n", exec);

		rc = 1;
		goto cleanup;
	}

	if (argc > 1) {
		for (i = 1; i < argc; i++) {
			action = argv[i];

			ARGUMENT_HELP(FLAG_HELP);
			ARGUMENT_BOOLEAN(FLAG_VERBOSE, verbose);
			ARGUMENT_BOOLEAN(FLAG_FORCE, force);
			ARGUMENT_INT(FLAG_DELAY, delay);
			ARGUMENT_STRING(FLAG_SCRIPT, script);
			ARGUMENT_STRING(FLAG_WHITELIST, whitelist);
			ARGUMENT_STRING(FLAG_LOG, log);
		}
	}

	signal(SIGINT, sig_handler);

	printl("%s starting...\n", exec);
	printv("Setting delay to %d second%s..\n", delay, delay == 1 ? "." : "s.");

	if (script != NULL) {
		printl("Setting script to %s...\n", script);
	} else {
		printl("Preparing to shutdown on trigger...\n");
	}

	if (force == TRUE) {
		printv("Preparing to force execution on kill...\n");
	}

	rc = listen();

	execute();

cleanup:
	return rc;
}