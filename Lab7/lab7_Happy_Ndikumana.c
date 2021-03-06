// Backlight Library
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL with LCD Interface
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// Red Backlight LED:
//   M0PWM3 (PB5) drives an NPN transistor that powers the red LED
// Green Backlight LED:
//   M0PWM5 (PE5) drives an NPN transistor that powers the green LED
// Blue Backlight LED:
//   M0PWM4 (PE4) drives an NPN transistor that powers the blue LED
// Blue Backlight LED:
//   M0PWM4 (PB4) drives an NPN transistor that powers the blue LED

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "backlight.h"
#include "clock.h"
#include "uart0.h"

// PortB masks
#define RED_BL_LED_MASK 32
#define ORANGE_BL_LED_MASK 16

// PortE masks
#define BLUE_BL_LED_MASK 16
#define GREEN_BL_LED_MASK 32

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize backlight
void initializePwm()
{
    initSystemClockTo40Mhz();

    // Enable clocks
    SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R0;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1 | SYSCTL_RCGCGPIO_R4;
    _delay_cycles(3);

    // Configure three backlight LEDs
    GPIO_PORTB_DIR_R |= RED_BL_LED_MASK | ORANGE_BL_LED_MASK;                       // make bit5 an output
    GPIO_PORTB_DR2R_R |= RED_BL_LED_MASK | ORANGE_BL_LED_MASK;                      // set drive strength to 2mA
    GPIO_PORTB_DEN_R |= RED_BL_LED_MASK | ORANGE_BL_LED_MASK;                       // enable digital
    GPIO_PORTB_AFSEL_R |= RED_BL_LED_MASK | ORANGE_BL_LED_MASK;                     // select auxilary function
    GPIO_PORTB_PCTL_R &= ~(GPIO_PCTL_PB5_M | GPIO_PCTL_PB4_M);                      // enable PWM
    GPIO_PORTB_PCTL_R |= GPIO_PCTL_PB5_M0PWM3 | GPIO_PCTL_PB4_M0PWM2;
    GPIO_PORTE_DIR_R |= GREEN_BL_LED_MASK | BLUE_BL_LED_MASK;  // make bits 4 and 5 outputs
    GPIO_PORTE_DR2R_R |= GREEN_BL_LED_MASK | BLUE_BL_LED_MASK; // set drive strength to 2mA
    GPIO_PORTE_DEN_R |= GREEN_BL_LED_MASK | BLUE_BL_LED_MASK;  // enable digital
    GPIO_PORTE_AFSEL_R |= GREEN_BL_LED_MASK | BLUE_BL_LED_MASK;// select auxilary function
    GPIO_PORTE_PCTL_R &= ~(GPIO_PCTL_PE4_M | GPIO_PCTL_PE5_M);    // enable PWM
    GPIO_PORTE_PCTL_R |= GPIO_PCTL_PE4_M0PWM4 | GPIO_PCTL_PE5_M0PWM5;

    // Configure PWM module 0 to drive RGB backlight
    // ORANG on M0PWM2 (PB4), MOPWM1a
    // RED   on M0PWM3 (PB5), M0PWM1b
    // BLUE  on M0PWM4 (PE4), M0PWM2a
    // GREEN on M0PWM5 (PE5), M0PWM2b
    SYSCTL_SRPWM_R = SYSCTL_SRPWM_R0;                // reset PWM0 module
    SYSCTL_SRPWM_R = 0;                              // leave reset state
    PWM0_1_CTL_R = 0;                                // turn-off PWM0 generator 1 (drives outs 2 and 3)
    PWM0_2_CTL_R = 0;                                // turn-off PWM0 generator 2 (drives outs 4 and 5)
    PWM0_1_GENB_R = PWM_0_GENB_ACTCMPBD_ZERO | PWM_0_GENB_ACTLOAD_ONE;
                                                     // output 3 on PWM0, gen 1b, cmpb
    PWM0_2_GENA_R = PWM_0_GENA_ACTCMPAD_ZERO | PWM_0_GENA_ACTLOAD_ONE;
                                                     // output 4 on PWM0, gen 2a, cmpa
    PWM0_2_GENB_R = PWM_0_GENB_ACTCMPBD_ZERO | PWM_0_GENB_ACTLOAD_ONE;
                                                     // output 5 on PWM0, gen 2b, cmpb
    PWM0_1_GENA_R = PWM_0_GENA_ACTCMPAD_ZERO | PWM_0_GENA_ACTLOAD_ONE;
                                                     // output 2 on PWM0, gen 1a, cmpb
    PWM0_1_LOAD_R = 1024;                            // set frequency to 40 MHz sys clock / 2 / 1024 = 19.53125 kHz
    PWM0_2_LOAD_R = 1024;
    PWM0_INVERT_R = PWM_INVERT_PWM3INV | PWM_INVERT_PWM4INV | PWM_INVERT_PWM5INV | PWM_INVERT_PWM2INV;
                                                     // invert outputs so duty cycle increases with increasing compare values
    PWM0_1_CMPB_R = 512;                               // red off (0=always low, 1023=always high)
    PWM0_1_CMPA_R = 512;                               //
    PWM0_2_CMPB_R = 512;                               // green off
    PWM0_2_CMPA_R = 512;                               // blue off

    PWM0_1_CTL_R = PWM_0_CTL_ENABLE;                 // turn-on PWM0 generator 1
    PWM0_2_CTL_R = PWM_0_CTL_ENABLE;                 // turn-on PWM0 generator 2
    PWM0_ENABLE_R = PWM_ENABLE_PWM3EN | PWM_ENABLE_PWM4EN | PWM_ENABLE_PWM5EN | PWM_ENABLE_PWM2EN;
                                                     // enable outputs
}

