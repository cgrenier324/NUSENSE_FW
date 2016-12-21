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

/*
 *  ======== mutex.c ========
 */

/* XDC module Headers */
#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/System.h>

/* BIOS module Headers */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/family/arm/m3/Hwi.h>
//#include <ti/sysbios/family/arm/cc26xx/Power.h>
//#include <ti/sysbios/family/arm/cc26xx/PowerCC2650.h>

/* TI-RTOS Header files */
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/mw/lcd/LCDDogm1286.h>
//#include <ti/drivers/lcd/LCDDogm1286.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/spi/SPICC26XXDMA.h>
#include <ti/drivers/dma/UDMACC26XX.h>


/* CC26XX Driver Library Header Files */
#include <driverlib/cpu.h>
#include <driverlib/aux_adc.h>
#include <driverlib/aux_wuc.h>
#include <inc/hw_aux_evctl.h>


/* Example/Board Header files */
#include "Board.h"

#define 	SAMPLERATE		20 								//Hz
#define		SAMPLEPERIOD	( 1000 / SAMPLERATE )			//milliseconds
#define 	RECORDLENGTH	30 								//seconds
#define 	SAMPLECOUNT 	(SAMPLERATE * RECORDLENGTH) 	//number of samples

uint16_t	DACTURNONVALUE  = 	0x00F8;							//16-bit code = VDACOUT*4096/VREF; (VDACOUT = 0.2 V, VREF = 3V3_A),
uint16_t	DACTURNOFFVALUE =	0xF000;							//16-bit code to power down DAC with Hi-Z at output

uint32_t 	ADC_sample_unadjusted = 0; //return value from AUXADCReadFifo()
uint32_t 	ADC_sample_adjusted = 0;
uint32_t 	adjusted_ADC_samples_uVolts[SAMPLECOUNT] = {0}; //array of returned values from AUXADCReadFifo()
uint32_t 	measurement_times[SAMPLECOUNT] = {0};
uint8_t	 	polarity_samples[SAMPLECOUNT] = {84};
int32_t 	gain_setting = 0;
int32_t 	offset_setting = 0;

#define TASKSTACKSIZE   512

void workTaskFxn(UArg arg0, UArg arg1);
void ADCTaskFxn(UArg arg0, UArg arg1);
void setDACVoltage(uint16_t DACValue);
//GPTimerCC26XX_registerInterrupt(hTimer, timerCallback, GPT_INT_TIMEOUT);
Int resource = 0;
Int finishCount = 0;
UInt32 sleepTickCount;

Task_Struct workTask, ADCTask;
Char workTaskStack[TASKSTACKSIZE], ADCTaskStack[TASKSTACKSIZE];

static PIN_Handle 	pinHandle;
static PIN_State 	pinState;

Semaphore_Struct ADCSem, ADCDoneSem;
Semaphore_Handle hADCSem, hADCDoneSem;

void doWork(void)
{
	PIN_setOutputValue(pinHandle, Board_LED1, 1);
	CPUdelay(12e6); /* Pretend to do something useful but time-consuming */
	PIN_setOutputValue(pinHandle, Board_LED1, 0);
}

void workTaskFunc(UArg arg0, UArg arg1)
{
	while (1)
	{
		doWork();
		Task_sleep(1000 * 1000 / Clock_tickPeriod); //delay 1 second
	}
}

/*
void timerCallback(GPTimerCC26XX_Handle handle, GPTimerCC26XX_IntMask interruptMask)
{
}
*/

void adcIsr(UArg a0)
{
	ADC_sample_unadjusted = AUXADCReadFifo(); // Pop sample from FIFO to allow clearing ADC_IRQ event
	HWREGBITW(AUX_EVCTL_BASE + AUX_EVCTL_O_EVTOMCUFLAGSCLR, AUX_EVCTL_EVTOMCUFLAGSCLR_ADC_IRQ_BITN) = 1; // Clear ADC_IRQ flag. Note: Missing driver for this.
	Semaphore_post(hADCDoneSem); // Post semaphore to wakeup task
}


