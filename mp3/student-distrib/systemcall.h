#ifndef _SYSTEMCALL_H
#define _SYSTEMCALL_H

#include "types.h"
#include "keyboard.h"
#include "file_sys.h"
#include "lib.h"
#include "rtc.h"

#define ARG_NUM 1024
#define FNAME_SIZE 32
#define BUFSIZE 4
#define FOUR_BYTE 4
#define PCB_MSK 0x007FE000

#define MAGIC_ONE       0x7F            // the four magic numbers
#define MAGIC_TWO       0x45
#define MAGIC_THREE     0x4C
#define MAGIC_FOUR      0x46
#define MAX_FILE_DESCRIPTOR 8
#define SIZE_OF_4MB     kernel
#define SIZE_OF_8MB     0x800000
#define SIZE_OF_8KB     0x2000
#define SIZE_OF_128MB   0x8000000
#define PROGRAM_IMAGE   0x8048000
#define VIRTUAL_START   32
#define KERNEL_MEM_END  0x800000 
#define WORD_SIZE       4
#define HALT_EXCEPTION  0x1F
#define KERNEL_STACK_OF 4
#define MAX_BUFFER_UPPER 127
#define SIZE_OF_NAME 31
#define ENTRY_OFFSET 24
#define VIDEO_ADDR SIZE_OF_128MB + 20 * SIZE_OF_4MB;

// #define NUM_
int32_t halt(uint8_t status);
int32_t getargs(uint8_t* buf, int32_t nbytes);
struct fop_table
{
    int32_t (*read)(int32_t fd, void* buf, int32_t nbytes);
    int32_t (*write)(int32_t fd, const void*buf, int32_t nbytes);
    int32_t (*open)(const uint8_t* filename);
    int32_t (*close)(int32_t fd);
};

struct fd_t
{
    fop_table* optable_ptr;
    int32_t inode;
    int32_t file_position;
    uint32_t flags;
};

struct pcb_t
{
    uint8_t args[ARG_NUM];
    fd_t file_array[MAX_FILE_DESCRIPTOR];
    int8_t pid;
    int8_t parent_pid;
    uint32_t parent_esp;
    uint32_t parent_ebp;
    uint32_t saved_esp;
    uint32_t saved_ebp;

};



/* function for file operation */
int32_t bad_call_open(const uint8_t* filename);
int32_t bad_call_close(int32_t fd);
int32_t open( const uint8_t* filename);
int32_t close(int32_t fd);
int32_t write(int32_t fd, const void* buf, int32_t nbytes);
int32_t read(int32_t fd, void* buf, int32_t nbytes);
int32_t Vidmap(uint8_t** screen_start);

void fop_init();
fop_table stdin_op;
fop_table stdout_op;
fop_table regular_op;
fop_table dir_op;
fop_table rtc_op;



#endif

