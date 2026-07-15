/*
 * Tasks.h
 *
 *  Created on: 19 giu 2026
 *      Author: Utente
 */

#ifndef TASKS_H_
#define TASKS_H_

#include "car_data.h"
#include "Communication/can.h"
#include "Scheduler.h"

extern CarData_t car_state;

/* Inizializzazione */
void TaskInit(void);

/* Task periodici chiamati dallo Scheduler */
void Task_ReadInputs(void);    /* 20 ms  – pulsanti, SR, manettino */
void Task_SendCAN(void);       /* 50 ms  – trasmissione frame verso la macchina */
void Task_UpdateDisplay(void); /* 100 ms – aggiornamento display Nextion */

void system_reset(void) ;

/* Task di monitoring: chiamato all'interno di Task_UpdateDisplay */
bool Task_CheckCANTimeouts(void);

/* Task di monitoring: chiamato all'interno di Task_UpdateDisplay e aggiorna lo stato degli errori */
void Task_Check_Errors(void);

#endif /* TASKS_H_ */
