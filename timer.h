#ifndef timer_h
#define timer_h

void print_report(time_t);
void print_status(char*);
void print_list();
int dir_exists(char*);
int create_directory(char*);
void create_timer(char*);
void delete_timer(char*);
int start_timer(char*);
int stop_timer(char*);
int is_running(char*);
int is_command(char*);
void print_usage(void);
void error(char*, ...);

#endif