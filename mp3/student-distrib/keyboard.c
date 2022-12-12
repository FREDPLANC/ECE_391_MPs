#include "keyboard.h"
#include "lib.h"
#include "i8259.h"
#include "systemcall.h"
#include "schedule.h"
int global_keyboard_index[MAX_TERMINAL] = {0,0,0};
volatile int keyboard_flag[MAX_TERMINAL] = {0,0,0};
//int global_keyboard_index = 0;
unsigned int status_shift[MAX_TERMINAL] = {0,0,0};  //0 for release, 1 for press
unsigned int status_ctrl[MAX_TERMINAL] = {0,0,0};   //0 for release, 1 for press
unsigned int status_caps[MAX_TERMINAL] = {0,0,0};   //0 for unlock, 1 for lock
unsigned int special_change[MAX_TERMINAL] = {0,0,0};    //0 for no special button change
unsigned int status_alt[MAX_TERMINAL] = {0,0,0};
/*this is a scan_code table, which is used for search each key's ASCII by global_keyboard_index*/



//when releasing caps and shift 
unsigned char scancode_lower[58] = 
{
    0,0,'1','2','3','4','5','6','7','8','9','0','-','=',
    8,0,'q','w','e','r','t','y','u','i','o','p','[',']',
    '\n',0,'a','s','d','f','g','h','j','k','l',';',39,
    '`',0,'\\','z','x','c','v','b','n','m',',','.','/',
    0,0,0,'\0'
};
//when pressing shift and releasing caps
unsigned char scancode_upper[58] =
{
    0,0,'!','@','#','$','%','^','&','*','(',')','_','+',
    8,0,'Q','W','E','R','T','Y','U','I','O','P','{','}',
    '\n',0,'A','S','D','F','G','H','J','K','L',':',34,
    '~',0,'|','Z','X','C','V','B','N','M','<','>','?',
    0,0,0,'\0'
};
//when pressing caps and releasing shift
unsigned char scancode_caps_only[58] = 
{
    0,0,'1','2','3','4','5','6','7','8','9','0','-','=',
    8,0,'Q','W','E','R','T','Y','U','I','O','P','[',']',
    '\n',0,'A','S','D','F','G','H','J','K','L',';',39,
    '`',0,'\\','Z','X','C','V','B','N','M',',','.','/',
    0,0,0,'\0'
};
//when preesing shift and caps
unsigned char scancode_shift_caps[58] = 
{
    0,0,'!','@','#','$','%','^','&','*','(',')','_','+',
    8,0,'q','w','e','r','t','y','u','i','o','p','{','}',
    '\n',0,'a','s','d','f','g','h','j','k','l',':',34,
    '~',0,'|','z','x','c','v','b','n','m','<','>','?',
    0,0,0,'\0'
};

/*  introduction: judge if pressing a special button and set status
 *  input: key
 *  output: none
 */
void special_button_status(unsigned int key)
{
    special_change[current_terminal] = 0;     //if the status of special button changes, set special_change to 1
    if ((key == LSHIFT_PRESS) || (key == RSHIFT_PRESS))     //if pressing the shift
    {
        status_shift[current_terminal] = 1;
        special_change[current_terminal] = 1;
    }
    if ((key == LSHIFT_RELEASE) || (key == RSHIFT_RELEASE))     // if releasing the shift
    {
        status_shift[current_terminal] = 0;
        special_change[current_terminal] = 1;
    }
    if (key == CAPS)    //press caps, the status of caps changes
    {
        status_caps[current_terminal] = ~status_caps[current_terminal];
        status_caps[current_terminal] = status_caps[current_terminal] & 0X01;   //status_caps is a 1 bit number
        special_change[current_terminal] = 1;
    }
    if (key == CTRL_PRESS)      //if pressing ctrl
    {
        status_ctrl[current_terminal] = 1;
        special_change[current_terminal] = 1;
    }
    if (key == CTRL_RELEASE)        //if releasing ctrl
    {
        status_ctrl[current_terminal] = 0;
        special_change[current_terminal] = 1;
    }
    if ((key == LALT_PRESS))     //if pressing the shift
    {
        status_alt[current_terminal] = 1;
        special_change[current_terminal] = 1;
    }
    if ((key == LALT_RELEASE))     // if releasing the shift
    {
        status_alt[current_terminal] = 0;
        special_change[current_terminal] = 1;
    }
    return;
}



/*
 * introduction: initialize the keyboard
 * input: none
 * output: none
 */
void keyboard_init()
{
    enable_irq(KEYBOARD_IRQ);       //enable the keyboard to receive interrupt
}

/*
 * introduction: create a handler for idt to call 
 * input: none
 * output: none
 */
