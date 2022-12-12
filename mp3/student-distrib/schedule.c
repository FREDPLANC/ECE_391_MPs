#include "schedule.h"
#include "lib.h"
#include "i8259.h"
#include "systemcall.h"
#include "keyboard.h"
#include "page.h"

/*
 *  pit_init()
 *  DESCRIPTION: Do all the initialization work for the pit and scheduler()
 *               
 *  INPUTS: NONE             
 *  OUTPUTS: NONE
 *  SIDEEFFECT: NONE
 *  RETURN VALUE: NONE
 */
void pit_init()
{
    //http://www.osdever.net/bkerndev/Docs/pit.htm
    // T = 0.05s
    int i, j;

    int frequency = 100;
    ter_scheduling = 0;
    int T = CLOCK_RATE / frequency;
    pressed_terminal = 0; 
    launch_ter = 0;
    first_boot = 0;
    current_terminal =0;
    ter_switch = 0;
    old_terminal = 0;
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 2; j++)
        {
            cursor_pos[i][j] = 0;
        }
        screen_xter[i] = 0;
        screen_yter[i] = 0;
    }
    screen_x = 0;
    screen_y = 0;
    

    outb(0x36,0x43);
    outb((T&0xFF),0x40);        //set hight byte
    outb((T>>8),0x40);          //set low byte
    enable_irq(0);

}

/*
 *  pit_interrupt_handler()
 *  DESCRIPTION: handle the pit interrupt
 *               
 *  INPUTS: NONE             
 *  OUTPUTS: NONE
 *  SIDEEFFECT: NONE
 *  RETURN VALUE: NONE
 */
void pit_interrupt_handler(){
    send_eoi(0);
    cli();
    scheduler();
    sti();
    return;

}

/*
 *  get_next_terid():
 *  DESCRIPTION: Obtain the next terminal number to schedule 
 *               
 *  INPUTS: NONE             
 *  OUTPUTS: NONE
 *  SIDEEFFECT: NONE
 *  RETURN VALUE: NONE
 */
void get_next_terid(){
    
    next_terminal = (ter_scheduling+1)%MAX_TERMINAL;
    while(tot_terminal.terminal_process_count[next_terminal] == 0){
        if(next_terminal == ter_scheduling){
            break;
        }
        next_terminal = (next_terminal+1)%MAX_TERMINAL;
    }
    
}

/*
 *  scheduler():
 *  DESCRIPTION: do the scheduler work, it won't be interrupted
 *               
 *  INPUTS: NONE             
 *  OUTPUTS: NONE
 *  SIDEEFFECT: remap the video address
 *  RETURN VALUE: NONE
 */
void scheduler()
{
    
    uint8_t* start_screen = (uint8_t*)VIDEO_ADDR; // the start VM addr of the video memory for fish
    uint32_t physical_addr = VIDEO_ADDR;  // PM addr for remap function
    
    // if there is no process/terminal, return.
    if(first_boot == 0){
        if(tot_terminal.terminal_process_count[0]!= 0) {
            first_boot = 1;
        }
        return;
    } 
    
    // obtain the next terminal number to schedule 
    get_next_terid();
    // decide whether there is a switch and need to lauch a new terminal
    if(tot_terminal.terminal_process_count[current_terminal] == 0 && ter_switch == 1){
       // obtain the current pcb's stack pointers, save it for later use
        int32_t cur_pid = tot_terminal.terminal_pid_table[ter_scheduling][tot_terminal.terminal_process_count[ter_scheduling] - 1];
        pcb_t *cur_pcb = (pcb_t*)(SIZE_8MB - SIZE_8KB*(cur_pid+1));
        asm volatile(
        "movl %%esp, %%eax;"
        "movl %%ebp, %%ebx;"
        :"=a"(cur_pcb->saved_esp),"=b"(cur_pcb->saved_ebp)
        :
        );
        //tot_terminal.current_terminal = current_terminal;
        // reset all the flags
        launch_ter =0; 
        ter_scheduling = current_terminal;
        ter_switch = 0;
        // backup the video memory
        update_terminal(old_terminal);
        // remap the VM addr 0xb8000 to PM addr 0xb8000, so that the shell could print the information string to the correct terminal
        new_video_map_2((uint32_t)VIDEO_MEMORY,(uint32_t)VIDEO_MEMORY);
        execute((uint8_t*)"shell");
        return;
    }
    // if no need to launch a new terminal, but ordinary switch, reset the flag and then back up the video memory 
    if(ter_switch == 1){
        tot_terminal.current_terminal = current_terminal;
        ter_switch = 0;
        update_terminal(old_terminal);
        
    }
    // if there is only one terminal being running, scheduler() do nothing, just return
    if(ter_scheduling == next_terminal ){
        return;
    }
    // remap the video addr 0xb8000 in VM addr to the correct physical addr  of video memory
    if(tot_terminal.current_terminal != next_terminal){
        if(next_terminal == 0){
            physical_addr = VIDEO_MEMORY1;
            }
        if(next_terminal == 1){
            physical_addr = VIDEO_MEMORY2;
            }
        if(next_terminal == 2){
            physical_addr = VIDEO_MEMORY3;
            }
        new_video_map_2((uint32_t)VIDEO_MEMORY,(uint32_t)physical_addr);
    }
    else{
        new_video_map_2((uint32_t)VIDEO_MEMORY,(uint32_t)VIDEO_MEMORY);
    }
   
   //  remap the fish image video addr 208MB in VM addr to the correct physical addr of video memory
    if(tot_terminal.current_terminal != next_terminal){
        if(next_terminal == 0){
            physical_addr = VIDEO_MEMORY1;
            }
        if(next_terminal == 1){
            physical_addr = VIDEO_MEMORY2;
            }
        if(next_terminal == 2){
            physical_addr = VIDEO_MEMORY3;
            }
        new_video_map(&start_screen,(uint32_t)physical_addr);
    }
    else{
        new_video_map(&start_screen,(uint32_t)0xb8000);
    }
    // obtain the pid of the process currently running, the last one in the pid array is the pid running 
    int32_t cur_pid = tot_terminal.terminal_pid_table[ter_scheduling][tot_terminal.terminal_process_count[ter_scheduling] - 1];
    // obtain the pid of the process to run
    int32_t next_pid = tot_terminal.terminal_pid_table[next_terminal][tot_terminal.terminal_process_count[next_terminal] - 1];
    // obtain the start address of each pcb
    pcb_t *cur_pcb = (pcb_t*)(SIZE_8MB - SIZE_8KB*(cur_pid+1));
    pcb_t *next_pcb = (pcb_t*)(SIZE_8MB - SIZE_8KB*(next_pid+1));
     
    // update the current terminal index
    ter_scheduling = next_terminal;
    // remap the program image virtual address to the current 
    new_map(SIZE_OF_128MB,SIZE_8MB + SIZE_OF_4MB * next_pid);

    tss.ss0 = KERNEL_DS;
    tss.esp0 = KERNEL_MEM_END - (next_pid) * SIZE_OF_8KB - WORD_SIZE;

    // save the current stack of the current process
    asm volatile(
        "movl %%esp, %%eax;"
        "movl %%ebp, %%ebx;"
        :"=a"(cur_pcb->saved_esp),"=b"(cur_pcb->saved_ebp)
        :
    );
    // restore the state of stack for the next process
    asm volatile(
        "movl %%eax, %%esp;"
        "movl %%ebx, %%ebp;"
        :
        :"a"(next_pcb->saved_esp), "b"(next_pcb->saved_ebp)
    );
   
}

