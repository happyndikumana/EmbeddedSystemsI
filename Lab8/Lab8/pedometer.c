
#include <stdint.h>
#include <stdio.h>
#include "clock.h"
#include "tm4c123gh6pm.h"
#include "pedometer.h"

//#define RED_LED      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 1*4)))
//#define GREEN_LED    (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))
//#define BLUE_LED     (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4)))
//#define PUSH_BUTTON  (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 4*4)))

//// PortC masks
//#define FREQ_IN_MASK0 64
//#define FREQ_IN_MASK1 16

// PortF masks
//#define BLUE_LED_MASK 4
//#define GREEN_LED_MASK 8
//#define PUSH_BUTTON_MASK 16

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

//bool timeMode = false;
//uint32_t frequency = 0;
//uint32_t time = 0;

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------


void setTimerModeUp()
{
    WTIMER0_CTL_R &= ~TIMER_CTL_TAEN;                // turn-off counter before reconfiguring
    WTIMER0_CFG_R = 4;                               // configure as 32-bit counter (A only)
    WTIMER0_TAMR_R = TIMER_TAMR_TAMR_CAP | TIMER_TAMR_TACDIR; // configure for edge time mode, count up, capture mode
    WTIMER0_CTL_R |= TIMER_CTL_TAEVENT_POS;          // measure time from positive edge to positive edge
    WTIMER0_IMR_R &= ~TIMER_IMR_CAEIM;               // turn-off interrupts
    WTIMER0_TAV_R = 0;                               // zero counter for first period
    WTIMER0_CTL_R |= TIMER_CTL_TAEN;                 // turn-on counter
}
void setTimerModeDown()
{
    WTIMER1_CTL_R &= ~TIMER_CTL_TAEN;                // turn-off counter before reconfiguring
    WTIMER1_CFG_R = 4;                               // configure as 32-bit counter (A only)
    WTIMER1_TAMR_R = TIMER_TAMR_TAMR_CAP;            // configure for edge time mode, count down, capture mode
    WTIMER1_CTL_R |= TIMER_CTL_TAEVENT_POS;          // measure time from positive edge to positive edge
    WTIMER1_IMR_R &= ~TIMER_IMR_CAEIM;               // turn-off interrupts
    WTIMER1_TAV_R = 400;                               // zero counter for first period
    WTIMER1_CTL_R |= TIMER_CTL_TAEN;                 // turn-on counter
}

int countBalance(uint8_t id, int count0, int count1)
{
    //id == 1, count1 is ahead
    if(id == 1)
    {
        double percent = ((double)count0/(double)count1);
        double percentDifference = (1 - percent);
        double temp = (double)count1 * percentDifference;
        count1 = count1 - (int)temp;
        return count1;
    }
    //id == 0, count0 is head;
    else if(id == 0)
    {
        double percent = ((double)count1/(double)count0);
        double percentDifference = (1 - percent);
        double temp = (double)count0 * percentDifference;
        count0 = count0 - (int)temp;
        return count0;
    }
    return -1;
}


