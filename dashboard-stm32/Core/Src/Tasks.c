/* Tasks.c  -  Inizializzazione e reset di sistema. */
/* Le task periodiche: Core/Src/Tasks/ */

#include "Tasks.h"
#include "Communication/serial.h"
#include "shift_register.h"

CarData_t car_state = {0};

void TaskInit(void)
{
    CanInit();
    SR_Init();
    car_state.val_regen    = 1;
    car_state.val_src      = 1;
    car_state.val_tvc      = 1;
    car_state.val_event    = 1;
    car_state.current_page = 1;\
    Nextion_Cmd("page page1");
    int8_t pos = SR_GetStablePosition();
    if (pos >= 1 && pos <= 3) { car_state.SR_map = (uint8_t)pos; }
}

void system_reset(void)
{
    Nextion_Cmd("rest");
    HAL_Delay(100);
    __disable_irq();
    NVIC_SystemReset();
}
