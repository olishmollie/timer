#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <errno.h>

void print_report(time_t);
void print_status();
char *create_directory();
int start_timer(void);
int stop_timer(void);
int is_running(void);
void unknown_command(char*);
void print_usage(void);

const unsigned long MAXBUFSIZE = 500;
static char filename[MAXBUFSIZE];

int main(int argc, char *argv[])
{
    if (argc <= 1) {
	print_usage();
	exit(1);
    }

    unsigned long start_time;
    char *command;

    command = argv[1];

    char *dirname = create_directory();
    if (!dirname) {
	printf("fatal: unable to create '~/.timer' directory\n");
	exit(errno);
    }

    snprintf(filename, MAXBUFSIZE, "%s/start.tm", dirname);
    free(dirname);

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

    int s = final % 60;
    int m = final / 60;
    int h = final / 3600;

    char sec[3], min[3], hr[3];

    snprintf(sec, (size_t)3, (s < 10) ? "0%d" : "%d", s);
    snprintf(min, (size_t)3, (m < 10) ? "0%d" : "%d", m);
    snprintf(hr,  (size_t)3, (h < 10) ? "0%d" : "%d", h);

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

char *create_directory()
{
    char *homedir = getenv("HOME");
    char *dirname = malloc(MAXBUFSIZE*sizeof(char));
    snprintf(dirname, MAXBUFSIZE, "%s/.timer", homedir);

    errno = 0;
    mkdir(dirname, 0777);
    if (errno == EACCES || errno == EMLINK ||
	    errno == ENOSPC || errno == EROFS) {
	return NULL;
    }

    return dirname;
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

void unknown_command(char* arg)
{
    printf("unknown command %s", arg);
    print_usage();
}

void print_usage()
{
    printf("usage: timer [start/stop/status]\n");
}