// Period timer service publishing latest time measurements every positive edge
//void wideTimer1Isr()
//{
//    if (timeMode)
//    {
//        time = WTIMER1_TAV_R;                        // read counter input
//        WTIMER1_TAV_R = 0;                           // zero counter for next edge
//        GREEN_LED ^= 1;                              // status
//    }
//    WTIMER1_ICR_R = TIMER_ICR_CAECINT;               // clear interrupt flag
//}
//0 - left, 1 - right
void setEncoderPosition(uint8_t id, int32_t position)
{
//    if(id != 1 && id != 0)
//        perror("Could not set position");
    if(id == 1)
        WTIMER1_TAV_R = position;
    else if(id == 0)
        WTIMER0_TAV_R = position;
}
int32_t getEncoderPosition(uint8_t id)
{
    if(id == 0)
    {
        return WTIMER0_TAV_R;
    }
    else if(id == 1)
        return WTIMER1_TAV_R;
    else
        return -1;
}
void selectEncoderIncMode(uint8_t id)
{
    if(id == 0)
    {
        WTIMER0_CTL_R &= ~TIMER_CTL_TAEN;
        WTIMER0_TAMR_R |= TIMER_TAMR_TACDIR;
        WTIMER0_CTL_R |= TIMER_CTL_TAEN;
    }
    else if(id == 1)
    {
        WTIMER1_CTL_R &= ~TIMER_CTL_TAEN;
        WTIMER1_TAMR_R |= TIMER_TAMR_TACDIR;
        WTIMER1_CTL_R |= TIMER_CTL_TAEN;
    }
}
void selectEncoderDecMode(uint8_t id)
{
    if(id == 0)
    {
        WTIMER0_CTL_R &= ~TIMER_CTL_TAEN;
        WTIMER0_TAMR_R &= ~ TIMER_TAMR_TACDIR;
        WTIMER0_CTL_R |= TIMER_CTL_TAEN;
    }
    else if(id == 1)
    {
        WTIMER1_CTL_R &= ~TIMER_CTL_TAEN;
        WTIMER1_TAMR_R &= ~TIMER_TAMR_TACDIR;
        WTIMER1_CTL_R |= TIMER_CTL_TAEN;
    }
}

// Initialize Hardware
void initPedometer()
{
    // Enable clocks
    // SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;
    SYSCTL_RCGCWTIMER_R |= SYSCTL_RCGCWTIMER_R1 | SYSCTL_RCGCWTIMER_R0;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R2 | SYSCTL_RCGCGPIO_R5;
    _delay_cycles(3);


    // Configure LED and pushbutton pins
    // GPIO_PORTF_DIR_R |= GREEN_LED_MASK | BLUE_LED_MASK;  // bits 1 and 2 are outputs, other pins are inputs
    // GPIO_PORTF_DIR_R &= ~PUSH_BUTTON_MASK;               // bit 4 is an input
    // GPIO_PORTF_DR2R_R |= GREEN_LED_MASK | BLUE_LED_MASK; // set drive strength to 2mA (not needed since default configuration -- for clarity)
    // GPIO_PORTF_DEN_R |= PUSH_BUTTON_MASK | GREEN_LED_MASK | BLUE_LED_MASK;
    //                                                      // enable LEDs and pushbuttons
    // GPIO_PORTF_PUR_R |= PUSH_BUTTON_MASK;                // enable internal pull-up for push button

    // Configure FREQ_IN for frequency counter0
    GPIO_PORTC_AFSEL_R |= FREQ_IN_MASK0;              // select alternative functions for FREQ_IN pin
    GPIO_PORTC_PCTL_R &= ~GPIO_PCTL_PC6_M;            // map alt fns to FREQ_IN
    GPIO_PORTC_PCTL_R |= GPIO_PCTL_PC6_WT1CCP0;       // selects the timer as the peripheral
    GPIO_PORTC_DEN_R |= FREQ_IN_MASK0;                // enable bit 6 for digital input

    // Configure FREQ_IN for frequency counter1
    GPIO_PORTC_AFSEL_R |= FREQ_IN_MASK1;              // select alternative functions for FREQ_IN pin
    GPIO_PORTC_PCTL_R &= ~GPIO_PCTL_PC4_M;            // map alt fns to FREQ_IN
    GPIO_PORTC_PCTL_R |= GPIO_PCTL_PC4_WT0CCP0;       // selects the timer as the peripheral
    GPIO_PORTC_DEN_R |= FREQ_IN_MASK1;                // enable bit 6 for digital input

    // Configure Wide Timer 0 & 1  as counter
    setTimerModeUp();
    setTimerModeDown();

    //set start distance to 0
    setEncoderPosition(1, 0);
    setEncoderPosition(0, 0);
    selectEncoderIncMode(1);
    selectEncoderIncMode(0);
}

