#include "rtc.h"
#include "lib.h"
#include "i8259.h"
#include "schedule.h"
#include "systemcall.h"
volatile int RTC_interrupt[3];
int time_counter[3];
volatile int virtual_frequency[3];
//int permmit_writing[3];
/*
 * introduction: initialize the rtc
 * input: none
 * output: none
 */
void rtc_init()
{
    int i;
    for (i = 0; i < 3; i++)
    {
        virtual_frequency[i] = 2;
        time_counter[i] = 0;
        RTC_interrupt[i] = 0;
        //permmit_writing[i] = 0;
    }
    
    char temp;
    //time_counter = 0;
    disable_irq(RTC_IRQ);   //no interrupts happen
    outb(REGISTER_B,RTC_PORT);  //select Status Register B, and disable NMI
    temp = inb(RTC_DATA_PORT);  // read the current value of register B
    outb(REGISTER_B,RTC_PORT);  // set the index again (a read will reset the index to register D)
    outb(temp|0x40,RTC_DATA_PORT);  // write the previous value ORed with 0x40. This turns on bit 6 of register B
    enable_irq(RTC_IRQ);
}

/*
 * introduction: create a handler for idt to call 
 * input: none
 * output: none
 */
void rtc_interrupt_handler()
{
    cli();
    //int i;
    if ((time_counter[0] == 512/virtual_frequency[0]) && (tot_terminal.terminal_process_count[0] != 0)) 
    {
        RTC_interrupt[0] = 1;
        time_counter[0] = 0;
    }
    if ((time_counter[1] == 512/virtual_frequency[1]) && (tot_terminal.terminal_process_count[1] != 0))
    {
        RTC_interrupt[1] = 1;
        time_counter[1] = 0;
    }
    if ((time_counter[2] == 512/virtual_frequency[2]) && (tot_terminal.terminal_process_count[2] != 0))
    {
        RTC_interrupt[2] = 1;
        time_counter[2] = 0;
    }
    outb(REGISTER_C,RTC_PORT);  //select register C
    inb(RTC_DATA_PORT); //throw away content
    if (tot_terminal.terminal_process_count[0] != 0)
    {
        time_counter[0]++;
    }
    if (tot_terminal.terminal_process_count[1] != 0)
    {
        time_counter[1]++;
    }
    if (tot_terminal.terminal_process_count[2] != 0)
    {
        time_counter[2]++;
    }
    



   
    
    // if (time_counter[0] > 1024/virtual_frequency[0])
    // {
    //     time_counter[0] = time_counter[0] % (1024/virtual_frequency[0]);
    // }
    // if (time_counter[1] > 1024/virtual_frequency[1])
    // {
    //     time_counter[1] = time_counter[1] % (1024/virtual_frequency[1]);
    // }
    // if (time_counter[2] > 1024/virtual_frequency[2])
    // {
    //     time_counter[2] = time_counter[2] % (1024/virtual_frequency[2]);
    // }
    //test_interrupts();
    send_eoi(RTC_IRQ);
    sti();
}

/*
 * introduction: set the frequency to the RTC 
 * input: int rate
 * output: none
 */
void set_frequency(int rate)
{
    char temp;
    rate = rate & 0x0F; //low 4 bits
    //disable_irq(RTC_IRQ);
    outb(REGISTER_A,RTC_PORT);
    temp = inb(RTC_DATA_PORT);
    outb(REGISTER_A,RTC_PORT);
    temp = temp & 0xF0;
    outb(temp + rate,RTC_DATA_PORT);
    //enable_irq(RTC_IRQ);
}



/*
 * frequency = 32768 >> (rate-1)   https://wiki.osdev.org/RTC
 */
 /*
 * introduction: calculate the rate
 * input: int frequency
 * output: int rate
 */
int get_interrupt_rate(int frequency)
{
    if (frequency == 2) {return 15;}
    if (frequency == 4) {return 14;}
    if (frequency == 8) {return 13;}
    if (frequency == 16) {return 12;}
    if (frequency == 32) {return 11;}
    if (frequency == 64) {return 10;}
    if (frequency == 128) {return 9;}
    if (frequency == 256) {return 8;}
    if (frequency == 512) {return 7;}
    if (frequency == 1024) {return 6;}
    return -1;  //return -1 for fail
}


/* RTC_open
 * introduction: open the rtc, intialize it and set the frequency to 2hz
 * input: none
 * output: 0 for success ,can't fail
 */
int RTC_open(const uint8_t* filename)
{
 
    int frequency = 512;
    int rate = get_interrupt_rate(frequency);
    RTC_interrupt[current_terminal] = 0;
    time_counter[current_terminal] = 0;
    //virtual_frequency[current_terminal] = 2;
    //permmit_writing[current_terminal] = 1;
    set_frequency(rate);
   
    return 0;
}
/* RTC_close
 * introduction: close the rtc
 * input: none
 * output: 0 for success ,can't fail
 */

int RTC_close(int32_t fd)
{
    return 0;
}
/* RTC_read
 * introduction: read the rtc, wait until receive the RTC_interrupt signal
 * input: none
 * output: 0 for success ,can't fail
 */

int RTC_read(int32_t fd, void* buf, int32_t nbytes)
{   
    RTC_interrupt[ter_scheduling] = 0;
    while(RTC_interrupt[ter_scheduling] == 0){}  //wait for the interrupt
    return 0;
}

/* RTC_write
 * introduction: write the rtc, set an other frequency for rtc
 * input: void* buffer (contains the frequency)
 * output: 0 for success, 1 for fail, if the frequency is not the power of 2, fail
 */

int RTC_write(int32_t fd, const void* buf, int32_t nbytes)
{
    // int frequency;
    // int rate;
    // frequency = *((int*)buf);
    // //printf("%d",frequency);
    
    // rate = get_interrupt_rate(frequency);
    // if (rate == -1)     //if the frequency is not the power of 2 and not >2,<1024, return -1
    // {
    //     return -1;      //if fail to set frequency, return -1
    // }
    // set_frequency(rate);
    // return 0;           //return 0 for success
    cli();
    // if (permmit_writing[current_terminal] == 0)
    // {
    //     sti();
    //     return 0;
    // }
    // permmit_writing[current_terminal] = 0;
    virtual_frequency[current_terminal] = *((int*)buf);
    //time_counter = 0;
    sti();
    return 0;
}

//last version with comment



