/* *****************************************************************************
 * P.H.2025: HAL de Sonido (Implementación nRF52)
 *
 * Implementa la generación de tonos usando el periférico PWM del nRF52840.
 * Utiliza PWM0 con un prescaler de DIV_16 para obtener frecuencias audibles.
 * ****************************************************************************/

#include "hal_sonido.h"
#include "board.h"
#include "nrf.h"

// Buffer para la secuencia PWM: [valor_comparacion]
// Debe estar en RAM retenida (static) para que el DMA del PWM acceda a él
static uint16_t seq_values[1]; 

void hal_sonido_iniciar(void) {
    // 1. Configurar el pin como salida
    uint32_t port = (BUZZER_PIN >> 5) & 1;
    uint32_t pin  = (BUZZER_PIN & 0x1F);
    
    if (port == 0) NRF_P0->DIRSET = (1 << pin);
    else           NRF_P1->DIRSET = (1 << pin);

    // 2. Configurar PWM0
    // Conectar el canal 0 del PWM0 al pin del Buzzer
    NRF_PWM0->PSEL.OUT[0] = BUZZER_PIN;
    
    // Modo Up-Counter (cuenta hacia arriba y reinicia)
    NRF_PWM0->MODE = (PWM_MODE_UPDOWN_Up << PWM_MODE_UPDOWN_Pos);
    
    // Prescaler: 16MHz / 16 = 1MHz (Tick de 1us). Precisión de microsegundos.
    NRF_PWM0->PRESCALER = (PWM_PRESCALER_PRESCALER_DIV_16 << PWM_PRESCALER_PRESCALER_Pos);
    
    // Configuración de polaridad y modo de carga (Common)
    NRF_PWM0->DECODER = (PWM_DECODER_LOAD_Common << PWM_DECODER_LOAD_Pos) |
                        (PWM_DECODER_MODE_RefreshCount << PWM_DECODER_MODE_Pos);
                        
    // Habilitar el periférico PWM
    NRF_PWM0->ENABLE = 1;
}

void hal_sonido_tocar(uint32_t frecuencia_hz, uint32_t duracion_ms) {
    if (frecuencia_hz == 0) {
        NRF_PWM0->TASKS_STOP = 1;
        return;
    }

    // Calcular el periodo en microsegundos (T = 1/f)
    // Con el reloj a 1MHz, el valor del contador equivale a us.
    uint32_t periodo_us = 1000000 / frecuencia_hz;
    
    // El "Top Value" del contador determina la frecuencia de la onda
    NRF_PWM0->COUNTERTOP = periodo_us;
    
    // Ciclo de trabajo 50% (para que suene fuerte y claro) -> La mitad del periodo
    // Bit 15 a 0 para polaridad estándar
    seq_values[0] = periodo_us / 2; 
    
    // Configurar la secuencia para el DMA
    NRF_PWM0->SEQ[0].PTR = (uint32_t)seq_values;
    NRF_PWM0->SEQ[0].CNT = 1; // 1 valor en el buffer
    NRF_PWM0->SEQ[0].REFRESH = 0;
    NRF_PWM0->SEQ[0].ENDDELAY = 0;
    
    // Disparar la tarea de inicio de secuencia
    NRF_PWM0->TASKS_SEQSTART[0] = 1;
    
    // Espera "sucia" para mantener la nota (bloqueante simple para esta práctica)
    // En un sistema real usaríamos alarmas para apagarlo asíncronamente.
    nrf_delay_ms(duracion_ms);
    
    // Apagar sonido al terminar el tiempo
    NRF_PWM0->TASKS_STOP = 1;
}

// Función auxiliar simple para delay (si no tienes nrf_delay.h incluido)
void nrf_delay_ms(uint32_t ms) {
    // Aproximación para bucle en C con optimización baja/media a 64MHz
    volatile uint32_t i = ms * 6000; 
    while(i--) __NOP();
}
