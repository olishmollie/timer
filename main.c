#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <getopt.h>

void print_report(time_t);
void print_status(void);
void list_timers(void);
int dir_exists(char*);
int create_directory(char*);
int start_timer(void);
int stop_timer(void);
int is_running(void);
int is_command(char*);
void unknown_command(char*);
void print_usage(void);

const unsigned long MAXBUFSIZE = 500;
char root[MAXBUFSIZE];
static char filename[MAXBUFSIZE];

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
            fprintf(stderr, "fatal: unable to create timer directory\n");
            exit(1);
        }
    }

    char *tname, *command;
    char dirname[MAXBUFSIZE];
    int c, nflag = 0, dflag = 0;

    while ((c = getopt(argc, argv, "n:d:")) != -1) {
       switch(c) {
           case 'n':
               nflag = 1;
               tname = optarg;
               if (is_command(tname)) {
                   fprintf(stderr, "fatal: timer name cannot be a command\n");
               }
               break;
           case 'd':
               dflag = 1;
               tname = optarg;
           default:
               ;
       }
    }

    if (nflag && dflag) {
        print_usage();
        exit(1);
    }

    if (nflag) {
        /* Create '~/.timer/<name>' directory */
        snprintf(dirname, MAXBUFSIZE, "%s/%s", root, tname);
        if (!create_directory(dirname)) {
            fprintf(stderr, "fatal: unable to create '%s' directory\n", dirname);
            exit(1);
        }
    }

    if (dflag) {
        snprintf(dirname, MAXBUFSIZE, "%s/%s", root, tname);
        if (is_running()) {
            print_status();
            exit(1);
        }
        errno = 0;
        rmdir(dirname);
        if (errno == ENOENT) {
            fprintf(stderr, "error: cannot find timer '%s'\n", tname);
            exit(1);
        }
        exit(0);
    }

    if (optind == argc) {
        print_usage();
        exit(1);
    }

    command = argv[optind];

    snprintf(filename, MAXBUFSIZE, "%s/start.tm", nflag ? dirname : root);

    int success = 0;
    if (strcmp(command, "start") == 0) {
        success = start_timer();
    } else if (strcmp(command, "stop") == 0) {
        success = stop_timer();
    } else if (strcmp(command, "status") == 0) {
        print_status();
    } else if (strcmp(command, "list") == 0) {
        list_timers();
    } else {
        unknown_command(command);
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

void print_status()
{
    int running = is_running();
    printf("timer is %srunning\n", running ? "" : "not ");
}

void list_timers()
{
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(root)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, "."))
                continue;
            printf("%s\n", ent->d_name);
        }
        closedir(dir);
    } else {
        fprintf(stderr, "Unable to open root dir\n");
        exit(1);
    }
}

int start_timer()
{
    if (!is_running()) {
        FILE *f;
        f = fopen(filename, "w");
        if (!f) {
            printf("fatal: unable to open %s\n", filename);
            return 0;
        }
        time_t t = time(NULL);
        fprintf(f, "%lu\n", t);
        fclose(f);
        return 1;
    } else {
        print_status();
        return 0;
    }
}

int stop_timer()
{
    if (is_running()) {
        FILE *f;
        f = fopen(filename, "r");
        if (!f) {
            printf("fatal: unable to stop timer\n");
            return 0;
        }
        time_t t;
        fscanf(f, "%lu", &t);
        fclose(f);
        remove(filename);
        print_report(t);
        return 1;
    } else {
        print_status();
        return 0;
    }
}

int is_running()
{
    int result;
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

void unknown_command(char* arg)
{
    printf("unknown command %s\n", arg);
    print_usage();
}

void print_usage()
{
    printf("usage: timer [-n <name>] start\n");
    printf("       timer [-n <name>] stop\n");
    printf("       timer -d <name>\n");
    printf("       timer list\n");
}
