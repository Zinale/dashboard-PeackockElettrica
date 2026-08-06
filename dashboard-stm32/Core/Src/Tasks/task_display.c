/*
 * task_display.c
 * Task_UpdateDisplay - 100 ms
 * Aggiorna il display Nextion con i dati di telemetria.
 *
 * Strategia:
 * - Errori: latch con timer (l'errore resta a schermo almeno ERROR_DISPLAY_HOLD_MS ms)
 * - Grafica (colori, sfondi crop, icone): edge-detection → inviata solo se mappa o pagina cambiano
 * - Dati numerici: inviati ogni ciclo (100 ms) solo per la pagina attiva
 */

#include "Tasks.h"
#include "Communication/serial.h"
#include "stdlib.h"   /* abs() */

#define ERROR_DISPLAY_HOLD_MS  2000
#define MAX_STEERING_ANGLE 120
#define MIN_MODULE_VOLTAGE 60.0f
#define MAX_MODULE_VOLTAGE 105.0f


/* =========================================================================
 * HELPER GRAFICI (Colori e Sfondi)
 * ========================================================================= */
static void get_map_colors(uint8_t map, uint16_t *bco, uint16_t *pco)
{
    switch (map) {
        case 1:  *bco = 2016;  *pco = 416;   break; /* ECO  */
        case 2:  *bco = 1055;  *pco = 13;    break; /* NORM */
        case 3:  *bco = 63488; *pco = 20480; break; /* GAS  */
        default: *bco = 0;     *pco = 0;     break;
    }
}

/* Calcola l'ID dell'immagine di sfondo basandosi su Mappa(1-3) e Pagina(1-4) */
static uint8_t get_bg_image_id(uint8_t map, uint8_t page)
{
    uint8_t base_id = 0;
    switch (map) {
        case 1:  base_id = 0; break; /* ECO  (0, 1, 2, 3) */
        case 2:  base_id = 8; break; /* NORM (8, 9, 10, 11) */
        case 3:  base_id = 4; break; /* GAS  (4, 5, 6, 7) */
        default: base_id = 0; break;
    }
    return base_id + (page - 1);
}

/* =========================================================================
 * Aggiornamento testo errore su Nextion
 * ========================================================================= */
static void Update_Error_Display(DashError_t err)
{
    static DashError_t last_sent  = ERR_NONE;
    static uint16_t    last_subcode = 0;

    uint16_t subcode = 0;
    if      (err == ERR_MCU_INV1_FAULT) subcode = car_state.mcu.inv1_error_num;
    else if (err == ERR_MCU_INV2_FAULT) subcode = car_state.mcu.inv2_error_num;

    if (err == last_sent && subcode == last_subcode) return;

    if (err == ERR_NONE) {
        Nextion_Cmd("error.txt=\"---\"");
    } else if (err == ERR_MCU_INV1_FAULT) {
        Nextion_Cmd("error.txt=\"L%d\"", subcode);
    } else if (err == ERR_MCU_INV2_FAULT) {
        Nextion_Cmd("error.txt=\"R%d\"", subcode);
    } else {
        Nextion_Cmd("error.txt=\"%d\"", err);
    }

    last_sent    = err;
    last_subcode = subcode;
}

/* =========================================================================
 * PAGINE
 * ========================================================================= */
static void Draw_Page1(bool r2d, bool sdc, bool force)
{
    static bool last_r2d = false;
    static bool last_sdc = false;

    // Aggiornamento BACKGROUND e CROP IMAGE (Solo se Mappa o Pagina cambiano)
    if (force) {
        uint8_t bg_id = get_bg_image_id(car_state.SR_map, 1);
        Nextion_Cmd("page1.pic=%d", bg_id); // Aggiorna attributo sfondo

        static const char* crop_objs[] = {
            "speed", "soc", "t_pack", "t_inv", "t_motor", "lvbatt_volt", "error"
        };
        for (size_t i = 0; i < (sizeof(crop_objs)/sizeof(crop_objs[0])); i++) {
            Nextion_Cmd("%s.picc=%d", crop_objs[i], bg_id); // Aggiorna il ritaglio per i testi
        }
        Nextion_Cmd("ref 0"); // Ridisegna tutti i componenti con i nuovi attributi
    }

    Nextion_Cmd("speed.txt=\"%d\"",   car_state.mcu.car_speed);
    Nextion_Cmd("soc.txt=\"%d\"",     car_state.bms.SoC_percent);

    int temp_pack_sum = 0;
    for (int i = 0; i < BMS_NUM_MODULES; i++)
        temp_pack_sum += car_state.bms.modules[i].temperature;
    Nextion_Cmd("t_pack.txt=\"%d\"",  temp_pack_sum / BMS_NUM_MODULES);

    Nextion_Cmd("t_inv.txt=\"%d\"",   (car_state.mcu.inv104_temp  + car_state.mcu.inv204_temp)  / 2);
    Nextion_Cmd("t_motor.txt=\"%d\"", (car_state.mcu.motor104_temp + car_state.mcu.motor204_temp) / 2);
    Nextion_Cmd("lvbatt_volt.txt=\"%.1f\"", car_state.pdm_vcu1.lv_battery_voltage);

    // Icone di Stato 
    if (r2d != last_r2d || force) {
        Nextion_Cmd("r2d_status.pic=%d", r2d ? 13 : 12);
        last_r2d = r2d;
    }
    if (sdc != last_sdc || force) {
        Nextion_Cmd("sdc_status.pic=%d", sdc ? 13 : 12);
        last_sdc = sdc;
    }

    /* SOC bar progressiva (10 blocchi) */
    uint8_t soc = car_state.bms.SoC_percent;
    for (int i = 1; i <= 10; i++) {
        int val = 0;
        if      (soc >= i * 10)       val = 100;
        else if (soc > (i - 1) * 10) val = (soc - (i - 1) * 10) * 10;
        Nextion_Cmd("soc_bar%d.val=%d", i, val);
    }
}

