#include <msp430.h>

#include "morse.h"

/**
 * main.c
 */
unsigned short int loop_counter;
volatile unsigned char morse_flag;

#define TIMER_PERIOD 13000

enum morse_demo_state {
    INIT,
    SET_SINGLE_CHARACTER,
    SINGLE_CHARACTER,
    NO_MESSAGE,
    TEXT,
    FINISHED
};

// Timer B0 interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = TIMER0_B0_VECTOR
__interrupt void Timer_B (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER0_B0_VECTOR))) Timer_B (void)
#else
#error Compiler not supported!
#endif
{
    TB0CCR0 += TIMER_PERIOD;                             // Add Offset to TBCCR0
    morse_flag = 1;
    TB0CCTL0 &= ~CCIFG_1;
}

void init_gpios() {
    P1SEL0 &= (~BIT0); // Set P1.0 SEL for GPIO
    P1SEL1 &= (~BIT0); // Set P1.0 SEL for GPIO
    P1DIR |= BIT0; // red LED set as output
    P6DIR |= 1<<6; // green LED set as output
    PM5CTL0 &= ~LOCKLPM5; // to let the ports leave high-Z
}

void main(void) {

	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
    init_gpios();
	P1OUT |= BIT0; // turn on red LED

    TB0CCTL0 |= CCIE;                             // TBCCR0 interrupt enabled
    TB0CCR0 = TIMER_PERIOD;                       // set the initial value
    TB0CTL |= TBSSEL__SMCLK | MC__CONTINUOUS | ID_3;   // SMCLK, continuous mode
    __enable_interrupt();

	loop_counter = 0;
	morse_flag = 0; // this will be set from an interrupt

	char* text = "Error message here ";
	const unsigned char text_length = 19;
	unsigned char textidx = 0;
	enum morse_demo_state mds = INIT;
	int i = 0;

	for (;;) {
	    // this is the main loop of the program
	    loop_counter++;

	    switch(mds) {
	        case INIT:
	            clearMorsError();
	            mds = SET_SINGLE_CHARACTER;
	            break;

            case SET_SINGLE_CHARACTER:
                setMorsError('S');
                mds = SINGLE_CHARACTER;
                break;

	        case SINGLE_CHARACTER:
	            // first example: we transmit a single character,
	            // after which we go to the next state
	            if (morse_flag) {
	                morse_flag = 0;
	                if (updateMorsE()) { // this needs to be polled periodically
	                    clearMorsError(); // the single character has been shown
	                    mds = NO_MESSAGE;
	                }
	            }
	            break;

	        case NO_MESSAGE:
	            // this is a state where we don't call the morse encoder at all
                if (morse_flag) {
                    i++;
                    morse_flag = 0;
                }
                if (i > 50) {
                    i = 0;
                    mds = TEXT;
                }
                break;

	        case TEXT:
	            if (morse_flag) {
	                morse_flag = 0;
	                if (updateMorsE()) {
	                    setMorsError(text[textidx++]);
	                    if (textidx > text_length-1) {
	                        textidx = 0;
	                        i++;
	                    }
	                }
	            }
	            if (i > 3) {
	                clearMorsError();
	                mds = FINISHED;
	            }
	            break;

	        default:
	            mds = FINISHED;
	    }

	    _delay_cycles(5678); // here you would do something else, which takes some time

	    // end of main loop, go to sleep, etc
//	    __bis_SR_register(LPM3_bits | GIE);           // Enter LPM3 w/ interrupts
	    __no_operation();                             // For debug
	}
}
