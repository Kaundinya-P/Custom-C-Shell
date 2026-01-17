#ifndef SIGNALS_H
#define SIGNALS_H

#include <sys/types.h>

void setup_signal_handlers();

void set_foreground_process_group(pid_t pgid);

void clear_foreground_process_group();

void handle_eof();

#endif