void sampleADC(void)
{
	gain_setting = AUXADCGetAdjustmentGain(AUXADC_REF_FIXED);
	offset_setting = AUXADCGetAdjustmentOffset(AUXADC_REF_FIXED);
	AUXWUCClockEnable(AUX_WUC_MODCLKEN0_ANAIF_M|AUX_WUC_MODCLKEN0_AUX_ADI4_M); // Enable clock for ADC digital and analog interface (not currently enabled in driver)
	AUXWUCClockEnable(AUX_WUC_ADI_CLOCK|AUX_WUC_MODCLKEN0_ANAIF_M|AUX_WUC_ADC_CLOCK);
	AUXADCSelectInput(GS_AUX_SIGNAL_IN); // Connect AUX IO0 (DIO30) as analog input. GS_SIGNAL_IN from GS board
	AUXADCEnableSync(AUXADC_REF_FIXED, AUXADC_SAMPLE_TIME_682_US, AUXADC_TRIGGER_MANUAL);
	Power_setConstraint(PowerCC26XX_SB_DISALLOW); // Disallow STANDBY mode while using the ADC.

	PIN_setOutputValue(pinHandle, GS_PSU_EN, 1); //enable analog rail that provides power for DAC and rest of signal acquisiton circuit
	System_printf("GS Analog Rails on\n");
	System_flush();
	setDACVoltage(DACTURNONVALUE); //equates to setting DAC output voltage to 200 mV
	System_printf("DAC voltage set to 200mV\n");
	System_flush();

	PIN_setOutputValue(pinHandle, Board_LED2, 1);
	System_printf("Starting ADC Sampling\n");
	System_flush();


	uint16_t i = 0;
	while(i < SAMPLECOUNT)
	{
		Task_sleep( SAMPLEPERIOD * 1000 / Clock_tickPeriod); 	//Sleep in IDLE mode for time between samples
		AUXADCGenManualTrigger(); 					// Trigger ADC sampling
		measurement_times[i] = Clock_getTicks();

		Semaphore_pend(hADCDoneSem, BIOS_WAIT_FOREVER ); 	// Wait in IDLE until done

		ADC_sample_adjusted = AUXADCAdjustValueForGainAndOffset(ADC_sample_unadjusted, gain_setting, offset_setting);
		adjusted_ADC_samples_uVolts[i] = AUXADCValueToMicrovolts(AUXADC_FIXED_REF_VOLTAGE_NORMAL, ADC_sample_adjusted);
		polarity_samples[i] = PIN_getInputValue(GS_POLARITY_IN);
		i++;
	}

	AUXADCDisable(); // Disable ADC
	Power_releaseConstraint(PowerCC26XX_SB_DISALLOW); // Allow STANDBY mode again
	setDACVoltage(DACTURNOFFVALUE); // Power down DAC with Hi-Z at output
	System_printf("DAC set to HIGH Z\n");
	System_flush();
	PIN_setOutputValue(pinHandle, GS_PSU_EN, 0); //disable analog rails for power saving
	System_printf("GS Power Rail disabled\n");
	System_flush();

	System_printf( "ADC Sampling Complete!\nData:\n");
	System_flush();

	System_printf( "Time (milliseconds), Voltage (uV), Signal Polarity\n");
	System_flush();

	uint16_t j;
	uint32_t time[10] = {0};
	uint32_t uVolts[10] = {0};
	uint8_t pol_bit[10] = {0};
	for (i = 0; i < SAMPLECOUNT; i=i+10)
	{
		for (j = 0; j < 10; j++)
		{
			time[j] = measurement_times[i+j] - measurement_times[0];
			uVolts[j] = adjusted_ADC_samples_uVolts[i+j];
			pol_bit[j] = polarity_samples[i+j];
		}
		System_printf("%i, %i, %i\n%i, %i, %i\n%i, %i, %i\n%i, %i, %i\n%i, %i, %i\n%i, %i, %i\n%i, %i, %i\n%i, %i, %i\n%i, %i, %i\n%i, %i, %i\n", time[0], uVolts[0], pol_bit[0], time[1], uVolts[1], pol_bit[1], time[2], uVolts[2], pol_bit[2], time[3], uVolts[3], pol_bit[3], time[4], uVolts[4], pol_bit[4], time[5], uVolts[5], pol_bit[5], time[6], uVolts[6], pol_bit[6], time[7], uVolts[7], pol_bit[7], time[8], uVolts[8], pol_bit[8], time[9], uVolts[9], pol_bit[9]);
		System_flush();
	}
	PIN_setOutputValue(pinHandle, Board_LED2, 0);
}

void ADCTaskFunc(UArg arg0, UArg arg1)
{
	Semaphore_pend(hADCSem, BIOS_WAIT_FOREVER); //stall further execution until semaphore is posted from button press
	sampleADC();
}

