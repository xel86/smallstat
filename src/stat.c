#include "stat.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

extern int errno;

void get_stat_snapshot(struct snapshot *snap, const char *pid) {
    char path[21];
    snprintf(path, sizeof(path), "/proc/%s/stat", pid);

    FILE *f_pid_stat = fopen(path, "r");
    FILE *f_stat = fopen("/proc/stat", "r");
    if (f_pid_stat == NULL) {
        fprintf(stderr, "Could not open %s with error %s, exiting.", path,
                strerror(errno));
        exit(1);
    }

    if (f_stat == NULL) {
        fprintf(stderr, "Could not open /proc/stat with error %s, exiting.",
                strerror(errno));
        exit(1);
    }

    int matches =
        fscanf(f_pid_stat,
               "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu"
               "%lu %ld %ld %*d %*d %*d %*d %llu %lu %ld",
               &snap->utime, &snap->stime, &snap->cutime, &snap->cstime,
               &snap->starttime, &snap->vsize, &snap->rss);

    fclose(f_pid_stat);
    if (matches != 7) {
        fprintf(stderr, "Failed to parse all values from %s, exiting", path);
        exit(1);
    }

    /* Get page size in bytes and multiply by the number of process pages */
    snap->rss *= getpagesize();

    unsigned long cpu_time[10];
    matches = fscanf(f_stat, "%*s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                     &cpu_time[0], &cpu_time[1], &cpu_time[2], &cpu_time[3],
                     &cpu_time[4], &cpu_time[5], &cpu_time[6], &cpu_time[7],
                     &cpu_time[8], &cpu_time[9]);

    fclose(f_stat);
    if (matches != 10) {
        fprintf(stderr, "Failed to parse cpu time from /proc/stat, exiting");
        exit(1);
    }

    snap->cpu_time = 0;
    for (int i = 0; i < 10; i++) snap->cpu_time += cpu_time[i];
}
