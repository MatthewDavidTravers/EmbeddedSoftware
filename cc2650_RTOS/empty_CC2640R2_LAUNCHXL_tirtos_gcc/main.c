/*
 * main.c
 *
 *  Created on: 25 Mar 2019
 *      Author: matthewtravers
 */


#include <stdint.h>

/* POSIX Header files */
#include <pthread.h>

/* RTOS header files */
#include <ti/sysbios/BIOS.h>

/* Example/Board Header files */
#include "Board.h"

/* Task 01 header */
#include "task_01.h"


/* Stack size in bytes */
#define THREADSTACKSIZE    1024

/*
 *  ======== main ========
 */
int main(void)
{

    /* Call driver init functions */
    Board_init();

    /* Create Task 01 */
    createTask01();

    /* Start the RTOS up*/
    BIOS_start();

    /* Should never reach here */
    return (0);
}
