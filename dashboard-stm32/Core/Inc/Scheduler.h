/*
 * Scheduler.h
 *
 *  Created on: 19 giu 2026
 *      Author: zinga
 */

#ifndef INC_SCHEDULER_H_
#define INC_SCHEDULER_H_

#include "main.h"

/* ==========================================
 * CONFIGURAZIONE INTERVALLI TASK (in ms)
 * ========================================== */
#define INTERVAL_READ_INPUTS    20   // Lettura pulsanti, SR e manettino
#define INTERVAL_SEND_CAN       50   // Invio parametri e mappa CAN
#define INTERVAL_UPDATE_DISP    100  // Refresh dati Display

/* ========================================== */

void SchedulerInit(void);
void SchedulerTimerInterruptCallBack(void);
void SchedulerManagementFunction(void);


#endif /* INC_SCHEDULER_H_ */
