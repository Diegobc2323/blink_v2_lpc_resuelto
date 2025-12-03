/* *****************************************************************************
 * P.H.2025: HAL de Interrupciones Externas (nRF - Desacoplado con Extern)
 * ****************************************************************************/

#include "hal_ext_int.h"
#include <nrf.h>
#include <stddef.h>



extern const uint32_t g_hal_ext_int_pines[]; 
extern const uint8_t  g_hal_ext_int_num_pines;

static hal_ext_int_callback_t s_hal_callback = NULL;

// Configuración base por defecto
static const uint32_t PIN_CNF_DEF = 
    (GPIO_PIN_CNF_DIR_Input        << GPIO_PIN_CNF_DIR_Pos)   |
    (GPIO_PIN_CNF_INPUT_Connect    << GPIO_PIN_CNF_INPUT_Pos) |
    (GPIO_PIN_CNF_PULL_Pullup      << GPIO_PIN_CNF_PULL_Pos)  |
    (GPIO_PIN_CNF_SENSE_Disabled   << GPIO_PIN_CNF_SENSE_Pos);

void hal_ext_int_iniciar(hal_ext_int_callback_t callback) 
{
    s_hal_callback = callback;

    // Usamos las variables externas para configurar
    for (uint8_t i = 0; i < g_hal_ext_int_num_pines; i++) {
        uint32_t pin = g_hal_ext_int_pines[i];
        if (pin < 32) {
            NRF_P0->PIN_CNF[pin] = PIN_CNF_DEF;
        }
    }

    NRF_GPIOTE->INTENSET = GPIOTE_INTENSET_PORT_Msk;
    NVIC_ClearPendingIRQ(GPIOTE_IRQn);
    NVIC_EnableIRQ(GPIOTE_IRQn);
}

void hal_ext_int_habilitar(uint32_t id_linea)
{
    if (id_linea >= g_hal_ext_int_num_pines) return;
    uint32_t pin = g_hal_ext_int_pines[id_linea];
    
    if (pin < 32) {
        // Habilitar SENSE_Low
        uint32_t cnf = NRF_P0->PIN_CNF[pin];
        cnf &= ~GPIO_PIN_CNF_SENSE_Msk;
        cnf |= (GPIO_PIN_CNF_SENSE_Low << GPIO_PIN_CNF_SENSE_Pos);
        NRF_P0->PIN_CNF[pin] = cnf;
    }
}

void hal_ext_int_deshabilitar(uint32_t id_linea)
{
    if (id_linea >= g_hal_ext_int_num_pines) return;
    uint32_t pin = g_hal_ext_int_pines[id_linea];
    
    if (pin < 32) {
        uint32_t cnf = NRF_P0->PIN_CNF[pin];
        cnf &= ~GPIO_PIN_CNF_SENSE_Msk;
        cnf |= (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
        NRF_P0->PIN_CNF[pin] = cnf;
    }
}

void hal_ext_int_limpiar_pendiente(uint32_t id_linea)
{
    if (id_linea >= g_hal_ext_int_num_pines) return;
    uint32_t pin = g_hal_ext_int_pines[id_linea];
    
    if (pin < 32) {
        NRF_P0->LATCH = (1UL << pin);
    }
}

void hal_ext_int_habilitar_despertar(uint32_t id)    { hal_ext_int_habilitar(id); }
void hal_ext_int_deshabilitar_despertar(uint32_t id) { hal_ext_int_deshabilitar(id); }

void GPIOTE_IRQHandler(void) 
{
    if (NRF_GPIOTE->EVENTS_PORT) {
        NRF_GPIOTE->EVENTS_PORT = 0;
        uint32_t latch = NRF_P0->LATCH;
        
        for (uint8_t i = 0; i < g_hal_ext_int_num_pines; i++) {
            uint32_t pin = g_hal_ext_int_pines[i];
            if (pin < 32 && (latch & (1UL << pin))) {
                NRF_P0->LATCH = (1UL << pin);
                if (s_hal_callback) s_hal_callback(i);
            }
        }
    }
}