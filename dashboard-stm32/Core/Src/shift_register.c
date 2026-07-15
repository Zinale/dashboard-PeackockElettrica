#include "shift_register.h"

// Globale nel file (nascosta all'esterno)
static SR_State_t sr_state = {
    .currentValue = 0,
    .lastRawValue = 0xFF,
    .debounceCounter = 0,
    .lastReadTime = 0,
    .isValid = 0
};

// ====================================================================
// FUNZIONI PRIVATE
// ====================================================================

// Lettura grezza bit-banging

// Lettura grezza bit-banging (Identica alla tua, ottima)
static uint8_t SR_ReadRaw(void) {
    uint8_t data = 0;
    
    // Pulse sul latch per caricare i dati paralleli (PL attivo basso)
    HAL_GPIO_WritePin(SR_latch_GPIO_Port, SR_latch_Pin, GPIO_PIN_RESET);
    for (volatile int i = 0; i < 72; i++) __NOP(); // circa ~1us @ 72MHz
    HAL_GPIO_WritePin(SR_latch_GPIO_Port, SR_latch_Pin, GPIO_PIN_SET);
    
    for (volatile int i = 0; i < 10; i++) __NOP(); // Delay sicurezza
    
    // Leggi i dati (MSB first: Q7 -> Q0)
    for (int i = 0; i < 8; i++) {
        data <<= 1;
        
        if (HAL_GPIO_ReadPin(SR_data_GPIO_Port, SR_data_Pin) == GPIO_PIN_SET) {
            data |= 1;
        }
        
        HAL_GPIO_WritePin(SR_clock_GPIO_Port, SR_clock_Pin, GPIO_PIN_SET);
        for (volatile int j = 0; j < 5; j++) __NOP(); // Hold time
        HAL_GPIO_WritePin(SR_clock_GPIO_Port, SR_clock_Pin, GPIO_PIN_RESET);
        
        if (i < 7) {
            for (volatile int j = 0; j < 5; j++) __NOP(); 
        }
    }
    
    return data;
}

// Conta i bit attivi 
static uint8_t countBits(uint8_t value) {
    uint8_t count = 0;
    while (value) {
        count += value & 1;
        value >>= 1;
    }
    return count;
}

// Converte il Byte nella Posizione fisica del selettore (1-8)
static int8_t byteToPosition(uint8_t data) {
    uint8_t bitCount = countBits(data);
    
    if (bitCount == 0) return 0; // Nessuna posizione attiva
    if (bitCount != 1) return -1; // Errore: cortocircuito tra posizioni
    
    for (int i = 0; i < 8; i++) {
        if (data & (1 << i)) {
            return i + 1; 
        }
    }
    return -1; 
}


// ====================================================================
// FUNZIONI PUBBLICHE (chiamate da Tasks.c)
// ====================================================================

void SR_Init(void) {
    HAL_GPIO_WritePin(SR_clock_GPIO_Port, SR_clock_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SR_latch_GPIO_Port, SR_latch_Pin, GPIO_PIN_SET);
    
    uint8_t initialValue = SR_ReadRaw();
    sr_state.currentValue = initialValue;
    sr_state.lastRawValue = initialValue;
    sr_state.debounceCounter = SR_DEBOUNCE_SAMPLES; 
    sr_state.lastReadTime = HAL_GetTick();
    sr_state.isValid = 0;
}

void SR_ProcessPeriodic(void) {
    uint32_t currentTime = HAL_GetTick();
    
    if ((currentTime - sr_state.lastReadTime) < SR_READ_INTERVAL_MS) {
        return;
    }
    sr_state.lastReadTime = currentTime;
    
    uint8_t rawValue = SR_ReadRaw();
    
    // Logica di Debouncing
    if (rawValue == sr_state.lastRawValue) {
        if (sr_state.debounceCounter < SR_DEBOUNCE_SAMPLES) {
            sr_state.debounceCounter++;
        }
        
        if (sr_state.debounceCounter >= SR_DEBOUNCE_SAMPLES) {
            if (rawValue != sr_state.currentValue) {
                sr_state.currentValue = rawValue;
                sr_state.isValid = 1; // Alza la flag per Tasks.c
            }
        }
    } else {
        sr_state.lastRawValue = rawValue;
        sr_state.debounceCounter = 1; // Resetta il contatore se fluttua
    }
}

uint8_t SR_HasChanged(void) {
    if (sr_state.isValid) {
        sr_state.isValid = 0; // Consuma l'evento (Reset del flag)
        return 1;
    }
    return 0;
}

int8_t SR_GetStablePosition(void) {
    return byteToPosition(sr_state.currentValue);
}

uint8_t SR_GetRawValue(void) {
    return sr_state.lastRawValue;
}
