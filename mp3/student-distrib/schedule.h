#ifndef _SCHEDULE_H
#define _SCHEDULE_H

#include "types.h"
#define CLOCK_RATE  1193180
#define SIZE_8MB     0x800000
#define SIZE_8KB     0x2000



void pit_init();
void scheduler();
void get_next_terid();
void pit_interrupt_handler();

int32_t next_terminal; // indicate the id of the next terminal to run
int32_t ter_scheduling; // terminal that currently running

int32_t pressed_terminal;  // indicate which terminal need to switch to
int32_t launch_ter;        // flag of needing to launch new terminal
int32_t first_boot;        // flag indicates whether there is no terminal
int32_t ter_switch;        // flag indicates terminal switching
int32_t old_terminal;       // store the terminal number before switch
#endif

