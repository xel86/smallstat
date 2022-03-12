#ifndef STAT_H
#define STAT_H

#include <sys/types.h>

struct snapshot {
    unsigned long utime;          /* user mode cpu ticks */
    unsigned long stime;          /* kernel mode cpu ticks */
    long cutime;                  /* children user mode cpu ticks */
    long cstime;                  /* children kernel mode cpu ticks */
    unsigned long long starttime; /* process start time after boot */
    unsigned long vsize;          /* virtual memory size in bytes */
    long rss;                     /* number of pages process has in memory */
    double cpu_time;              /* total cpu ticks since boot*/
};

/* Fill *snap with updated values from /proc/pid/stat and /proc/stat */
void get_stat_snapshot(struct snapshot *snap, const char *pid);

#endif
