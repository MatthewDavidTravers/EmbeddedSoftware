/*
 * task_01.c
 *
 *  Created on: 25 Mar 2019
 *      Author: matthewtravers
 */


#include <ti/sysbios/knl/Task.h>
#include <xdc/runtime/Error.h>
//#include <ti/sysbios/knl/Clock.h>
//#include <ti/sysbios/knl/Event.h>
//#include <ti/sysbios/knl/Queue.h>
//#include <ti/drivers/utils/List.h>

/* For usleep() */
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
// #include <ti/drivers/I2C.h>
// #include <ti/drivers/SPI.h>
// #include <ti/drivers/UART.h>
// #include <ti/drivers/Watchdog.h>

/* Board Header file */
#include "Board.h"


/* Task properties */
#define TASK_01_STACKSIZE 512
#define TASK_01_PRIORITY 1
Task_Params task_01_Params;
Task_Handle task_01_handle;
Error_Block task_01_eb;


/* Task Functions*/
void createTask01( void );
static void initTask01( void );
static void spinTask01( void );
static void task01(UArg a0, UArg a1);


/* Create Task Dynamically */
void createTask01( void )
{
    Error_init(&task_01_eb);

    /* Create task_01 with priority 1 */
    Task_Params_init(&task_01_Params);
    task_01_Params.stackSize = TASK_01_STACKSIZE;
    task_01_Params.priority = TASK_01_PRIORITY;
    task_01_handle = Task_create((Task_FuncPtr)task01, &task_01_Params, &task_01_eb);
    if (task_01_handle == NULL) {
        spinTask01();
    }
}


/* Spin Task 01 */
static void spinTask01( void )
{
    volatile uint8_t x = 0;;

    while(1)
    {
        x++;
    }
}


/* Init Task 01 */
static void initTask01( void )
{
    /* Call driver init functions */
    GPIO_init();
    // I2C_init();
    // SPI_init();
    // UART_init();
    // Watchdog_init();

    /* Configure the LED pin */
    GPIO_setConfig(Board_GPIO_LED0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
}


/* Task 01 */
static void task01(UArg a0, UArg a1)
{

    /* 1 second delay */
    uint32_t time = 1;

    /* Initialise the task*/
    initTask01();

    /* Turn on user LED */
    GPIO_write(Board_GPIO_LED0, Board_GPIO_LED_ON);

    while (1) {
        sleep(time);
        GPIO_toggle(Board_GPIO_LED0);
    }
}
