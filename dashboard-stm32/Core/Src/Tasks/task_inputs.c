/*
 * task_inputs.c
 *  Task_ReadInputs - 20 ms
 *  Legge pulsanti fisici e shift register, poi processa le azioni.
 */

#include "Tasks.h"
#include "Communication/serial.h"
#include "shift_register.h"

/* Macro locale per settare un errore nel car_state */
#define SET_ERROR(e)  do { car_state.error_flag = true; car_state.error = (e); } while(0)

/* =========================================================================
 * Apply_SR_Position
 * Mappa la posizione fisica (1-8) sulle variabili car_state.
 *   1-3  -> mappa motore (ECO / NORM / GAS)
 *   4-7  -> selezione parametro dinamico (REGEN / SRC / OS / TVC)
 *   0, 8 -> neutro, nessuna azione
 *  -1    -> errore hardware (cortocircuito tra posizioni)
 * ========================================================================= */
static void Apply_SR_Position(int8_t pos)
{
    if (pos == -1) {
        if (!car_state.error_flag)
            SET_ERROR(ERR_SR_HARDWARE);
        return;
    }

    if (pos >= 1 && pos <= 3) {
        car_state.SR_map = pos;
        car_state.selected_setting = 0;
    } else if (pos >= 4 && pos <= 7) {
        car_state.selected_setting = pos - 3;
        if (car_state.current_page != 4) {
            car_state.current_page = 4;
            Nextion_Cmd("page page4");
        }
    }
    /* pos == 0 o pos == 8: nessuna azione */
}

/* =========================================================================
 * Read_Physical_Buttons
 * Legge GPIO con debounce a cooldown (BTN_COOLDOWN_MS).
 * Setta .just_pressed = true solo al fronte di salita.
 * ========================================================================= */
static void Read_Physical_Buttons(void)
{
    uint32_t now = HAL_GetTick();

    bool raw_r2d  = (HAL_GPIO_ReadPin(R2D_btn_GPIO_Port,  R2D_btn_Pin)  == GPIO_PIN_RESET);
    bool raw_page = (HAL_GPIO_ReadPin(PAGE_btn_GPIO_Port, PAGE_btn_Pin) == GPIO_PIN_RESET);

    /* Pulsante R2D */
    if (raw_r2d && !car_state.btn_1.pressed 
        &&   (now - car_state.btn_1.last_pressed_time > BTN_COOLDOWN_MS)) {
        car_state.btn_1.pressed           = true;
        car_state.btn_1.last_pressed_time = now;
        car_state.btn_1.just_pressed      = true;
    } else if (!raw_r2d) {
        car_state.btn_1.pressed = false;
    }

    /* Pulsante PAGE */
    if (raw_page && !car_state.btn_2.pressed &&
        (now - car_state.btn_2.last_pressed_time > BTN_COOLDOWN_MS)) {
        car_state.btn_2.pressed           = true;
        car_state.btn_2.last_pressed_time = now;
        car_state.btn_2.just_pressed      = true;
    } else if (!raw_page) {
        car_state.btn_2.pressed = false;
    }
}

/* =========================================================================
 * Read_SR
 * Aggiorna il debounce interno dello shift register e applica la posizione
 * se è cambiata.
 * ========================================================================= */
static void Read_SR(void)
{
    SR_ProcessPeriodic();

#ifdef SR_DEBUG_PRINT
    /* Stampa il valore grezzo ogni ~500 ms per non saturare la seriale */
    static uint32_t sr_last_print_time = 0;
    uint32_t now_dbg = HAL_GetTick();
    if ((now_dbg - sr_last_print_time) >= 100U) {
        sr_last_print_time = now_dbg;
        uint8_t  raw = SR_GetRawValue();
        int8_t   pos = SR_GetStablePosition();
        /* Converte il byte in stringa binaria (MSB a sinistra) */
        char bin_str[9];
        for (int _b = 7; _b >= 0; _b--) {
            bin_str[7 - _b] = ((raw >> _b) & 1) ? '1' : '0';
        }
        bin_str[8] = '\0';
        Display_Message(&huart2, "=====================================\r\n");
        Display_Message(&huart2,
            "[SR] RAW=0x%02X (b%s) | POS=%d\r\n",
            raw, bin_str, (int)pos);
        Display_Message(&huart2, "=====================================\r\n");
    }
#endif /* SR_DEBUG_PRINT */

    if (SR_HasChanged()) {
        car_state.SR_raw_position = SR_GetStablePosition();
        Apply_SR_Position(SR_GetStablePosition());
    }
}

