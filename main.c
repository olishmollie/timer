#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <errno.h>

void print_report(time_t);
void print_status(void);
void list_timers(void);
int dir_exists(char*);
int create_directory(char*);
int start_timer(void);
int stop_timer(void);
int is_running(void);
void unknown_command(char*);
void print_usage(void);

const unsigned long MAXBUFSIZE = 500;
char root[MAXBUFSIZE];
static char filename[MAXBUFSIZE];

const char *commands[] = { "start", "stop", "status", "list" };

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

    if (strcmp(argv[1], "list") == 0) {
        list_timers();
        exit(1);
    }

    char *name, *command;
    size_t num_commands = sizeof(commands)/sizeof(commands[0]);

    /* Make sure name isn't a valid command */
    name = argv[1];
    for (int i = 0; i < num_commands; i++) {
        if (strcmp(name, commands[i]) == 0) {
            fprintf(stderr, "fatal: name cannot be a valid command\n");
            exit(1);
        }
    }

    /* Check if command is valid */
    command = argv[2];
    int valid = 0;
    for (int i = 0; i < num_commands; i++) {
        if (strcmp(command, commands[i]) == 0) {
            valid = 1;
            break;
        }
    }
    if (!valid) {
        unknown_command(command);
        exit(1);
    }

    /* Create '~/.timer/<name>' directory */
    char dirname[MAXBUFSIZE];
    snprintf(dirname, MAXBUFSIZE, "%s/%s", root, name);
    if (!create_directory(dirname)) {
        fprintf(stderr, "fatal: unable to create '%s' directory\n", dirname);
        exit(1);
    }

    snprintf(filename, MAXBUFSIZE, "%s/start.tm", dirname);

    int success = 0;
    if (strcmp(command, "start") == 0) {
        success = start_timer();
    } else if (strcmp(command, "stop") == 0) {
        success = stop_timer();
    } else if (strcmp(command, "status") == 0) {
        print_status();
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

void unknown_command(char* arg)
{
    printf("unknown command %s\n", arg);
    print_usage();
}

void print_usage()
{
    printf("usage: timer <name> start\n");
    printf("       timer <name> stop\n");
}
