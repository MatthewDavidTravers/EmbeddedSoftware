/*
 * task_01.c
 *
 *  Created on: 25 Mar 2019
 *      Author: matthewtravers
 */


#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <xdc/runtime/Error.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Mailbox.h>
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

/* Clock properties*/
#define LED_CLOCK_PERIOD 10000
Clock_Params PeriodicLEDClock_params;
Clock_Handle PeriodicLEDClock_handle;

/* Events */
#define MAILBOX_NOT_EMPTY_EVENT Event_Id_00
Event_Handle task01_Event;



/* Mailbox */
#define MAILBOX_DEPTH 5

typedef struct MsgObj {
    Int     id;
    Char    val;
} MsgObj;

typedef struct MailboxMsgObj {
    Mailbox_MbxElem  elem;      /* Mailbox header        */
    MsgObj           obj;       /* Application's mailbox */
} MailboxMsgObj;

MailboxMsgObj mailbox_buffer[MAILBOX_DEPTH];
Mailbox_Struct mailbox_struct;
Mailbox_Handle mailbox_handle;
Mailbox_Params mailbox_params;



/* Task Functions*/
void createTask01( void );
static void initTask01( void );
static void spinTask01( void );
static void task01(UArg a0, UArg a1);

/* Periodic Clock Callbacks*/
static void PeriodicLEDClock_task(UArg a0);



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
    /* Initialise the GPIO */
    GPIO_init();
    GPIO_setConfig(Board_GPIO_LED0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_write(Board_GPIO_LED0, Board_GPIO_LED_ON);

    /* Initialise the LED periodic clock*/
    Clock_Params_init(&PeriodicLEDClock_params);
    PeriodicLEDClock_params.period = LED_CLOCK_PERIOD;
    PeriodicLEDClock_params.startFlag = TRUE;
    PeriodicLEDClock_params.arg = (UArg)0x5555;
    PeriodicLEDClock_handle = Clock_create(PeriodicLEDClock_task, LED_CLOCK_PERIOD, &PeriodicLEDClock_params, &task_01_eb);
    if (PeriodicLEDClock_handle == NULL) {
        spinTask01();
    }

    /* Setup the Events */
    task01_Event = Event_create(NULL, &task_01_eb);
    if (task01_Event == NULL)
    {
        spinTask01();
    }


    /* Construct a Mailbox instance */
    Mailbox_Params_init(&mailbox_params);
    mailbox_params.buf = (Ptr)mailbox_buffer;
    mailbox_params.bufSize = sizeof(mailbox_buffer);
    mailbox_params.readerEvent = task01_Event;
    mailbox_params.readerEventId = MAILBOX_NOT_EMPTY_EVENT;
    Mailbox_construct(&mailbox_struct, sizeof(MsgObj), MAILBOX_DEPTH, &mailbox_params, NULL);
    mailbox_handle = Mailbox_handle(&mailbox_struct);

}


/* Task 01 */
static void task01(UArg a0, UArg a1)
{

    /* Initialise the task*/
    initTask01();

    UInt events;
    MsgObj msg;

    while (1)
    {
        /* Pend on OR'd events */
        events = Event_pend(task01_Event,
                            Event_Id_NONE,
                            MAILBOX_NOT_EMPTY_EVENT,
                            BIOS_WAIT_FOREVER);

        /* Check if a message was posted */
        if (events & MAILBOX_NOT_EMPTY_EVENT)
        {
            Mailbox_pend(mailbox_handle, &msg, BIOS_NO_WAIT);
            GPIO_toggle(Board_GPIO_LED0);
        }
    }
}


static void PeriodicLEDClock_task(UArg a0)
{
    MsgObj msg;

    msg.id = 0;
    msg.val = 0;
    Mailbox_post(mailbox_handle, &msg, BIOS_NO_WAIT);
}