void setPwmDutyCycle(uint8_t id, uint16_t pwmA, uint16_t pwmB)
{
    if(id == 0)
    {
        PWM0_1_CMPA_R = (pwmA * 1024) / 100;
        PWM0_1_CMPB_R = (pwmB * 1024) / 100;
    }
    else if(id == 1)
    {
        PWM0_2_CMPA_R = (pwmA * 1024) / 100;
        PWM0_2_CMPB_R = (pwmB * 1024) / 100;
    }
}
//void setBacklightRgbColor(uint16_t red, uint16_t green, uint16_t blue)
//{
//    PWM0_1_CMPB_R = red;
//    PWM0_2_CMPA_R = blue;
//    PWM0_2_CMPB_R = green;
//}

void waitMicrosecond(uint32_t us)
{
    __asm("WMS_LOOP0:   MOV  R1, #6");          // 1
    __asm("WMS_LOOP1:   SUB  R1, #1");          // 6
    __asm("             CBZ  R1, WMS_DONE1");   // 5+1*3
    __asm("             NOP");                  // 5
    __asm("             NOP");                  // 5
    __asm("             B    WMS_LOOP1");       // 5*2 (speculative, so P=1)
    __asm("WMS_DONE1:   SUB  R0, #1");          // 1
    __asm("             CBZ  R0, WMS_DONE0");   // 1
    __asm("             NOP");                  // 1
    __asm("             B    WMS_LOOP0");       // 1*2 (speculative, so P=1)
    __asm("WMS_DONE0:");                        // ---
                                                // 40 clocks/us + error
}

int main()
{
    initializePwm();

    while(1)
    {
        setPwmDutyCycle(1, 80, 0);
        waitMicrosecond(1000000);
        setPwmDutyCycle(1, 50, 0);
        waitMicrosecond(1000000);
        setPwmDutyCycle(1, 80, 100);
        waitMicrosecond(1000000);
        setPwmDutyCycle(1, 80, 80);
        waitMicrosecond(1000000);
        setPwmDutyCycle(1, 0, 50);
        waitMicrosecond(1000000);
        setPwmDutyCycle(1, 100, 80);
        waitMicrosecond(1000000);
        setPwmDutyCycle(1, 0, 0);
    }


    //while(true);
}