/* =========================================================================
 * Process_Input_Actions
 * - Hold entrambi i tasti > 2 s -> system_reset()
 * - Tasto R2D -> toggle r2d (con condizioni di sicurezza)
 * - Tasto PAGE -> cambia pagina O cicla il parametro selezionato (1->2->3->4->1)
 * ========================================================================= */
static void Process_Input_Actions(void)
{
    static uint32_t dual_press_start = 0;
    static bool     dual_press_active = false;

    /* --- COMBO: entrambi i tasti premuti -> reset dopo 2 s --- */
    if (car_state.btn_1.pressed && car_state.btn_2.pressed) {
        if (!dual_press_active) {
            dual_press_active = true;
            dual_press_start  = HAL_GetTick();
        } else if ((HAL_GetTick() - dual_press_start) > 2000) {
            system_reset();
        }
        /* Blocca just_pressed per evitare azioni singole al rilascio */
        car_state.btn_1.just_pressed = false;
        car_state.btn_2.just_pressed = false;
        return;
    } else {
        dual_press_active = false;
    }

    /* --- btn_1: MENO su page4 con parametro selezionato, altrimenti R2D --- */
    if (car_state.btn_1.just_pressed) {
        if (car_state.current_page == 4 && car_state.selected_setting != 0) {
            /* Pagina Settings: btn_1 = MENO (valore minimo 1) */
            switch (car_state.selected_setting) {
                case 1: if (car_state.val_regen > 1) car_state.val_regen--; break;
                case 2: if (car_state.val_src   > 1) car_state.val_src--;   break;
                case 3: if (car_state.val_os    > 1) car_state.val_os--;    break;
                case 4: if (car_state.val_tvc   > 1) car_state.val_tvc--;   break;
                default: break;
            }
        } else {
            /* Tutte le altre pagine: btn_1 = R2D toggle */
            if (car_state.r2d) {
                car_state.r2d = false;
            } else if (car_state.mcu.brake_pressure_front > 5 &&
                       car_state.mcu.car_speed < 2 &&
                       car_state.bms.sdc_state) {
                car_state.r2d = true;
            }
            //TODO: Decommentare la condizione di sicurezza per R2D 
            else{
                car_state.r2d = true;
            }
        }
        car_state.btn_1.just_pressed = false;
    }
    /* Forza R2D off se SDC non è attivo */
    if (!car_state.bms.sdc_state)
        car_state.r2d = false;

    /* --- PAGE: cambia pagina o cicla parametro selezionato --- */
    if (car_state.btn_2.just_pressed) {
        if (car_state.selected_setting == 0) {
            car_state.current_page++;
            if (car_state.current_page > 4) car_state.current_page = 1;
            Nextion_Cmd("page page%d", car_state.current_page);
        } else {
            switch (car_state.selected_setting) {
                case 1: car_state.val_regen = (car_state.val_regen % 4) + 1; break;
                case 2: car_state.val_src   = (car_state.val_src   % 4) + 1; break;
                case 3: car_state.val_os    = (car_state.val_os    % 4) + 1; break;
                case 4: car_state.val_tvc   = (car_state.val_tvc   % 4) + 1; break;
                default: break;
            }
        }
        car_state.btn_2.just_pressed = false;
    }
}

/* =========================================================================
 * Task_ReadInputs  [20 ms]
 * ========================================================================= */
void Task_ReadInputs(void)
{
    Read_Physical_Buttons();
#ifdef USE_SR
    Read_SR();
#endif
    Process_Input_Actions();
}