void keyboard_interrupt_handler()
{   cli();
    unsigned int key;
    
    unsigned int value;
    key = 0;
    key = inb(KEYBOARD_PORT) & 0xFF;    //low 8 bits

    special_button_status(key);

    if (key == 0x39)    //0x39 is the scancode for space
    {
        if (global_keyboard_index[current_terminal] < 127)        //the 127 place of the buffer is '\0'
        {
            keyboard_buffer[current_terminal][global_keyboard_index[current_terminal]++] = 32;  //32 for blank
            put_terminal(32);
            //print_stuff(32,global_keyboard_index);
        }
        send_eoi(KEYBOARD_IRQ);
        sti();
        return;
    }
    if (key == 0x0F)    //0x0F is the scancode for TAB
    {
        if (global_keyboard_index[current_terminal] <= 127-4)
        {
            keyboard_buffer[current_terminal][global_keyboard_index[current_terminal]++] = 32;  //32 for blank
            put_terminal(32);
            //print_stuff(32,global_keyboard_index);
            keyboard_buffer[current_terminal][global_keyboard_index[current_terminal]++] = 32;  //32 for blank
            put_terminal(32);
            //print_stuff(32,global_keyboard_index);
            keyboard_buffer[current_terminal][global_keyboard_index[current_terminal]++] = 32;  //32 for blank
            put_terminal(32);
            //print_stuff(32,global_keyboard_index);
            keyboard_buffer[current_terminal][global_keyboard_index[current_terminal]++] = 32;  //32 for blank
            put_terminal(32);
            //print_stuff(32,global_keyboard_index);
        }
        send_eoi(KEYBOARD_IRQ);
        sti();
        return;
    }
    if(status_alt[current_terminal] == 1 ) // based on the alt + ? button, change the flag, wait for the scheduler() to do the switch terminal and backup Video memory work
    {
        if(ter_switch == 0){
            old_terminal = current_terminal;
            switch(key){
                case F1_PRESS:
                    current_terminal = 0;
                    cli(); 
                    ter_switch = 1;
                    sti();
                    break;
                case F2_PRESS:
                    current_terminal = 1;
                    cli();
                    ter_switch = 1;                  
                    sti();
                    break;
                case F3_PRESS:
                    current_terminal = 2;
                    cli();
                    ter_switch = 1;
                    sti();
                    break;
            }
        }
        
    }
    if (key>=0 && key<=57)      //if the scancode is in the array
    {
        if ((status_ctrl[current_terminal]==1) && (key == 0x26))  //scancode of 'l' is 0x26
        {
            clean_screen_terminal();         //if press ctrl+l, clean the screen, clean the current terminal
            send_eoi(KEYBOARD_IRQ);
            sti();
            return;
        }
        if (key == ENTER)       
        {
            //terminal_read(global_keyboard_index);
            //terminal_write(terminal_read(global_keyboard_index));
            // int write;
            // write = terminal_read(global_keyboard_index);
            // terminal_write(write);
    		// terminal_read();

            keyboard_flag[current_terminal] = 1;
            send_eoi(KEYBOARD_IRQ);
            sti();
            return;
        }
        if (key == BACKSPACE)       //if press backspace 
        {
            //int old_index = global_keyboard_index;
            if (global_keyboard_index[current_terminal] > 0){
                keyboard_buffer[current_terminal][global_keyboard_index[current_terminal]-1] = '\0';
                global_keyboard_index[current_terminal] = global_keyboard_index[current_terminal] -1;
                change_line_terminal(-1);    //change line
                update_cursor_terminal(0);
            }
            else{
                send_eoi(KEYBOARD_IRQ);
                sti();
                return; 
            }
            send_eoi(KEYBOARD_IRQ);
            sti();
            return;
        }
        if (special_change[current_terminal] == 1)    //if the special button has been changed, no need for further judgement below
        {
            send_eoi(KEYBOARD_IRQ);
            sti();
            return;
        }
        if ((status_caps[current_terminal]==0)&&(status_shift[current_terminal]==0))
        {
            value = scancode_lower[key];    //use different scancode array to find the correct ascii
        }
        if ((status_caps[current_terminal]==1)&&(status_shift[current_terminal]==0))
        {
            
            value = scancode_caps_only[key];    //use different scancode array to find the correct ascii
        }
        if ((status_caps[current_terminal]==0)&&(status_shift[current_terminal]==1))
        {
            value = scancode_upper[key];    //use different scancode array to find the correct ascii
        }
        if ((status_caps[current_terminal]==1)&&(status_shift[current_terminal]==1))
        {
            value = scancode_shift_caps[key];   //use different scancode array to find the correct ascii
        }
        
        if (value == '\0')      // if enter a '\0', return
        {
            send_eoi(KEYBOARD_IRQ);
            sti();
            return;
        }
        
        if (global_keyboard_index[current_terminal] < 127)    //if the buffer still have space 127 is the max
        {
            keyboard_buffer[current_terminal][global_keyboard_index[current_terminal]++] = value;   //put the value in the buffer
            //print_stuff(value,global_keyboard_index);       //print the value
            put_terminal(value);
            //putc(value);
        }
    } 
        
    send_eoi(KEYBOARD_IRQ);
    sti();
    return;
} 
/* print_stuff
 * introduction: a modified version of putc, which can change line and scroll down according to the index
 * input: value: the value to print; indexP, the index of it on the screen, which is almost the same as screen_X
 * output: none
 */
