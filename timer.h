#ifndef timer_h
#define timer_h

void print_report(time_t);
void print_status(char*);
void compile_timer_list(char**, int*);
void print_list(char**, int);
int dir_exists(char*);
int create_directory(char*);
int start_timer(char*);
int stop_timer(char*);
int is_running(char*);
int is_command(char*);
void print_usage(void);
void error(char*, ...);

#endif