static void Draw_Page2(bool r2d, bool sdc, bool force, uint16_t bco, uint16_t pco)
{
    static bool last_r2d = false;
    static bool last_sdc = false;

    // Aggiornamento BACKGROUND e CROP IMAGE
    if (force) {
        uint8_t bg_id = get_bg_image_id(car_state.SR_map, 2);
        Nextion_Cmd("page2.pic=%d", bg_id);

        static const char* crop_objs[] = {
            "speed", "speed_fl", "speed_fr", "app1", "app2", "brake_bar", 
            "cool_temp", "susp_fl", "susp_fr", "susp_rl", "susp_rr", "sas_angle","error"
        };
        for (size_t i = 0; i < (sizeof(crop_objs)/sizeof(crop_objs[0])); i++) {
            Nextion_Cmd("%s.picc=%d", crop_objs[i], bg_id);
        }

        // Colori per le barre progressive del SAS
        Nextion_Cmd("sas_perc_left.bco=%d",  pco);
        Nextion_Cmd("sas_perc_left.pco=%d",  bco);
        Nextion_Cmd("sas_perc_right.bco=%d", bco);
        Nextion_Cmd("sas_perc_right.pco=%d", pco);
        Nextion_Cmd("ref 0");
    }

    Nextion_Cmd("speed.txt=\"%d\"",      car_state.mcu.car_speed);
    Nextion_Cmd("speed_fl.txt=\"%d\"",   car_state.mcu.wheel_speed_FL);
    Nextion_Cmd("speed_fr.txt=\"%d\"",   car_state.mcu.wheel_speed_FR);
    Nextion_Cmd("app1.txt=\"%d\"",       car_state.mcu.app1_raw);
    Nextion_Cmd("app2.txt=\"%d\"",       car_state.mcu.app2_raw);
    Nextion_Cmd("brake_bar.txt=\"%d\"",  car_state.mcu.brake_pressure_front);
    Nextion_Cmd("cool_temp.txt=\"%d\"",  car_state.pdm_vcu2.coolant_temp);
    Nextion_Cmd("susp_fl.txt=\"%d\"",    car_state.ecu.front_suspension_L);
    Nextion_Cmd("susp_fr.txt=\"%d\"",    car_state.ecu.front_suspension_R);
    Nextion_Cmd("susp_rl.txt=\"%d\"",    car_state.pdm_vcu2.rear_suspension_L);
    Nextion_Cmd("susp_rr.txt=\"%d\"",    car_state.pdm_vcu2.rear_suspension_R);

    if (r2d != last_r2d || force) {
        Nextion_Cmd("r2d_status.pic=%d", r2d ? 13 : 12);
        last_r2d = r2d;
    }
    if (sdc != last_sdc || force) {
        Nextion_Cmd("sdc_status.pic=%d", sdc ? 13 : 12);
        last_sdc = sdc;
    }

    int sas = car_state.mcu.sas_percentage; 
    int abs_sas = abs(sas);
    if (abs_sas > MAX_STEERING_ANGLE) {
        abs_sas = MAX_STEERING_ANGLE;
    }
    int sas_perc = (abs_sas * 100) / MAX_STEERING_ANGLE;
    int sas_left_val  = (sas < 0) ? (100 - sas_perc) : 100;
    int sas_right_val = (sas > 0) ? sas_perc : 0;
    Nextion_Cmd("sas_angle.txt=\"%d\"", sas); 
    Nextion_Cmd("sas_perc_left.val=%d", sas_left_val);
    Nextion_Cmd("sas_perc_right.val=%d", sas_right_val);
}

