/* *****************************************************************************
 * P.H.2025: HAL de Interrupciones Externas (Implementación nRF - Nivel)
 *
 * Implementa la detección de NIVEL BAJO (SENSE_Low) según la lógica requerida.
 * Utiliza el evento GPIOTE->EVENTS_PORT, activado por las señales DETECT
 * de los pines GPIO.
 * ****************************************************************************/

#include "hal_ext_int.h"
#include <nrf.h>          
#include <stddef.h>   
#include "board.h"

/* #define NRF_GPIO_PIN_MAP(port, pin) (((port) << 5) | ((pin) & 0x1F))
#define BUTTONS_NUMBER 4

#define BUTTON_1       NRF_GPIO_PIN_MAP(0,11)
#define BUTTON_2       NRF_GPIO_PIN_MAP(0,12)
#define BUTTON_3       NRF_GPIO_PIN_MAP(0,24)
#define BUTTON_4       NRF_GPIO_PIN_MAP(0,25) */

static hal_ext_int_callback_t s_hal_callback = NULL;

// Mapeo de pines (índice = ID lógico de línea, valor = pin físico)
static const uint32_t s_pins_lineas[BUTTONS_NUMBER] = {
    BUTTON_1, BUTTON_2, BUTTON_3, BUTTON_4
};

// Configuración base de un pin de botón (sin SENSE)
#if (BUTTON_PULL == 1)
    #define _PULL_CONFIG (GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos)
#else
    #define _PULL_CONFIG (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos)
#endif

static const uint32_t BUTTON_PIN_CNF_BASE = 
    (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
    (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) |
    _PULL_CONFIG |
    (GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos) |
    (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos); // SENSE deshabilitado por defecto


/**
 * @brief Rutina de Servicio de Interrupción (ISR) del GPIOTE.
 *
 * Procesa el evento PORT global para detectar qué pin ha cambiado de nivel.
 */
void GPIOTE_IRQHandler(void) __irq
{
    if (NRF_GPIOTE->EVENTS_PORT) {
        // 1. Limpiar el evento general de puerto
        NRF_GPIOTE->EVENTS_PORT = 0;
        
        // 2. Leer el registro LATCH para identificar qué pin(es) han causado la interrupción
        uint32_t latch_p0 = NRF_P0->LATCH;
        
        // 3. Iterar por cada línea de botón configurada
        for (uint32_t i = 0; i < BUTTONS_NUMBER; i++) {
            uint32_t pin = s_pins_lineas[i];
            
            // Comprobar si el pin está marcado en el LATCH
            if (pin < 32 && (latch_p0 & (1UL << pin))) {
                
                // 4. Limpiar el LATCH para este pin (escribiendo un 1)
                NRF_P0->LATCH = (1UL << pin);
                
                // 5. Llamar al callback del driver (si está registrado)
                if (s_hal_callback) {
                    s_hal_callback(i); // i es el ID lógico de la línea
                }
            }
            // (La lógica para el Puerto 1 (NRF_P1) no es necesaria para la DK)
        }
    }
}

/**
 * @brief Inicializa el subsistema de interrupciones externas.
 *
 * Configura los pines GPIO, habilita el evento de puerto y la IRQ global.
 */
void hal_ext_int_iniciar(hal_ext_int_callback_t callback) 
{
    // 1. Guardar el puntero al callback que invocará la ISR
    s_hal_callback = callback;

    // 2. Configurar los pines GPIO de los botones (inicialmente sin SENSE activo)
    for (uint32_t i = 0; i < BUTTONS_NUMBER; i++) {
        uint32_t pin = s_pins_lineas[i];
        if (pin < 32) {
            NRF_P0->PIN_CNF[pin] = BUTTON_PIN_CNF_BASE;
        } else {
            // Lógica para P1 (no aplica en DK)
        }
    }

    // 3. Habilitar la interrupción para el evento PORT
    NRF_GPIOTE->INTENSET = GPIOTE_INTENSET_PORT_Msk;

    // 4. Habilitar la IRQ global del GPIOTE en el NVIC
    NVIC_ClearPendingIRQ(GPIOTE_IRQn);
    NVIC_EnableIRQ(GPIOTE_IRQn);
}

/**
 * @brief Habilita la interrupción por NIVEL BAJO (SENSE_Low) para una línea.
 */
void hal_ext_int_habilitar(uint32_t id_linea)
{
    if (id_linea >= BUTTONS_NUMBER) return;
    uint32_t pin = s_pins_lineas[id_linea];
    
    // Habilitar SENSE_Low en el pin para detectar la pulsación
    if (pin < 32) {
        NRF_P0->PIN_CNF[pin] = BUTTON_PIN_CNF_BASE | (GPIO_PIN_CNF_SENSE_Low << GPIO_PIN_CNF_SENSE_Pos);
    }
}

/**
 * @brief Deshabilita la interrupción por NIVEL para una línea.
 *
 * Desactiva la funcionalidad SENSE en el pin.
 */
void hal_ext_int_deshabilitar(uint32_t id_linea)
{
    if (id_linea >= BUTTONS_NUMBER) return;
    uint32_t pin = s_pins_lineas[id_linea];
    
    // Deshabilitar SENSE en el pin
    if (pin < 32) {
        NRF_P0->PIN_CNF[pin] = BUTTON_PIN_CNF_BASE | (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
    }
}

/**
 * @brief Limpia cualquier evento pendiente (LATCH) en una línea.
 */
void hal_ext_int_limpiar_pendiente(uint32_t id_linea)
{
    if (id_linea >= BUTTONS_NUMBER) return;
    uint32_t pin = s_pins_lineas[id_linea];
    
    // Limpiar el LATCH (memoria de detección) para este pin
    if (pin < 32) {
        NRF_P0->LATCH = (1UL << pin);
    }
}



