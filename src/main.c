#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "stat.h"

void get_comm_name(char *target, const char *pid) {
    char path[21];
    snprintf(path, sizeof(path), "/proc/%s/comm", pid);

    FILE *comm = fopen(path, "r");
    if (comm == NULL) {
        fprintf(stderr,
                "Could not open comm file %s, process with pid %s "
                "most likely doesn't exist.",
                path, pid);
        exit(1);
    }

    fscanf(comm, "%s", target);
}

int main(int argc, char **argv) {
    int ncores = sysconf(_SC_NPROCESSORS_ONLN);
    int update_interval = 1;

    char *pid;
    if (argc > 1)
        pid = argv[1];
    else {
        printf("Need to include pid as an argument");
        exit(1);
    }

    /* comm file cannot be longer than 16 bytes (including null terminator) */
    char comm[16];
    get_comm_name(comm, pid);

    printf("[*] %s (%s)\n", comm, pid);

    unsigned long long updates = 0;
    double peak = 0.0;
    double avg = 0.0;
    while (1) {
        struct snapshot old, new;
        unsigned long new_ticks, old_ticks;

        get_stat_snapshot(&old, pid);
        old_ticks = old.utime + old.stime + old.cutime + old.cstime;

        sleep(update_interval);

        get_stat_snapshot(&new, pid);
        new_ticks = new.utime + new.stime + new.cutime + new.cstime;

        double cpu_usage =
            ncores * 100 *
            ((new_ticks - old_ticks) / (double)(new.cpu_time - old.cpu_time));

        if (cpu_usage > peak) peak = cpu_usage;

        avg = (updates * avg + cpu_usage) / (updates + 1);
        updates++;

        printf("\r    CPU Usage: %.03lf Average: %.03lf Peak: %.03lf",
               cpu_usage, avg, peak);

        fflush(stdout);
    }
}
