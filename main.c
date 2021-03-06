#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>

#include "timer.h"

#ifdef _WIN32
# define  mkdir( D, M )  _mkdir( D )
#endif

#define MAXBUFSIZE 500
#define MAXTIMERS 25

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

    create_timer("root");

    char *command, *tname = NULL;

    command = argv[1];

    // TODO: variadic args
    if (argc >= 3)
        tname = argv[2];

    int success = 0;

    if (strcmp(command, "start") == 0) {
        success = start_timer(tname);
    } else if (strcmp(command, "stop") == 0) {
        success = stop_timer(tname);
    } else if (strcmp(command, "status") == 0) {
        print_status(tname);
    } else if (strcmp(command, "list") == 0) {
        print_list();
    } else if (strcmp(command, "create") == 0) {
        create_timer(tname);
    } else if (strcmp(command, "delete") == 0) {
        delete_timer(tname);
    } else {
        error("unknown command %s", command);
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
    if (tname == NULL) tname = "root";
    int running = is_running(tname);
    printf("%s is %srunning\n", tname, running ? "" : "not ");
}

void print_list()
{
    DIR *dir;
    struct dirent *ent;
    int status;
    if ((dir = opendir(root)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, ".")) {
                continue;
            }
            status = is_running(ent->d_name);
            printf("%s: %srunning\n", ent->d_name, status ? "" : "not ");
        }
        closedir(dir);
    } else {
        error("unable to access timer directory");
    }
}

void create_timer(char *tname)
{
    if (tname == NULL)
	error("'create' takes argument <name>");
    char dirname[MAXBUFSIZE];
    snprintf(dirname, MAXBUFSIZE, "%s/%s", root, tname);
    if (!create_directory(dirname))
        error("unable to create timer '%s'", tname);
}

void delete_timer(char *tname)
{
    if (tname == NULL)
	error("'delete' takes argument <name>");
    char dirname[MAXBUFSIZE];
    snprintf(dirname, MAXBUFSIZE, "%s/%s", root, tname);
    if (is_running(tname))
	error("%s is running", tname);
    errno = 0;
    rmdir(dirname);
    if (errno == ENOENT)
        error("cannot find timer '%s'", tname);
}

int start_timer(char *tname)
{
    if (tname == NULL) tname = "root";
    if (is_running(tname)) {
	error("%s is running", tname);
	return 1;
    } else {
        FILE *f;
	char filename[MAXBUFSIZE];
	snprintf(filename, MAXBUFSIZE, "%s/%s/start.tm", root, tname);
        if ((f = fopen(filename, "w")) != NULL) {
            time_t t = time(NULL);
            fprintf(f, "%lu\n", t);
            fclose(f);
            return 1;
        } else {
            error("unable to start timer '%s'", tname);
        }
        return 0;
    }
}

int stop_timer(char *tname)
{
    if (tname == NULL) tname = "root";
    if (!is_running(tname)) {
	error("%s is not running", tname);
        return 1;
    } else {
        FILE *f;
	char filename[MAXBUFSIZE];
	snprintf(filename, MAXBUFSIZE, "%s/%s/start.tm", root, tname ? tname : "");
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
    if (tname == NULL) tname = "root";
    int result;
    char filename[MAXBUFSIZE];
    snprintf(filename, MAXBUFSIZE, "%s/%s/start.tm", root, tname);
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
    printf("usage: timer <command> [<name>]\n");
}

void error(char *fmt, ...)
{
    fprintf(stderr, "timer: ");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    printf("\n");
    exit(1);
}