static void Draw_Page3(bool force, uint16_t bco, uint16_t pco)
{
    // Aggiornamento BACKGROUND e CROP IMAGE
    if (force) {
        uint8_t bg_id = get_bg_image_id(car_state.SR_map, 3);
        Nextion_Cmd("page3.pic=%d", bg_id);

        static const char* crop_objs[] = {
            "speed", "lvbatt_volt", "fan_perc", "max_temp_pack", "curr_dis_max", 
            "curr_reg_max", "temp_mot1", "temp_mot2", "temp_inv1", "temp_inv2", 
            "current", "soc", "soh", "m1_volt", "m2_volt", "m3_volt", "m4_volt", 
            "m5_volt", "m6_volt","error"
        };
        for (size_t i = 0; i < (sizeof(crop_objs)/sizeof(crop_objs[0])); i++) {
            Nextion_Cmd("%s.picc=%d", crop_objs[i], bg_id);
        }

        // Colori per le barre dei moduli
        for (int i = 1; i <= BMS_NUM_MODULES; i++) {
            Nextion_Cmd("m%d_perc.bco=%d", i, pco); /* bco/pco invertiti per i moduli */
            Nextion_Cmd("m%d_perc.pco=%d", i, bco);
        }
        Nextion_Cmd("ref 0");
    }

    int temp_pack_sum = 0, temp_pack_max = 0;
    for (int i = 0; i < BMS_NUM_MODULES; i++) {
        temp_pack_sum += car_state.bms.modules[i].temperature;
        if (car_state.bms.modules[i].t_max > temp_pack_max)
            temp_pack_max = car_state.bms.modules[i].t_max;
    }

    Nextion_Cmd("speed.txt=\"%d\"",        car_state.mcu.car_speed);
    Nextion_Cmd("lvbatt_volt.txt=\"%.1f\"", car_state.pdm_vcu1.lv_battery_voltage);
    Nextion_Cmd("fan_perc.txt=\"%d\"",      car_state.bms.battery_fan_percent);
    Nextion_Cmd("max_temp_pack.txt=\"%d\"", temp_pack_max);
    Nextion_Cmd("curr_dis_max.txt=\"%d\"",  car_state.bms.max_discharge_current);
    Nextion_Cmd("curr_reg_max.txt=\"%d\"",  car_state.bms.max_regen_current);
    Nextion_Cmd("temp_mot1.txt=\"%d\"",     car_state.mcu.motor104_temp);
    Nextion_Cmd("temp_mot2.txt=\"%d\"",     car_state.mcu.motor204_temp);
    Nextion_Cmd("temp_inv1.txt=\"%d\"",     car_state.mcu.inv104_temp);
    Nextion_Cmd("temp_inv2.txt=\"%d\"",     car_state.mcu.inv204_temp);
    Nextion_Cmd("current.txt=\"%d\"",       car_state.lem_cab.LEM_current);
    Nextion_Cmd("soc.txt=\"%d\"",           car_state.bms.SoC_percent);
    Nextion_Cmd("soh.txt=\"%d\"",           car_state.bms.SoH_percent);

for (int i = 0; i < BMS_NUM_MODULES; i++) {
        float mv = (float)car_state.bms.modules[i].voltage;
        if (mv < 100.0f) Nextion_Cmd("m%d_volt.txt=\"%.2f\"", i + 1, mv);
        else             Nextion_Cmd("m%d_volt.txt=\"%.1f\"", i + 1, mv);

        int bar_val = 0;
        if (mv >= MAX_MODULE_VOLTAGE) {
            bar_val = 100; 
        } 
        else if (mv <= MIN_MODULE_VOLTAGE) {
            bar_val = 0;  
        } 
        else {
            bar_val = (int)(((mv - MIN_MODULE_VOLTAGE) / (MAX_MODULE_VOLTAGE - MIN_MODULE_VOLTAGE)) * 100.0f);
        }
        Nextion_Cmd("m%d_perc.val=%d", i + 1, bar_val);
    }
}

