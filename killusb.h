/* Executable Name */
#define EXEC "killusb"

/* Program Version */
#define VERSION "1.0.1"

/* -DEVICES Macro (Maximum USB Devices) */
#ifndef EVICES
#define EVICES 256
#endif

/* Maximum Device IDs To Read */
#define DEVICES_IDS EVICES * 2

/* -DELAY Macro (Polling Delay Time In Seconds) */
#ifndef ELAY
#define ELAY 1
#endif

/* -DLOG Macro (Log File Path) */
#ifndef LOG
#define LOG "/var/log/" EXEC ".log"
#endif

/* Introduce Boolean Type */
typedef enum {
	FALSE,
	TRUE
} boolean;

/* Argument Flags */
#define FLAG_HELP      "--help"
#define FLAG_VERBOSE   "--verbose"
#define FLAG_FORCE     "--force"
#define FLAG_DELAY     "--delay"
#define FLAG_SCRIPT    "--script"
#define FLAG_WHITELIST "--whitelist"
#define FLAG_LOG       "--log"

/* Argument Macros */
#define ARGUMENT_CLEANUP() {\
	rc = 1;\
	goto cleanup;\
}

#define ARGUMENT_CONTINUE() {\
	i += 1;\
	continue;\
}

#define ARGUMENT_CHECK() {\
	if (i + 1 >= argc) {\
		usage();\
		ARGUMENT_CLEANUP();\
	}\
}

#define ARGUMENT_FLAGS(FLAG, CONTENT) {\
	if (strcmp(action, FLAG) == 0 || strncmp(action, FLAG + 1, 2) == 0) {\
		CONTENT\
	}\
}

#define ARGUMENT_HELP(FLAG) {\
	ARGUMENT_FLAGS(FLAG, {\
		usage();\
		ARGUMENT_CLEANUP();\
	})\
}

#define ARGUMENT_BOOLEAN(FLAG, VARIABLE) {\
	ARGUMENT_FLAGS(FLAG, {\
		VARIABLE = TRUE;\
		continue;\
	})\
}

#define ARGUMENT_STRING(FLAG, VARIABLE) {\
	ARGUMENT_FLAGS(FLAG, {\
		ARGUMENT_CHECK()\
		VARIABLE = argv[i + 1];\
		ARGUMENT_CONTINUE();\
	})\
}

#define ARGUMENT_INT(FLAG, VARIABLE) {\
	ARGUMENT_FLAGS(FLAG, {\
		ARGUMENT_CHECK()\
		VARIABLE = atoi(argv[i + 1]);\
		ARGUMENT_CONTINUE();\
	})\
}

/* External reboot() Function */
int reboot(int);

/* Struct To Hold Counts */
struct count {
	int old, new;
};

/* Struct To Hold Device Data */
struct device {
	char device_ids_old[DEVICES_IDS];
	char device_ids_new[DEVICES_IDS];
};

/* Started Flag */
static boolean started = FALSE;

/* Verbose Flag */
static boolean verbose = FALSE;

/* Force Flag */
static boolean force = FALSE;

/* Delay Value */
static int delay = ELAY;

/* Script Filename */
static char *script = NULL;

/* Whitelist Value */
static char *whitelist = NULL;

/* Log Value */
static char *log = LOG;

/* Executable Name */
static char *exec = EXEC;