void print_stuff(int value,int indexP){
    if((indexP % 80) == 0){ // check whether the buffer has filled a row, 80 is the length of the x-axis
        putc(value);
        change_line(1);  // change line by one, if necssary scroll down for one line
        update_cursor(0);  // the offset is not sure
    }
    else{
        putc(value);
        update_cursor(0);
    }
    return;
}

/* terminal_open
 * introduction: clean the screen and update the cursor, which should be added more function in later checkpoints
 * input: nbytes : the bytes to be read
 * output: 0 if success
 */
//////////////////////////////////////////////////////
int terminal_open(const uint8_t* filename){
    clean_screen();                     // clean the screen first
    update_cursor(0);                   // update the cursor
    return 0;
}
/* terminal_close
 * introduction: close the terminal
 * input: nbytes
 * output: 0 if success
 */
int terminal_close(int32_t fd){
    disable_cursor();
    return 0;
}
/* terminal_read
 * introduction: read the character from the keyboard and store it in the terminal buffer
 * input: nbytes: bytes to be read
 * output: bytes that is successfully read
 */
int terminal_read(int32_t fd, void* buf, int32_t nbytes){
    //puts("terminal read called");
    int byte_read;
    int i = 0;
    char * kb_buf =  (char*)buf;
    if (fd!=0) {return -1;}
    //printf("%d",global_keyboard_index);
    byte_read = 0;
    if(strlen(keyboard_buffer[ter_scheduling]) >= 128) return -1;
    if(global_keyboard_index[ter_scheduling] >= 128) global_keyboard_index[ter_scheduling] = 127;

    while(!keyboard_flag[ter_scheduling]);              // wait untill the keyboard has entered a "enter"
    keyboard_flag[ter_scheduling] = 0;                  // reset the flag to be used next time
    /////////////////// Maybe space for enter actions
    //putc((int)('\n')); // change the line

    if((global_keyboard_index[ter_scheduling]) != 80){  // if the index is 80(last bit of the characr + 1), that should change one line
        change_line(1);
        kb_buf[global_keyboard_index[ter_scheduling]] = '\n';
        terminal_buffer[ter_scheduling][global_keyboard_index[ter_scheduling]] = '\n';
        byte_read++;
    }
   
    update_cursor(0);
    //update_cursor_terminal(0);


    /////////////////// Maybe space for enter actions

    for(i = 0; i < global_keyboard_index[ter_scheduling]; i++){
        /*
        if(keyboard_buffer[i] == '\n'){ // when the enter is detected, end reading
            keyboard_buffer[i] = '\0';
            break;
        }
        */
        if(i >= 128){    // check for overflow
            terminal_buffer[ter_scheduling][i] = '\0';
            kb_buf[i] = '\0';
            break;
        }
        kb_buf[i] = keyboard_buffer[ter_scheduling][i];
        terminal_buffer[ter_scheduling][i] = keyboard_buffer[ter_scheduling][i];
        byte_read += 1;
        keyboard_buffer[ter_scheduling][i] = '\0'; // clear the buffer
    }
    //terminal_buffer[global_keyboard_index] = '\n';
    //putc(kb_buf[2]);
    //printf("%d",&nbytes);
    global_keyboard_index[ter_scheduling] = 0; ////////////////////////////////////////////////pay much attention here, it will initialize the index
    nbytes = 0;
    return byte_read;
}
/* terminal_write 
 * introduction: write the bytes read into the buffer
 * input: nbytes : bytes to be written
 * output: bytes that are successfully written
 */
int terminal_write(int32_t fd, const void* buf, int32_t nbytes){
    int byte_write;
    int i = 0;
    byte_write = 0;
    char * termi_buf = (char *)buf;
    if (fd!=1) {return -1;}
    //if (nbytes > 80)  scrolling(1);
    //puts("this is output:");
    //putc(termi_buf[2]);
    
    if(nbytes == 0){
        nbytes = strlen(termi_buf);
    }
    for(i = 0; i < nbytes; i++){
        // if((i == 80) & (terminal_buffer[i] != '\n')){    // if it meet the end of the line or user pressed an "enter" 80 is the end character of the line
        //     change_line(1);
        // }
        // if(terminal_buffer[i] == '\n'){
        //     change_line(1);
        // }
        // else{
        //     putc(terminal_buffer[i]);
        // }
        putc(termi_buf[i]);
        byte_write += 1;
        //termi_buf[i] = '\0'; // clear the buffer
        update_cursor(0); // update the cursor by one place 
    
    }
    return byte_write;
}

void terminal_table_init(){
    int i,j;
    tot_terminal.t_count = 0;
    tot_terminal.current_terminal = 0;
    for (i= 0; i < MAX_TERMINAL; i ++){
        for (j = 0; j < MAX_PROC; j++){
            tot_terminal.terminal_pid_table[i][j] = 0;
        }
        tot_terminal.terminal_process_count[i] = 0;
    }
}