static void Draw_Page4(bool force, uint16_t bco, uint16_t pco)
{
    static uint8_t last_setting = 0xFF;
    static uint8_t last_map     = 0xFF;
    static uint8_t last_regen = 1, last_src = 1, last_tvc = 1, last_event = 1;

    // Aggiornamento BACKGROUND e CROP IMAGE
    if (force) {
        uint8_t bg_id = get_bg_image_id(car_state.SR_map, 4);
        Nextion_Cmd("page4.pic=%d", bg_id);

        static const char* crop_objs[] = {
            "speed", "soc", "soh", "regen_text", "src_text", "tvc_text", "event_text",
            "regen_num", "src_num", "tvc_num", "event_num","error"
        };
        for (size_t i = 0; i < (sizeof(crop_objs)/sizeof(crop_objs[0])); i++) {
            Nextion_Cmd("%s.picc=%d", crop_objs[i], bg_id);
        }
        Nextion_Cmd("ref 0");
    }

    Nextion_Cmd("speed.txt=\"%d\"",  car_state.mcu.car_speed);
    Nextion_Cmd("soc.txt=\"%d\"",    car_state.bms.SoC_percent);
    Nextion_Cmd("soh.txt=\"%d\"",    car_state.bms.SoH_percent);
    Nextion_Cmd("regen_num.val=%d",  car_state.val_regen);
    Nextion_Cmd("src_num.val=%d",    car_state.val_src);
    Nextion_Cmd("tvc_num.val=%d",    car_state.val_tvc);
    Nextion_Cmd("event_num.val=%d",  car_state.val_event);

    bool changed = force
        || car_state.selected_setting != last_setting
        || car_state.SR_map           != last_map
        || car_state.val_regen        != last_regen
        || car_state.val_src          != last_src
        || car_state.val_tvc          != last_tvc
        || car_state.val_event        != last_event;

    if (!changed) return;

    /* Icone +/- in base alla mappa */
    uint8_t pic_p = 0, pic_m = 0;
    uint16_t txt_active = bco;
    switch (car_state.SR_map) {
        case 1: pic_p = 23; pic_m = 21; break; /* ECO  */
        case 2: pic_p = 22; pic_m = 20; break; /* NORM */
        case 3: pic_p = 15; pic_m = 14; break; /* GAS  */
        default: break;
    }
    (void)pco; 

    /* Helper macro per aggiornare una riga parametro */
#define DRAW_PARAM(name, sel, val) \
    Nextion_Cmd(name "_text.pco=%d",  (car_state.selected_setting == (sel)) ? txt_active : 65535); \
    Nextion_Cmd(name "_p.pic=%d",     pic_p); \
    Nextion_Cmd(name "_m.pic=%d",     pic_m); \
    Nextion_Cmd("vis " name "_m,%d",  ((val) > 1) ? 1 : 0); \
    Nextion_Cmd("vis " name "_p,%d",  ((val) < 4) ? 1 : 0)

    DRAW_PARAM("regen", 1, car_state.val_regen);
    DRAW_PARAM("src",   2, car_state.val_src);
    DRAW_PARAM("tvc",    3, car_state.val_tvc);
    DRAW_PARAM("event",   4, car_state.val_event);

#undef DRAW_PARAM

    last_setting = car_state.selected_setting;
    last_map     = car_state.SR_map;
    last_regen   = car_state.val_regen;
    last_src     = car_state.val_src;
    last_tvc     = car_state.val_tvc;
    last_event   = car_state.val_event;
}

/* =========================================================================
 * Task_UpdateDisplay  [100 ms]
 * ========================================================================= */
void Task_UpdateDisplay(void)
{
	//car_state.mcu.car_speed++;
    static DashError_t displayed_error = ERR_NONE;
    static uint32_t    error_timer     = 0;
    static uint8_t     last_page = 0xFF;
    static uint8_t     last_map  = 0xFF;

    /* --- 1. Errori: latch con hold timer --- */
    Task_Check_Errors();
    DashError_t current_error = car_state.error;

    if (current_error != ERR_NONE) {
        bool more_severe = (displayed_error == ERR_NONE) || (current_error < displayed_error);
        if (more_severe || (HAL_GetTick() - error_timer > ERROR_DISPLAY_HOLD_MS)) {
            displayed_error = current_error;
            error_timer     = HAL_GetTick();
        }
    } else if (displayed_error != ERR_NONE &&
               (HAL_GetTick() - error_timer > ERROR_DISPLAY_HOLD_MS)) {
        displayed_error = ERR_NONE;
    }

    Update_Error_Display(displayed_error);

    /* --- 2. Pre-calcolo stato grafico --- */
    bool force = (car_state.current_page != last_page || car_state.SR_map != last_map);
    last_page = car_state.current_page;
    last_map  = car_state.SR_map;

    bool r2d = car_state.r2d;
    bool sdc = car_state.bms.sdc_state;

    uint16_t bco = 0, pco = 0;
    get_map_colors(car_state.SR_map, &bco, &pco);

    /* --- 3. Aggiornamento pagina attiva --- */
    switch (car_state.current_page) {
        case 1: Draw_Page1(r2d, sdc, force);        break;
        case 2: Draw_Page2(r2d, sdc, force, bco, pco); break;
        case 3: Draw_Page3(force, bco, pco);        break;
        case 4: Draw_Page4(force, bco, pco);        break;
        default: break;
    }
}
