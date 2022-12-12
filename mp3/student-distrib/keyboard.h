#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "types.h"

#define KEYBOARD_IRQ    1
#define KEYBOARD_PORT   0x60
#define KEYBOARD_CMD   0x64

#define BUFFERSIZE 128
#define TERMINAL_SIZE 1000

#define TAB     0x0F
#define CAPS    0x3A
#define LSHIFT_PRESS    0x2A
#define LSHIFT_RELEASE  0xAA
#define RSHIFT_PRESS    0x36
#define RSHIFT_RELEASE  0xB6
#define CTRL_PRESS      0x1D
#define CTRL_RELEASE    0x9D
#define BACKSPACE       0x0E
#define ENTER           0x1C
#define MAX_TERMINAL    3
#define MAX_PROC    6


#define LALT_PRESS  0x38
#define LALT_RELEASE 0xB8
// #define RALT_PRESS  
// #define RALT_RELEASE

#define F1_PRESS 0x3B
#define F2_PRESS 0x3C
#define F3_PRESS 0x3D



char keyboard_buffer[MAX_TERMINAL][BUFFERSIZE];
char terminal_buffer[MAX_TERMINAL][TERMINAL_SIZE];


void special_button_status(unsigned int key);

void keyboard_init();

void keyboard_interrupt_handler();

void print_stuff(int value, int indexP);

int terminal_open(const uint8_t* filename);
int terminal_close(int32_t fd);
int terminal_read(int32_t fd, void* buf, int32_t nbytes);
int terminal_write(int32_t fd, const void* buf, int32_t nbytes);

struct terminal_status
{
    uint32_t terminal_pid_table[MAX_TERMINAL][MAX_PROC];
    int32_t current_terminal;
    int32_t t_count;
    uint32_t terminal_process_count[MAX_TERMINAL];
    
};

terminal_status_t tot_terminal; // a structure that stores all the information needed for the three terminals
int current_terminal;           // indicate which terminal should be on screen (or to switch to)

void terminal_table_init();
int32_t execute(const uint8_t* command);


#endif

