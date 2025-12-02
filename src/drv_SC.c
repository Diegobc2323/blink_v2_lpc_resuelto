#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "drv_SC.h"
#include "hal_SC.h"

static uint32_t m_in_critical_region = 0;

uint32_t drv_SC_entrar_disable_irq(void){
    // Deshabilita las interrupciones y devuelve el estado actual de la región crítica
    deshabilitar_irq();
    return(m_in_critical_region++);
}

void drv_SC_salir_enable_irq(void){
    // Restaura el estado previo de las interrupciones
    m_in_critical_region--;
    if (m_in_critical_region == 0) {
        habilitar_irq();
    }
}

