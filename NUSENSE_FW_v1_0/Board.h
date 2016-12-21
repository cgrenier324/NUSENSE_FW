/*
 * Copyright (c) 2015-2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __BOARD_H
#define __BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ti/drivers/Power.h>
#include "CC2650DK_7ID.h"
#include <driverlib/ioc.h>

/* These #defines allow us to reuse TI-RTOS across other device families */
#define     Board_LED1              Board_DK_LED1
#define     Board_LED2              Board_DK_LED2
//#define     Board_LED3              Board_DK_LED3
//#define     Board_LED4              Board_DK_LED4

#define     Board_LED0              Board_DK_LED4

#define     Board_BUTTON0           Board_KEY_UP
#define     Board_BUTTON1           Board_KEY_DOWN

#define     Board_UART0             Board_UART
#define     Board_AES0              Board_AES
#define     Board_WATCHDOG0         Board_WATCHDOG

//GlucoSense Board GPIO Definitions
#define 	GS_SIGNAL_IN						IOID_30 //analog input, sampled by ADC
#define 	GS_POLARITY_IN						IOID_26 //digital input

#define 	GS_PSU_EN							IOID_28 //enables analog rails for measurement
#define 	GS_INTEG_EN							IOID_20 //enables hardware integrator circuit, would require reconfiguring of analog input for ADC
#define 	GS_NEG_BIAS_EN						IOID_23 //enables DNP op-amp for biasing


/*
#if CURRENT_BOARD == BOARD_VERSION_1
	#define 	GS_INTEG_EN							IOID_20 //enables hardware integrator circuit, would require reconfiguring of analog input for ADC
	#define 	GS_NEG_BIAS_EN						IOID_23 //enables DNP op-amp for biasing
#elif CURRENT_BOARD == BOARD_VERSION_2
	#define		GS_CV_EN							IOID_20
	#define		GS_POL_EN							IOID_23
#endif
*/

#define 	GS_SPI0_ADC_CSN 					IOID_29 // External ADC SPI chip select, set HIGH initially
#define 	GS_SPI0_DAC_CSN						IOID_24 //External DAC SPI chip select, set HIGH initially

#define		GS_AUX_SIGNAL_IN					ADC_COMPB_IN_AUXIO0 //ADC is in AUX domain so map IO to corresponding AUX pin


#define     Board_initGeneral() { \
    Power_init(); \
    if (PIN_init(BoardGpioInitTable) != PIN_SUCCESS) \
        {System_abort("Error with PIN_init\n"); \
    } \
}

#define     Board_initGPIO()
#define     Board_initSPI()         SPI_init()
#define     Board_initUART()        UART_init()
#define     Board_initWatchdog()    Watchdog_init()
#define     GPIO_toggle(n)
#define     GPIO_write(n,m)

typedef enum CC2650_GPTimerName {
    CC2650_GPTIMER0A = 0,
    CC2650_GPTIMER0B,
    CC2650_GPTIMERUNITSCOUNT
} CC2650_GPTimerName;

typedef enum CC2650_GPTimers {
    CC2650_GPTIMER0 = 0,
    CC2650_GPTIMERCOUNT
} CC2650_GPTimers;

typedef enum CC2650_PWM {
    CC2650_PWM0 = 0,
    CC2650_PWM1,
    CC2650_PWMCOUNT
} CC2650_PWM;

#define Board_PWMPIN0  IOID_25     //7x7 LED1
#define Board_PWMPIN1  IOID_27     //7x7 LED2


#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H */
