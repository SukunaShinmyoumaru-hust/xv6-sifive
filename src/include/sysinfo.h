#ifndef __SYSINFO_H
#define __SYSINFO_H

#include "types.h"

#define BNUM 	2500


struct sysinfo {
	long uptime;             /* Seconds since boot */
	unsigned long loads[3];  /* 1, 5, and 15 minute load averages */
	unsigned long totalram;  /* Total usable main memory size */
	unsigned long freeram;   /* Available memory size */
	unsigned long sharedram; /* Amount of shared memory */
	unsigned long bufferram; /* Memory used by buffers */
	unsigned long totalswap; /* Total swap space size */
	unsigned long freeswap;  /* Swap space still available */
	unsigned short procs;    /* Number of current processes */
	unsigned long totalhigh; /* Total high memory size */
	unsigned long freehigh;  /* Available high memory size */
	unsigned int mem_unit;   /* Memory unit size in bytes */
	char _f[20-2*sizeof(long)-sizeof(int)];
							/* Padding to 64 bytes */
};

void logbufinit();

#define SYSLOG_ACTION_CLOSE (0)
#define SYSLOG_ACTION_OPEN (1)
#define SYSLOG_ACTION_READ (2)
#define SYSLOG_ACTION_READ_ALL (3)
#define SYSLOG_ACTION_READ_CLEAR (4)
#define SYSLOG_ACTION_CLEAR (5)
#define SYSLOG_ACTION_CONSOLE_OFF (6)
#define SYSLOG_ACTION_CONSOLE_ON (7)
#define SYSLOG_ACTION_CONSOLE_LEVEL (8)
#define SYSLOG_ACTION_SIZE_UNREAD (9)
#define SYSLOG_ACTION_SIZE_BUFFER (10)

#endif