void pinInterruptHandler(PIN_Handle handle, PIN_Id pinId)
{
	Semaphore_post(hADCSem);
}

void setDACVoltage(uint16_t DAC_set_value)
{
	SPI_Handle      DAC_spiHandle;
	SPI_Params      DAC_spiParams;
	SPI_Transaction DAC_spiTransaction;

	// Init SPI and specify non-default parameters
	SPI_Params_init(&DAC_spiParams);
	DAC_spiParams.bitRate     = 1000000;
	DAC_spiParams.dataSize	   = 16;
	DAC_spiParams.frameFormat = SPI_POL0_PHA1;
	DAC_spiParams.mode        = SPI_MASTER;

	uint16_t txBuf[1] = {DAC_set_value};    // Transmit buffer

	// Configure the transaction
	DAC_spiTransaction.count = sizeof(txBuf);
	DAC_spiTransaction.txBuf = txBuf;
	DAC_spiTransaction.rxBuf = NULL;

	// Open the SPI and perform transfer to the first slave
	DAC_spiHandle = SPI_open(Board_SPI0, &DAC_spiParams);
	if (!DAC_spiHandle)
	{
		System_printf("SPI did not open\n");
		System_flush();
	}

	PIN_Id csnPin  = PIN_ID(GS_SPI0_DAC_CSN);
	int result = SPI_control(DAC_spiHandle, SPICC26XXDMA_SET_CSN_PIN, &csnPin);
	if (result == -1)
	{
		System_printf("Reset of CSN pin failed\n");
		System_flush();
	}

	result = SPI_transfer(DAC_spiHandle, &DAC_spiTransaction);
	if (!result)
	{
		System_printf("Unsuccessful SPI transfer. Error %i\n", DAC_spiTransaction.status);
		System_flush();
	}

	SPI_close(DAC_spiHandle);
}


/*
 *  ======== main ========
 */
int main(void)
{
	PIN_init(BoardGpioInitTable);

	System_printf("Board GPIO's initialized\n");
	System_flush();

	/* Open pins */
	pinHandle = PIN_open(&pinState, BoardGpioInitTable);
	if(!pinHandle) {
		System_abort("Error initializing board pins\n");
	}

	SPI_init();
	System_printf("Board SPI initialized\n");
	System_flush();

	PIN_registerIntCb(pinHandle, pinInterruptHandler);
	PIN_setInterrupt(pinHandle, Board_KEY_SELECT | PIN_IRQ_NEGEDGE);

	/* Set up the LED task */
	Task_Params workTaskParams;
	Task_Params_init(&workTaskParams);
	workTaskParams.stackSize = 256;
	workTaskParams.priority = 1;
	workTaskParams.stack = &workTaskStack;
	Task_construct(&workTask, workTaskFunc, &workTaskParams, NULL);

	Task_Params ADCTaskParams;
	Task_Params_init(&ADCTaskParams);
	ADCTaskParams.stackSize = 512;
	ADCTaskParams.priority = 2;
	ADCTaskParams.stack = &ADCTaskStack;
	Task_construct(&ADCTask, ADCTaskFunc, &ADCTaskParams, NULL);

	Semaphore_Params sADCParams;
	Semaphore_Params_init(&sADCParams);
	sADCParams.mode = Semaphore_Mode_BINARY;
	Semaphore_construct(&ADCSem, 0, &sADCParams);
	hADCSem = Semaphore_handle(&ADCSem);

	// Construct a semaphore for pending while acquiring ADC sample
	Semaphore_Params sADCDoneParams;
	Semaphore_Params_init(&sADCDoneParams);
	sADCDoneParams.mode = Semaphore_Mode_BINARY;
	Semaphore_construct(&ADCDoneSem, 0, &sADCDoneParams);
	hADCDoneSem = Semaphore_handle(&ADCDoneSem);

	Hwi_Struct hwi;
	Hwi_Params hwiParams;
	Hwi_Params_init(&hwiParams);
	hwiParams.enableInt = true;
	Hwi_construct(&hwi, INT_AUX_ADC_IRQ, adcIsr, &hwiParams, NULL);

	/* We want to sleep for 10000 microseconds */
    sleepTickCount = 10000 / Clock_tickPeriod;

	/* Start kernel. */
	BIOS_start();

	return (0);
}

