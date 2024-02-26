/*
 * Copyright (c) 2015-2020, Texas Instruments Incorporated
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
 *  ======== gpiointerrupt.c ========
 */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>

/* Driver configuration */
#include "ti_drivers_config.h"

/* Timer */
#include <ti/drivers/Timer.h>

/* I2C */
#include <ti/drivers/I2C.h>

/* UART2 */
#include <ti/drivers/UART2.h>
#define DISPLAY(x) UART2_write(uart, &output, x, &bytesWritten);

/*
 *  ======== UART2 ========
 *  Code provided by the lab
 */
// UART Global Variables
char output[64];
size_t bytesWritten = 0;

// Driver Handles - Global variables
UART2_Handle uart;

void initUART(void)
{
    UART2_Params uartParams;

    // Configure the driver
    UART2_Params_init(&uartParams);
    uartParams.writeMode = UART2_Mode_BLOCKING;
    uartParams.readMode = UART2_Mode_BLOCKING;
    uartParams.readReturnMode = UART2_ReadReturnMode_FULL;
    uartParams.baudRate = 115200;

    // Open the driver
    uart = UART2_open(CONFIG_UART2_0, &uartParams);

    if (uart == NULL) {
        /* UART_open() failed */
        while (1);
    }
}

/*
 * ======== Timer ========
 * Code provided by the lab
 */
// Driver Handles - Global variables
Timer_Handle timer0;

volatile unsigned char TimerFlag = 0;

// Variables for measuring temp of room, set temp of thermostat, if heat is on, and seconds since board reset
int temperature = 0;
int setPoint = 0;
char heat = ' ';
float seconds = 0;

void timerCallback(Timer_Handle myHandle, int_fast16_t status)
{
    TimerFlag = 1;
    seconds += 0.1; // Increase decimal count of seconds since board was reset
}

void initTimer(void)
{
    Timer_Params params;

    // Init the driver
    Timer_init();

    // Configure the driver
    Timer_Params_init(&params);
    params.period = 100000; // Set to 100ms in order to check button every 200ms and temp every 500ms
    params.periodUnits = Timer_PERIOD_US;
    params.timerMode = Timer_CONTINUOUS_CALLBACK;
    params.timerCallback = timerCallback;

    // Open the driver
    timer0 = Timer_open(CONFIG_TIMER_0, &params);

    if (timer0 == NULL) {
        /* Failed to initialized timer */
        while (1) {}
    }

    if (Timer_start(timer0) == Timer_STATUS_ERROR) {
        /* Failed to start timer */
        while (1) {}
    }
}

/*
 *    ======== I2C ========
 *    Code provided by the lab
 */
// I2C Global Variables
static const struct {
    uint8_t address;
    uint8_t resultReg;
    char *id;
} sensors[3] = {
                { 0x48, 0x0000, "11X" },
                { 0x49, 0x0000, "116" },
                { 0x41, 0x0001, "006" }
};
uint8_t     txBuffer[1];
uint8_t     rxBuffer[2];
I2C_Transaction     i2cTransaction;

// Driver Handles - Global variables
I2C_Handle  i2c;

// Make sure you call initUART() before calling this function.
void initI2C(void)
{
    int8_t      i, found;
    I2C_Params  i2cParams;

    DISPLAY(snprintf(output, 64, "Initializing I2C Driver - "))

    // Init the driver
    I2C_init();

    // Configure the driver
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;

    // Open the driver
    i2c = I2C_open(CONFIG_I2C_0, &i2cParams);
    if (i2c == NULL)
    {
        DISPLAY(snprintf(output, 64, "Failed\n\r"))
        while (1);
    }

    DISPLAY(snprintf(output, 32, "Passed\n\r"))

    // Boards were shipped with different sensors.
    // Welcome to the world of embedded systems.
    // Try to determine which sensor we have.
    // Scan through the possible sensor addresses

    /* Common I2C transaction setup */
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = rxBuffer;
    i2cTransaction.readCount = 0;

    found = false;
    for (i=0; i<3; ++i)
    {
        i2cTransaction.targetAddress = sensors[i].address;
        txBuffer[0] = sensors[i].resultReg;

        DISPLAY(snprintf(output, 64, "Is this %s? ", sensors[i].id))
        if (I2C_transfer(i2c, &i2cTransaction))
        {
            DISPLAY(snprintf(output, 64, "Found\n\r"))
        found = true;
            break;
        }
        DISPLAY(snprintf(output, 64, "No\n\r"))
    }

    if(found)
    {
        DISPLAY(snprintf(output, 64, "Detected TMP%s I2C address: "
                "%x\n\r", sensors[i].id, i2cTransaction.targetAddress ))
    }
    else
    {
        DISPLAY(snprintf(output, 64, "Temperature sensor not found, "
                "contact professor\n\r"))
    }
}

int16_t readTemp(void)
{
    int j;
    int16_t temperature = 0;
    i2cTransaction.readCount = 2;

    if (I2C_transfer(i2c, &i2cTransaction))
    {
        /*
         * Extract degrees C from the received data;
         * see TMP sensor datasheet
         */
        temperature = (rxBuffer[0] << 8) | (rxBuffer[1]);
        temperature *= 0.0078125;

        /*
         * If the MSB is set '1', then we have a 2's complement
         * negative value which needs to be sign extended
         */
        if (rxBuffer[0] & 0x80)
        {
            temperature |= 0xF000;
        }
    }
    else
    {
        DISPLAY(snprintf(output, 64, "Error reading temperature sensor (%d)\n\r",i2cTransaction.status))
                         DISPLAY(snprintf(output, 64, "Please power cycle your board by "
                                 "unplugging USB and plugging back in.\n\r"))
    }

    return temperature;
}

