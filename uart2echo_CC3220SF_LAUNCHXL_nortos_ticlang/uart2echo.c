/*
 * Copyright (c) 2020, Texas Instruments Incorporated
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
 *  ======== uart2echo.c ========
 */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART2.h>

/* Driver configuration */
#include "ti_drivers_config.h"

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    char input;
    const char echoPrompt[] = "Echoing characters:\r\n";
    UART2_Handle uart;
    UART2_Params uartParams;
    size_t bytesRead;
    size_t bytesWritten = 0;
    uint32_t status = UART2_STATUS_SUCCESS;

    /* Call driver init functions */
    GPIO_init();

    /* Configure the LED pin */
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);

    /* Create a UART where the default read and write mode is BLOCKING */
    UART2_Params_init(&uartParams);
    uartParams.baudRate = 115200;

    uart = UART2_open(CONFIG_UART2_0, &uartParams);

    if (uart == NULL)
    {
        /* UART2_open() failed */
        while (1) {}
    }

    /* Turn on user LED to indicate successful initialization */
    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON); // Controls red LED on opposite side of board from usb port

    UART2_write(uart, echoPrompt, sizeof(echoPrompt), &bytesWritten);

    enum LED_States { LED_Idle, LED_On, LED_Off } LED_State; // Defines new data types for the LED state
    LED_State = LED_Idle; // Sets LED state to starting state

    /* Loop forever echoing */
    while (1)
    {
        bytesRead = 0;
        while (bytesRead == 0)
        {
            status = UART2_read(uart, &input, 1, &bytesRead);

            if (status != UART2_STATUS_SUCCESS)
            {
                /* UART2_read() failed */
                while (1) {}
            }
        }

        bytesWritten = 0;
        while (bytesWritten == 0)
        {
            status = UART2_write(uart, &input, 1, &bytesWritten); // Saves the last terminal input (ends when user hits enter) in variable input

            if (status != UART2_STATUS_SUCCESS)
            {
                /* UART2_write() failed */
                while (1) {}
            }
        }

        if (input == 'O'){ // Checks input for a capital O
            UART2_read(uart, &input, 1, &bytesRead); // Gets next input from user
            UART2_write(uart, &input, 1, &bytesWritten); // Displays next input from user
            if (input == 'N'){  // Checks input for a capital ON
                LED_State = LED_On; // Sets state of LED to LED_On
            }
            else if (input == 'F'){ // Checks input for a capital OF
                UART2_read(uart, &input, 1, &bytesRead); // Gets next input from user
                UART2_write(uart, &input, 1, &bytesWritten); // Displays next input from user
                if (input == 'F'){ // Checks input for a capital OFF
                    LED_State = LED_Off; // Sets state of LED to LED_Off
                }
            }
            else{
                LED_State = LED_Idle; // catch all that sets the LED state to idle
            }
        }

        switch (LED_State){ // Switch for handling state transitions and actions
        case LED_Idle:
            break;

        case LED_On:
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON); // Turns red LED on opposite side of board from usb port on
            LED_State = LED_Idle;
            break;

        case LED_Off:
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF); // Turns red LED on opposite side of board from usb port off
            LED_State = LED_Idle;
            break;

        defaul:
        LED_State = LED_Idle;
        break;
        }
    }
}
