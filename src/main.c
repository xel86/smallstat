#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "stat.h"

const char *units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
char *bytes_to_human_readable(char *str, double bytes) {
    int i = 0;
    while (bytes > 1000) {
        bytes /= 1000;
        i++;
    }

    sprintf(str, "%.*f%s", i, bytes, units[i]);
    return str;
}

int string_to_int(const char *str) {
    int value = 0;

    while (isdigit(*str)) {
        value *= 10;
        value += *str - '0';
        str++;
    }
    return value;
}

void print_help() {
    printf("\nUsage: smallstat [PID] [OPTION]...");
    printf("\nMonitor process cpu usage, peak, and average overtime.");
    printf("\nExample: smallstat 1234\n");
    printf("\nArguments:");
    printf("\n  -i [int]\tUpdate interval in seconds");
    printf("\n  -a      \tCPU usage percentage will be relative to all cores.");
    printf("\n");
}

void get_opts(int argc, char **argv, char **pid, int *interval, int *ncores) {
    opterr = 0;
    int c;
    while ((c = getopt(argc, argv, "hp:i:a")) != -1) {
        switch (c) {
            case 'h':
                print_help();
                exit(1);
            case 'i':
                *interval = string_to_int(optarg);
                break;
            case 'a':
                *ncores = 1;
                break;
            case '?':
                if (optopt == 'i')
                    fprintf(
                        stderr,
                        "Option -i requires a update interval in seconds. \n");
                else
                    fprintf(stderr, "Unknown option character -%c.\n", optopt);

                print_help();
                exit(1);
            default:
                print_help();
                exit(1);
        }
    }

    if (optind == argc) {
        fprintf(stderr,
                "Need to provide a PID to track (ex: smallstat 1234)\n");
        print_help();
        exit(1);
    }

    *pid = argv[optind];
}

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
    /* Default option values */
    int ncores = sysconf(_SC_NPROCESSORS_ONLN);
    int update_interval = 1;
    char *pid = NULL;

    get_opts(argc, argv, &pid, &update_interval, &ncores);

    if (pid == NULL) {
        fprintf(stderr, "Need to provide a PID to monitor. (smallstat 1234)\n");
        print_help();
        exit(1);
    }

    if (update_interval < 1) {
        fprintf(stderr,
                "Update interval (-i) must be an interger greater than or "
                "equal to 1");
        print_help();
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

        if (updates != 0) {
            /* Clears CPU usage line, moves up one, clears memory usage line
             * https://en.wikipedia.org/wiki/ANSI_escape_code */
            printf("\x1b[2K");
            printf("\x1b[1A");
            printf("\x1b[2K");
            printf("\r");
        }

        get_stat_snapshot(&new, pid);
        new_ticks = new.utime + new.stime + new.cutime + new.cstime;

        double cpu_usage =
            ncores * 100 *
            ((new_ticks - old_ticks) / (double)(new.cpu_time - old.cpu_time));

        if (cpu_usage > peak) peak = cpu_usage;

        avg = (updates * avg + cpu_usage) / (updates + 1);
        updates++;

        char mem[32];
        printf("    CPU Usage (%%): %.03lf Average: %.03lf Peak: %.03lf \n",
               cpu_usage, avg, peak);
        printf("    MEM Usage (RSS): %s",
               bytes_to_human_readable(mem, new.rss));

        fflush(stdout);
    }
}
