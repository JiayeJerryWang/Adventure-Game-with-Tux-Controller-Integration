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
 
#define debug(str, ...) \
    printk(KERN_DEBUG "%s: " str, __FUNCTION__, ## __VA_ARGS__)
 
/************************ local varibles *************************/
//spinlock_t lock = SPIN_LOCK_UNLOCKED;
//unsigned long irq;
unsigned long led_status;
unsigned long button_status;
unsigned int ack;

/************************ local functions *************************/
int tuxctl_init (struct tty_struct* tty);
int tuxctl_set_led(struct tty_struct* tty, unsigned long arg);
int tuxctl_buttons (struct tty_struct* tty, unsigned long arg);
 
/************************ Protocol Implementation *************************/
 
/* tuxctl_handle_packet()
 * IMPORTANT : Read the header for tuxctl_ldisc_data_callback() in
 * tuxctl-ld.c. It calls this function, so all warnings there apply
 * here as well.
 */
void tuxctl_handle_packet (struct tty_struct* tty, unsigned char* packet)
{
    unsigned a, b, c;
    unsigned char buf[2];
 
    a = packet[0]; /* Avoid printk() sign extending the 8-bit */
    b = packet[1]; /* values when printing them. */
    c = packet[2];
    switch(a) {
        case MTCP_ACK:
            //if(ack == 1) {
                ack = 0;
                tuxctl_set_led(tty, led_status);
            //}
            return;
        case MTCP_BIOC_EVENT:
		/*byte 1  __7_____4___3___2___1_____0____
			| 1 X X X | C | B | A | START |
			-------------------------------
		 *byte 2  __7_____4_____3______2______1_____0___
			| 1 X X X | right | down | left | up |
			--------------------------------------
         * shift to | r | l |d | u | c | b | a | s |
         */
            button_status = ((b & 0xF) | (((((c >> 1) & 0x1) << 2) | (((c >> 2) & 0x1) << 1) | ((c & 0x9))) << 4) );
            return;
        case MTCP_RESET:
            button_status = 0xFF;
            led_status = led_status;
            buf[0] = MTCP_BIOC_ON;
            buf[1] = MTCP_LED_USR;
            tuxctl_ldisc_put(tty, buf, 2);
            ack = 0;
            tuxctl_set_led(tty, led_status);
            return;
        default:
            return;
    }
    /*printk("packet : %x %x %x\n", a, b, c);*/
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
int
tuxctl_ioctl (struct tty_struct* tty, struct file* file,
          unsigned cmd, unsigned long arg)
{
    switch (cmd) {
    case TUX_INIT:
        return tuxctl_init(tty);
    case TUX_BUTTONS:
        /* Returns -EINVAL error if this pointer is not valid */
        if (arg == 0) {
            return -EINVAL;
        } else {
            return tuxctl_buttons(tty, arg);            
        }
    case TUX_SET_LED:
        return tuxctl_set_led(tty, arg);
    case TUX_LED_ACK:
        return -EINVAL;
    case TUX_LED_REQUEST:
        return -EINVAL;
    case TUX_READ_LED:
        return -EINVAL;
    default:
        return -EINVAL;
    }
}
 
/*
 * tuxctl_init
 * Description: Takes no arguments. Initializes any variables associated with the driver and returns 0.
 * Inputs: tty - pointer to a tty_struct for use in calling the function tuxctl_ldisc_put
 * Outputs: None
 * Returns: This ioctl should return 0
 * Side Effects:
 */
int
tuxctl_init (struct tty_struct* tty) {
    unsigned char buf[2];
    led_status = 0x0; /* initialize led status to display nothing */
    button_status = 0xFF; /* initialize button status to (active low) 0xFF */
    ack = 0; 
    buf[0] = MTCP_BIOC_ON; /* Enable Button interrupt-on-change */
    buf[1] = MTCP_LED_USR; /* Put the LED display into user-mode. */
    tuxctl_ldisc_put(tty, buf, 2);
    return 0;
}
 
/*
 * tuxctl_set_led
 * Description: The argument is a 32-bit integer of the following form: The low 16-bits specify a number whose
 * hexadecimal value is to be displayed on the 7-segment displays. The low 4 bits of the third byte
 * specifies which LED’s should be turned on. The low 4 bits of the highest byte (bits 27:24) specify
 * whether the corresponding decimal points should be turned on.
 * Inputs:  tty - pointer to a tty_struct
 *          arg - pointer to integer in user space
 * Outputs: None
 * Returns: This ioctl should return 0
 * Side Effects:
 */
int
tuxctl_set_led(struct tty_struct* tty, unsigned long arg) {
    int i, j;

    /* led value store the lower 16 bit */
    unsigned int led_value = (arg & 0xFFFF);

    /* led_on store the  low 4 bits of the third byte */
    unsigned char led_on = ((arg >> 16) & 0xF);

    /* decimal store the low 4 bits of the highest byte */
    unsigned char decimal = ((arg >> 24) & 0xF);

    /* bit mask of seven segment display from 0 to F */
    unsigned char seven_segment_bitmask[16] = {0xE7, 0x06, 0xCB, 0x8F, 0x2E, 0xAD, 0xED, 0x86, 0xEF, 0xAF, 0xEE, 0x6D, 0xE1, 0x4F, 0xE9, 0xE8};

    /* intialize the 4 sevem segment led display to all dark */
    unsigned char seven_segment[4] = {0x0, 0x0, 0x0, 0x0};

    /* led_buf store the argument, byte 1 specifies which of the LED's to set, byte 2-5 determines one byte for each led to set.*/
    unsigned char led_buf[6];

    /* save led_status for reset */
    led_status =  arg;

    /* if ack is set, do not set led to avoid spamming */
    if (ack == 1) {
        return 0;
    } else {
        //ack = 1;
        /* set the 4 sevem segment led display */
        for (i = 0; i < 4; i++) {
            if (((led_on >> i) & 0x1) == 1) {
                seven_segment[i] = seven_segment_bitmask[(led_value >> (4 * i)) & 0x000F];
                seven_segment[i] = seven_segment[i] | (((decimal >> i) & 0x1) << 4);
            } else {
                seven_segment[i] = seven_segment[i] | (((decimal >> i) & 0x1) << 4);            
            }
        }

        /* set the led buf */
        led_buf[0] = MTCP_LED_SET;
        led_buf[1] = 0xF;

        /* assign led value to the led_buf*/
        for (j = 2; j < 6; j++) {
            led_buf[j] = seven_segment[j - 2];
        }

        tuxctl_ldisc_put(tty, led_buf, 6);
        //ack = 1;
        return 0;
    }
}
 
/*
 * tuxctl_buttons
 * Description: Takes a pointer to a 32-bit integer. Returns -EINVAL error if this pointer is not valid.
 * Otherwise sets the bits of the low byte corresponding to the currently pressed buttons.
 * Inputs:  tty - pointer to a tty_struct
 *          arg - pointer to integer in user space
 * Outputs: None
 * Returns: Returns -EINVAL error if this pointer is not valid.
 * Side Effects:
 */
int
tuxctl_buttons (struct tty_struct* tty, unsigned long arg) {
        *(unsigned long*) arg = button_status;
        return 0;
}
 
