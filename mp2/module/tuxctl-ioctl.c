/* tuxctl-ioctl.c
 *
 * Driver (skeleton) for the mp2 tuxcontrollers for ECE391 at UIUC.
 *
 * Mark Murphy 2006
 * Andrew Ofisher 2007
 * Steve Lumetta 12-13 Sep 2009
 * Puskar Naha 2013
 */

#include <asm/current.h>
#include <asm/uaccess.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/spinlock.h>

#include "tuxctl-ld.h"
#include "tuxctl-ioctl.h"
#include "mtcp.h"


void BIOC_handler(struct tty_struct* tty, unsigned int b, unsigned int c);
void TUX_INIT_handler(struct tty_struct * tty);
void TUX_SET_LED_handler(struct tty_struct * tty, unsigned long arg);
void MTCP_reset(struct tty_struct* tty);
static unsigned int ack_flag;
unsigned long button_status;
unsigned long led_display;
char middle_bits,right_side,left_size;

static spinlock_t lock_tux = SPIN_LOCK_UNLOCKED;
#define debug(str, ...) \
	printk(KERN_DEBUG "%s: " str, __FUNCTION__, ## __VA_ARGS__)
/*
struct status_lock{
	spinlock_t button_lock;
	unsigned long button_status;
}status_lock;
*/
// static spinlock_t button_lock = SPIN_LOCK_UNLOCKED;
// THIS HEXtobits ARE WRITTEN through the procotol that defined in the htcp.h
static const unsigned char hexToBits[16] = {
	0xE7, // 0
    0x06, // 1
    0xCB, // 2      
	0x8F, // 3
    0x2E, // 4
    0xAD, // 5
    0xED, // 6
    0x86, // 7
    0xEF, // 8
    0xAF, // 9
    0xEE, // A
    0x6D, // b this is lower 
    0xE1, // C
    0x4F, // d this is lower
    0xE9, // E
    0xE8, // F
};

/************************ Protocol Implementation *************************/

/* tuxctl_handle_packet()
 * IMPORTANT : Read the header for tuxctl_ldisc_data_callback() in 
 * tuxctl-ld.c. It calls this function, so all warnings there apply 
 * here as well.
 */
void tuxctl_handle_packet (struct tty_struct* tty, unsigned char* packet)
{
    unsigned a, b, c;
	

    a = packet[0]; /* Avoid printk() sign extending the 8-bit */
    b = packet[1]; /* values when printing them. */
    c = packet[2];

	switch(a){
		case MTCP_BIOC_EVENT:
			BIOC_handler(tty,b,c);
			break;
		case MTCP_ACK:
			ack_flag = 1; // the last command is over 
			break;
		case MTCP_RESET:
			MTCP_reset(tty);
			break;
	}
    /*printk("packet : %x %x %x\n", a, b, c); */
}

/******** IMPORTANT NOTE: READ THIS BEFORE IMPLEMENTING THE IOCTLS ************
 *                                                                            *
 * The ioctls should not spend any time waiting for responses to the commands *
 * they send to the controller. The data is sent over the serial line at      *
 * 9600 BAUD. At this rate, a byte takes approximately 1 millisecond to       *
 * transmit; this means that there will be about 9 milliseconds between       *
 * the time you request that the low-level serial driver send the             *
 * 6-byte SET_LEDS packet and the time the 3-byte ACK packet finishes         *
 * arriving. This is far too long a time for a system call to take. The       *
 * ioctls should return immediately with success if their parameters are      *
 * valid.                                                                     *
 *                                                                            *
 ******************************************************************************/

/* MTCP_reset()
 * IMPORTANT : init the tux and restore the led values
 * input: tty_struct
 * output: none
 * return: none
 */

void MTCP_reset(struct tty_struct* tty){
	int flag;
	unsigned char write_buffer[2] = {MTCP_BIOC_ON, MTCP_LED_USR};
	if (ack_flag == 1) {
		//unsigned char write_buffer[2] = {MTCP_BIOC_ON, MTCP_LED_USR};
		tuxctl_ioctl(tty, NULL,TUX_SET_LED,led_display);
	}
	ack_flag = 0;
	spin_lock_irqsave(&lock_tux,flag);
	button_status = 0xFF;
	spin_unlock_irqrestore(&lock_tux,flag);
	//status_lock.button_lock;
	//button_lock = SPIN_LOCK_UNLOCKED;
	// unsigned char write_buffer[2] = {MTCP_BIOC_ON, MTCP_LED_USR};
	tuxctl_ldisc_put(tty, write_buffer, 2);
	
	//tuxctl_ioctl(tty, NULL,TUX_INIT,0);
	
	return;
}

/*
 * void bioc_handler(struct tty_struct * tty, char b, char c)
 * DESCRIPTION:   This is called whenever a BIOC event occurs, reads
 *                the values of the buttons and stores them in the
 *                button_status global variable, which is active low
 * INPUTS:        tty - the location from where the line discipline
 *                gets the button input
 *                b - second byte given to the program by the line discipline
 *                c - third byte given to the program by the line discipline
 * OUTPUTS:       none
 * SIDE EFFECTS:  Takes the second and third bytes given from the line
 *                discipline from a BIOC event and writes the status of the
 *                buttons to the button_status global variable
 */
void BIOC_handler(struct tty_struct* tty, unsigned int b, unsigned int c){
	//unsigned long flags;
	//spin_lock_irqsave(&(status_lock.button_lock),flags);
	int flag;
	middle_bits = (~c) & 0x0F;
	left_size = (((middle_bits & 0x02) << 1) | ((middle_bits & 0x04) >> 1) | (middle_bits & 0X09)) << 4;
	right_side = ((~b)& 0x0F);
	spin_lock_irqsave(&lock_tux,flag);
	
	button_status = left_size | right_side;
	//button_status = (b & 0x0F) | ((c & 0x09) << 4) | ((c & 0x04) << 3) | ((c & 0x02) << 5); // That is move the bits into the form that we require
	//button_status = ((c & 0x09)|((c & 0x02) << 1)|((c & 0x04) >> 1) << 4) | ( b % 0x0F);
	button_status = (~button_status); // it is active low;
	button_status = button_status & 0x00FF;
	spin_unlock_irqrestore(&lock_tux,flag);
	//printk("button_status: %lx \n", button_status);
	//printf("the button status is %x",button_status);
	//spin_unlock_irqrestore(&(status_lock.button_lock),flags);
	return;
}
/*
 * int tuxctl_ioctl (struct tty_struct* tty, struct file* file, unsigned cmd, unsigned long arg)
 * DESCRIPTION:   This is used by the tux to handle command into line discipline
 * INPUTS:        tty - the location from where the line discipline gets the button input
 * 				  file - the file to read
 *                cmd - the command to be recognized
 *                arg - the argument to be execute
 * OUTPUTS:       none
 * SIDE EFFECTS:  none
 */
int 
tuxctl_ioctl (struct tty_struct* tty, struct file* file, 
	      unsigned cmd, unsigned long arg)
{
	//unsigned long flags;
	int flag;
	int check;
	unsigned long temporary;
    switch (cmd) {
		case TUX_INIT:
			TUX_INIT_handler(tty);
			break;
		case TUX_BUTTONS:
			if(arg == 0){
				return -EINVAL;
			}
			//spin_lock_irqsave(&(status_lock.button_lock),flags);
			spin_lock_irqsave(&lock_tux,flag);
			temporary = 0x00FF & button_status;
			spin_unlock_irqrestore(&lock_tux,flag);

			//printk("the button message %x", temporary);
				//*(unsigned long*)arg =  (0x00FF & button_status);
			check = copy_to_user((void*)arg, (void*)&temporary,sizeof(unsigned long));
			//spin_unlock_irqrestore(&(status_lock.button_lock),flags);
			if(check == 0){
				break;
			}
			return -EINVAL;
		case TUX_SET_LED:
			TUX_SET_LED_handler(tty, arg);
			return 0;
		// case TUX_LED_ACK:
		// case TUX_LED_REQUEST:
		// case TUX_READ_LED:
	default:
	    return -EINVAL;
    }
	return 0;
}
/*
 * void TUX_INIT_handler(struct tty_struct * tty)
 * DESCRIPTION:   This is used to init the Tux, that is init everything needed
 * INPUTS:        tty - the location from where the line discipline gets the button input
 * 				  
 * OUTPUTS:       none
 * SIDE EFFECTS:  none
 */
void TUX_INIT_handler(struct tty_struct * tty){
	unsigned char write_buffer[2] = {MTCP_BIOC_ON, MTCP_LED_USR};
	int flag;
	ack_flag = 0;
	// button_status = 0;
	led_display = 0;
	spin_lock_irqsave(&lock_tux,flag);
	button_status = 0xFF;
	spin_unlock_irqrestore(&lock_tux,flag);
	//button_lock = SPIN_LOCK_UNLOCKED;
	//status_lock.button_lock
	
	
	tuxctl_ldisc_put(tty, write_buffer, 2);
	return;
}

/*
 * void TUX_SET_LED_handler(struct tty_struct * tty, unsigned long arg)
 * DESCRIPTION:   This is used to set the led display value, the rule be obey is in the head file
 * INPUTS:        tty - the location from where the line discipline gets the button input
 * 				  arg - the argument
 * 				  
 * OUTPUTS:       none
 * SIDE EFFECTS:  none
 */
void TUX_SET_LED_handler(struct tty_struct * tty, unsigned long arg){
	int i;
	short hex = arg & 0xFFFF;
	
	
	unsigned int led_setting = (arg >> 16) & 0x000F; // led_setting stores the part that which led is on, 4 bits long
	unsigned int decimal = (arg >> 24) & 0x000F; // decimal store the part that decide which decimal points should on, 4 bits long

	
	//char led[4];
	char write_buffer[6] = {MTCP_LED_SET, 0x0F, 0, 0, 0, 0}; // the buffer stores things the things to be written in to displicine line,0x0F means the led to light
	int bitmask = 0x01; // set 1 to be on

	led_display = arg;
	for(i = 0; i < 4; i++){
		if((bitmask & led_setting) == 1){
			write_buffer[i+2] = hexToBits[(hex >> (i*4)) & 0x0F]; // convert the hex to bits that is going to be displayed on screen
		}
		
		if((bitmask & decimal) == 1){
			write_buffer[i+2] = write_buffer[i+2] | 0x10; // check the decimal points
		}
		led_setting = led_setting >> 1;
		decimal = decimal >> 1;
	}
	tuxctl_ldisc_put(tty, write_buffer, 6);
	
}
