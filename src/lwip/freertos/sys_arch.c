/*
 *
 *    Copyright (c) 2015 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    This document is the property of Nest. It is considered
 *    confidential and proprietary information.
 *
 *    This document may not be reproduced or transmitted in any form,
 *    in whole or in part, without the express written permission of
 *    Nest.
 *
 *    Description:
 *      LwIP sys_arch definitions for use with FreeRTOS.
 *
 */

#include <stdlib.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#include "arch/sys_arch.h"
#include "lwip/opt.h"
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/stats.h"

static portSTACK_TYPE gTCPIPThreadStack[TCPIP_THREAD_STACKSIZE];

static StaticQueue_t gTCPIPMsgQueue;
static uint8_t gTCPIPMsgQueueStorage[SYS_MESG_QUEUE_LENGTH * sizeof(void*)];

void sys_init(void)
{
    // nothing to do.
}

err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
    *sem = xSemaphoreCreateBinary();
    if (*sem != NULL)
    {
        if (count != 0)
        {
            xSemaphoreGive(*sem);
        }
        SYS_STATS_INC_USED(sem);
        return ERR_OK;
    }
    else
    {
        SYS_STATS_INC(sem.err);
        return ERR_MEM;
    }
}


void sys_sem_free(sys_sem_t *sem)
{
    vSemaphoreDelete(*sem);
    SYS_STATS_DEC(sem);
}

void sys_sem_signal(sys_sem_t *sem)
{
    xSemaphoreGive(*sem);
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
    TickType_t timeoutTicks, startTime, elapsedTime;
    BaseType_t res;

    if (timeout == 0)
        timeoutTicks = portMAX_DELAY;
    else
        timeoutTicks = (TickType_t)(timeout / portTICK_PERIOD_MS);

    startTime = xTaskGetTickCount();

    do
    {
        res = xSemaphoreTake(*sem, timeoutTicks);
    } while (res != pdTRUE && timeout == 0);

    if (res == pdTRUE)
    {
        elapsedTime = (xTaskGetTickCount() - startTime) * portTICK_PERIOD_MS;
        if (elapsedTime == 0)
            elapsedTime = 1;
        return (u32_t)elapsedTime;
    }
    else
        return SYS_ARCH_TIMEOUT;
}

err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
    if (size != SYS_MESG_QUEUE_LENGTH)
    {
        SYS_STATS_INC(mbox.err);
        return ERR_MEM;
    }
    *mbox = xQueueCreateStatic((UBaseType_t)size, (UBaseType_t) sizeof(void *), gTCPIPMsgQueueStorage, &gTCPIPMsgQueue);
    if (*mbox != NULL)
    {
        SYS_STATS_INC_USED(mbox);
        return ERR_OK;
    }
    else
    {
        SYS_STATS_INC(mbox.err);
        return ERR_MEM;
    }
}

void sys_mbox_free(sys_mbox_t *mbox)
{
    vQueueDelete(*mbox);
}

void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
    BaseType_t res;
    res = xQueueSendToBack(*mbox, &msg, (TickType_t) (SYS_POST_BLOCK_TIME_MS / portTICK_RATE_MS));
    LWIP_ASSERT("Error posting to LwIP mbox", res == pdTRUE);
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
    TickType_t timeoutTicks, startTime, elapsedTime;
    BaseType_t res;
    void *dummy;

    if (msg == NULL)
        msg = &dummy;

    if (timeout == 0)
        timeoutTicks = portMAX_DELAY;
    else
        timeoutTicks = (TickType_t)(timeout / portTICK_PERIOD_MS);

    startTime = xTaskGetTickCount();

    do
    {
        res = xQueueReceive(*mbox, (void *)msg, timeoutTicks);
    } while (res != pdTRUE && timeout == 0);

    if (res == pdTRUE)
    {
        elapsedTime = (xTaskGetTickCount() - startTime) * portTICK_PERIOD_MS;
        if (elapsedTime == 0)
            elapsedTime = 1;
        return (u32_t)elapsedTime;
    }
    else
    {
        *msg = NULL;
        return SYS_ARCH_TIMEOUT;
    }
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
    BaseType_t res;
    void *dummy;

    if (msg == NULL)
        msg = &dummy;

    res = xQueueReceive(*mbox, (void *)msg, 0);

    return (res == pdTRUE) ? 0 : SYS_MBOX_EMPTY;
}

err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
    BaseType_t res;

    res = xQueueSendToBack(*mbox, &msg, 0);

    if (res == pdTRUE)
        return ERR_OK;
    else
    {
        SYS_STATS_INC(mbox.err);
        return ERR_MEM;
    }
}

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
    BaseType_t res;
    TaskHandle_t taskH;

    if (strcmp(name, TCPIP_THREAD_NAME) != 0 || stacksize != TCPIP_THREAD_STACKSIZE)
        return NULL;

    res = xTaskGenericCreate(thread, name, (unsigned short) stacksize, arg, (UBaseType_t) prio, &taskH, (StackType_t *)gTCPIPThreadStack, NULL);
    if (res == pdPASS)
        return taskH;
    else
        return NULL;
}

sys_prot_t sys_arch_protect(void)
{
    taskENTER_CRITICAL();
    return 1;
}

void sys_arch_unprotect(sys_prot_t pval)
{
    taskEXIT_CRITICAL();
}

