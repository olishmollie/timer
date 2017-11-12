#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>

#include "timer.h"

const unsigned long MAXBUFSIZE = 500;
const int MAXTIMERS = 25;
char *command_list[MAXTIMERS];
char root[MAXBUFSIZE];

int main(int argc, char *argv[])
{
    if (argc <= 1) {
        print_usage();
        exit(1);
    }

    /* Create root directory */
    snprintf(root, MAXBUFSIZE, "%s/.timer", getenv("HOME"));
    if (!dir_exists(root)) {
        if (!create_directory(root)) {
            error("unable to create timer directory");
        }
    }

    int numtimers = 0;

    compile_timer_list(command_list, &numtimers);

    char *command, *tname = NULL;
    char dirname[MAXBUFSIZE];
    int c, nflag = 0, dflag = 0;

    while ((c = getopt(argc, argv, "n:d:")) != -1) {
       switch(c) {
           case 'n':
               nflag = 1;
               tname = optarg;
               if (is_command(tname)) {
                   error("invalid name '%s'", tname);
               }
               break;
           case 'd':
               dflag = 1;
               tname = optarg;
               break;
           case '?':
               exit(opterr);
           default:
               ;
       }
    }

    if (nflag) {
        /* Create '~/.timer/<name>' directory */
        snprintf(dirname, MAXBUFSIZE, "%s/%s", root, tname);
        if (!create_directory(dirname)) {
            error("unable to create timer '%s'", tname);
        }
    }

    if (dflag) {
        snprintf(dirname, MAXBUFSIZE, "%s/%s", root, tname);
        if (is_running(tname)) {
            print_status(tname);
            exit(1);
        }
        errno = 0;
        rmdir(dirname);
        if (errno == ENOENT) {
            error("cannot find timer '%s'", tname);
        }
        exit(0);
    }

    int success = 0;

    if (optind != argc) {

        command = argv[optind];

        if (strcmp(command, "start") == 0) {
            success = start_timer(tname);
        } else if (strcmp(command, "stop") == 0) {
            success = stop_timer(tname);
        } else if (strcmp(command, "status") == 0) {
            print_status(tname);
        } else if (strcmp(command, "list") == 0) {
            print_list(command_list, numtimers);
        } else {
            error("unknown command %s", command);
        }
    }

    return success;
}

void print_report(time_t start_time)
{
    char buf[9];

    time_t final = time(NULL) - start_time;
    double hrs = final / 3600.0;

    long s = final % 60;
    long m = final / 60;
    long h = final / 3600;

    char sec[3], min[3], hr[3];

    snprintf(sec, (size_t)3, (s < 10) ? "0%ld" : "%ld", s);
    snprintf(min, (size_t)3, (m < 10) ? "0%ld" : "%ld", m);
    snprintf(hr,  (size_t)3, (h < 10) ? "0%ld" : "%ld", h);

    snprintf(buf, (size_t)9, "%s:%s:%s", hr, min, sec);

    printf("\n");
    printf("time: \t%s\n", buf);
    printf("hours:\t%.2f\n", hrs);
}

void print_status(char *tname)
{
    int running = is_running(tname);
    char *name = tname ? tname : "root";
    printf("%s is %srunning\n", name, running ? "" : "not ");
}

void compile_timer_list(char **list, int *numtimers)
{
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(root)) != NULL) {
        int i = 0;
        while ((ent = readdir(dir)) != NULL) {
            if (i == MAXTIMERS) {
                error("maximum number of timers (%d) reached", MAXTIMERS);
            }
            if (strstr(ent->d_name, ".")) {
                continue;
            }
            list[i++] = ent->d_name;
        }
        *numtimers = i;
        closedir(dir);
    } else {
        error("unable to access timer directory");
    }
}

void print_list(char **list, int len)
{
    int status;
    /* Print status of anonymous timer */
    status = is_running(NULL);
    printf("root: %srunning\n", status ? "" : "not ");
    for (int i = 0; i < len; i++) {
        status = is_running(list[i]);
        printf("%s: %srunning\n", list[i], status ? "" : "not ");
    }
}

int start_timer(char *tname)
{
    char filename[MAXBUFSIZE];
    snprintf(filename, MAXBUFSIZE, "%s/%s/start.tm", root, tname ? tname : "");
    if (is_running(tname)) {
        print_status(tname);
        return 0;
    } else {
        FILE *f;
        if ((f = fopen(filename, "w")) != NULL) {
            time_t t = time(NULL);
            fprintf(f, "%lu\n", t);
            fclose(f);
            return 1;
        } else {
            error("unable to start timer '%s'", tname ? tname : "root");
        }
        return 0;
    }
}

int stop_timer(char *tname)
{
    char filename[MAXBUFSIZE];
    snprintf(filename, MAXBUFSIZE, "%s/%s/start.tm", root, tname ? tname : "");
    if (!is_running(tname)) {
        print_status(tname);
        return 0;
    } else {
        FILE *f;
        if ((f = fopen(filename, "r")) != NULL) {
            time_t t;
            fscanf(f, "%lu", &t);
            fclose(f);
            remove(filename);
            print_report(t);
            return 1;
        } else {
            error("unable to stop timer '%s'", tname ? tname : "root");
        }
        return 0;
    }
}

int is_running(char *tname)
{
    int result;
    char filename[MAXBUFSIZE];
    snprintf(filename, MAXBUFSIZE, "%s/%s/start.tm", root, tname ? tname : "");
    FILE *f = fopen(filename, "r");
    result = f ? 1 : 0;
    fclose(f);
    return result;
}

int dir_exists(char *dirname)
{
    DIR *d = opendir(dirname);
    if (d) {
        closedir(d);
        return 1;
    } else {
        return 0;
    }
}

int create_directory(char *dirname)
{
    if (!dir_exists(dirname)) {
        errno = 0;
        mkdir(dirname, 0755);
        if (errno == EACCES || errno == EMLINK ||
            errno == ENOSPC || errno == EROFS) {
            return 0;
        }
    }
    return 1;
}

int is_command(char *tname)
{
    const char *commands[] = { "start", "stop", "status", "list" };
    const size_t num_commands = sizeof(commands)/sizeof(commands[0]);

    for (int i = 0; i < num_commands; i++) {
        if (strcmp(tname, commands[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

void print_usage()
{
    printf("usage: timer [-n <name>] [start/stop/status]\n");
    printf("       timer -d <name>\n");
    printf("       timer list\n");
}

void error(char *fmt, ...)
{
    fprintf(stderr, "timer: ");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    printf("\n");
}
