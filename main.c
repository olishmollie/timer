#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

time_t start_timer(char*);
time_t stop_timer(char*);
int is_running(char*);

char *time_str(time_t);

void unknown_command(char*);
void print_usage(void);

const unsigned long MAXBUFSIZE = 500;

int main(int argc, char *argv[])
{
    if (argc <= 1) {
	print_usage();
	exit(1);
    }

    unsigned long start_time;
    char *command, *homedir, *dirname, *filename;

    command = argv[1];

    /* Create directory if doesn't exist */
    homedir = getenv("HOME");
    dirname = malloc(MAXBUFSIZE*sizeof(char));
    snprintf(dirname, MAXBUFSIZE, "%s/.timer", homedir);
    mkdir(dirname, 0777);
    free(dirname);

    filename = malloc(MAXBUFSIZE*sizeof(char));
    snprintf(filename, MAXBUFSIZE, "%s/start.tm", dirname);

    if (strcmp(command, "start") == 0) {
	start_time = start_timer(filename);
	printf("timer begun\n");
    } else if (strcmp(command, "stop") == 0) {
	start_time = stop_timer(filename);
	time_t ft = time(NULL) - start_time;
	printf("timer stopped\n");
	printf("time: %s\n", time_str(ft));
	printf("hours: %.2f\n", ft/3600.0);
    } else {
	unknown_command(command);
    }

    free(filename);

    return 0;
}

time_t start_timer(char *filename)
{
    if (!is_running(filename)) {
	FILE *f;
	f = fopen(filename, "w");
	if (!f) {
	    printf("fatal: unable to open file\n");
	    free(filename);
	    exit(1);
	}
	time_t t = time(NULL);
	fprintf(f, "%lu\n", t);
	fclose(f);
	return t;
    } else {
	printf("timer is running. ");
	printf("run 'timer stop' to stop timer\n");
	free(filename);
	exit(1);
    }
}

time_t stop_timer(char *filename)
{
    if (is_running(filename)) {
	FILE *f;
	f = fopen(filename, "r");
	if (!f) {
	    printf("fatal: unable to open file\n");
	    free(filename);
	    exit(1);
	}
	time_t t;
	fscanf(f, "%lu", &t);
	fclose(f);
	remove(filename);
	return t;
    } else {
	printf("timer never started. ");
	printf("run 'timer start' to start timer\n");
	free(filename);
	exit(1);
    }
}

int is_running(char *filename)
{
    int result;
    FILE *f = fopen(filename, "r");
    result = f ? 1 : 0;
    fclose(f);
    return result;
}

char *time_str(time_t t)
{
    char *buf = malloc(MAXBUFSIZE*sizeof(char));
    int sec = t % 60;
    int min = t / 60;
    int hr = t / 3600;
    snprintf(buf, (size_t)MAXBUFSIZE, "%d:%d:%d", hr, min, sec);
    return buf;
}

void unknown_command(char* arg)
{
    printf("unknown command %s", arg);
    print_usage();
}

void print_usage()
{
    printf("usage: timer [start/stop]\n");
}
