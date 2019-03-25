/*
 * task_01.c
 *
 *  Created on: 25 Mar 2019
 *      Author: matthewtravers
 */

#include "stdlib.h"
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <xdc/runtime/Error.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Mailbox.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>

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

#define BUTTON_DEBOUNCE_LENGTH 100
Clock_Params button0DebounceClock_params;
Clock_Handle button0DebounceClock_handle;
uint_fast8_t button0State;


/* Events */
#define APPLICATION_MESSAGE Event_Id_00
#define PERIODIC_LED_CLOCK_EVENT (1U << 0)
#define BUTTON_0_HWI_EVENT (1U << 1)


Event_Handle task01_Event;



/* Mailbox */
#define MAILBOX_DEPTH 5

typedef struct MsgObj {
    uint8_t event;
    void    *pData;
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

/* Button callbacks */
static void gpioButton0Callback(uint_least8_t index);
static void gpioButton0DebounceCallback(UArg a0);

/* Mailbox functions */
static void enqueMessage(uint8_t event, void *pData);



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
    GPIO_setConfig(Board_GPIO_LED1, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_write(Board_GPIO_LED1, Board_GPIO_LED_OFF);

    /* Set callback for hardware interrupt */
    GPIO_setConfig(Board_GPIO_BUTTON0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);
    GPIO_setCallback(Board_GPIO_BUTTON0, gpioButton0Callback);
    GPIO_enableInt(Board_GPIO_BUTTON0);

    /* Initialise the LED periodic clock*/
    Clock_Params_init(&PeriodicLEDClock_params);
    PeriodicLEDClock_params.period = LED_CLOCK_PERIOD;
    PeriodicLEDClock_params.startFlag = TRUE;
    //PeriodicLEDClock_params.arg = (UArg)0x5555;
    PeriodicLEDClock_handle = Clock_create(PeriodicLEDClock_task, LED_CLOCK_PERIOD, &PeriodicLEDClock_params, &task_01_eb);
    if (PeriodicLEDClock_handle == NULL) {
        spinTask01();
    }

    /* Initialise the button 0 debounce one-shot clock */
    Clock_Params_init(&button0DebounceClock_params);
    button0DebounceClock_params.period = 0;
    button0DebounceClock_params.startFlag = FALSE;
    button0DebounceClock_handle = Clock_create(gpioButton0DebounceCallback, BUTTON_DEBOUNCE_LENGTH, &button0DebounceClock_params, &task_01_eb);
    if (button0DebounceClock_handle == NULL) {
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
    //mailbox_params.readerEvent = task01_Event;
    //mailbox_params.readerEventId = APPLICATION_MESSAGE;
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
                            APPLICATION_MESSAGE,
                            BIOS_WAIT_FOREVER);

        /* Check if a message was posted */
        if (events & APPLICATION_MESSAGE)
        {
            /* Retrieve the message from the maibox */
            Mailbox_pend(mailbox_handle, &msg, BIOS_NO_WAIT);

            /* Determine the type of message */
            switch(msg.event)
            {
                case PERIODIC_LED_CLOCK_EVENT:
                {
                    GPIO_toggle(Board_GPIO_LED0);
                    break;
                }
                case BUTTON_0_HWI_EVENT:
                {
                    GPIO_toggle(Board_GPIO_LED1);
                    break;
                }
                default:
                    break;
            }

            /* Free up the message data memory */
            if(msg.pData != NULL)
            {
                free(msg.pData);
            }
        }
    }
}


static void PeriodicLEDClock_task(UArg a0)
{
    enqueMessage(PERIODIC_LED_CLOCK_EVENT, NULL);
}

static void gpioButton0DebounceCallback(UArg a0)
{
    if(GPIO_read(Board_GPIO_BUTTON0) == button0State)
    {
        enqueMessage(BUTTON_0_HWI_EVENT, NULL);
    }

    GPIO_enableInt(Board_GPIO_BUTTON0);

}

static void gpioButton0Callback(uint_least8_t index)
{
    GPIO_disableInt(Board_GPIO_BUTTON0);
    button0State = GPIO_read(Board_GPIO_BUTTON0);
    Clock_start(button0DebounceClock_handle);
}

static void enqueMessage(uint8_t event, void *pData)
{
    MsgObj msg;

    msg.event = event;
    msg.pData = pData;
    Mailbox_post(mailbox_handle, &msg, BIOS_NO_WAIT);
    Event_post(task01_Event, APPLICATION_MESSAGE); // only doing this as i cannot set the semaphore internal event in the cfg
}