/*
 *  ======== gpioButtonFxn0 ========
 *  Callback function for the GPIO interrupt on CONFIG_GPIO_BUTTON_0.
 *
 *  Note: GPIO interrupts are cleared prior to invoking callbacks.
 */
void gpioButtonFxn0(uint_least8_t index) // Increases set temperature by the thermostate
{
    /* Increase setPoint variable by 1 */
    setPoint+=1;
}

/*
 *  ======== gpioButtonFxn1 ========
 *  Callback function for the GPIO interrupt on CONFIG_GPIO_BUTTON_1.
 *  This may not be used for all boards.
 *
 *  Note: GPIO interrupts are cleared prior to invoking callbacks.
 */
void gpioButtonFxn1(uint_least8_t index) // Decreases set temperature by the thermostate
{
    /* Decrease setPoint variable by 1 */
    setPoint-=1;
}

/* Thermostat State Machine states */
enum Thermo_States {Thermo_Init, Thermo_On, Thermo_Off} Thermo_State;
Thermo_State = Thermo_Init; // Set Thermostat state to Thermo_Init

void TickFct_Thermo() {
    switch (Thermo_State){ // Switch for handling thermostat transitions and actions
    case Thermo_Init:
    Thermo_State = Thermo_Off;
    break;

    case Thermo_On:
    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
    heat = '1';
    if (setPoint > temperature){
        Thermo_State = Thermo_Off;
    }
    break;

    case Thermo_Off:
    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
    heat = '0';
    if (setPoint < temperature){
        Thermo_State = Thermo_On;
    }
    break;

    default:
    Thermo_State = Thermo_Init;
    break;
    }
}

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    /* Call driver init functions */
    GPIO_init();
    /* Call timer init functions */
    initTimer();

    /* Configure the LED and button pins */
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_BUTTON_0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

    /* Install Button callback */
    GPIO_setCallback(CONFIG_GPIO_BUTTON_0, gpioButtonFxn0);

    /* Enable interrupts */
    GPIO_enableInt(CONFIG_GPIO_BUTTON_0);

    /*
     *  If more than one input pin is available for your device, interrupts
     *  will be enabled on CONFIG_GPIO_BUTTON1.
     */
    if (CONFIG_GPIO_BUTTON_0 != CONFIG_GPIO_BUTTON_1)
    {
        /* Configure BUTTON1 pin */
        GPIO_setConfig(CONFIG_GPIO_BUTTON_1, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

        /* Install Button callback */
        GPIO_setCallback(CONFIG_GPIO_BUTTON_1, gpioButtonFxn1);
        GPIO_enableInt(CONFIG_GPIO_BUTTON_1);
    }

    /* initialize UART2, I2C, and Timer */
    initUART(); // The UART must be initialized before calling initI2C()
    initI2C();
    initTimer();

    // Scheduler times to wait for
    unsigned long buttonWaitTime = 200000; // Check button flags every 200ms
    unsigned long tempWaitTime = 500000; // Read temperature and update the LED every 500ms
    unsigned long updateWaitTime = 1000000; // Output to UART every 1s

    // Scheduler events
    unsigned long checkButtonPress_elapsedTime = buttonWaitTime; // Variable for checking button wait time
    unsigned long readTemp_elapsedTime = tempWaitTime; // Variable for checking temp wait time
    unsigned long updateAndReport_elapsedTime = updateWaitTime; // Variable for checking output wait time

    // Timer period
    unsigned long timerPeriod = 100000; // 100 ms (same as timer period for timer driver)

    while (1) {
        /*
         * If the wait time is greater than the set time for checking the buttons,
         * then check the buttons and reset the wait time to 0
         */
        if (checkButtonPress_elapsedTime >= buttonWaitTime) {
            GPIO_enableInt(CONFIG_GPIO_BUTTON_0);
            GPIO_enableInt(CONFIG_GPIO_BUTTON_1);
            checkButtonPress_elapsedTime = 0;
        }

        /*
         * If the wait time is greater than the set time for reading the temp of the room,
         * then read the temp of the room and reset the wait time to 0
         */
        if (readTemp_elapsedTime >= tempWaitTime) {
            temperature = readTemp();
            readTemp_elapsedTime = 0;
        }

        /*
         * If the wait time is greater than the set time for updating the UART,
         * then update the UART and reset the wait time to 0
         */
        if (updateAndReport_elapsedTime  >= updateWaitTime) {

            // Display output to terminal
            DISPLAY(snprintf(output, 64, "<%02d,%02d,%d,%04d>\n\r", temperature, setPoint, heat, seconds));

            // Call TickFct_SwitchHeater
            TickFct_Thermo();
        }

        while (!TimerFlag){} // Wait for timer period
        TimerFlag = 0; // Lower flag raised by timer

        // Add timer period to wait times
        checkButtonPress_elapsedTime += timerPeriod;
        readTemp_elapsedTime  += timerPeriod;
        updateAndReport_elapsedTime  += timerPeriod;
    }

    return (NULL);
}
