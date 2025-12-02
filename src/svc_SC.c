#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "svc_SC.h"

static uint32_t m_in_critical_region = 0;

uint32_t svc_SC_entrar_disable_irq(void){
    // Deshabilita las interrupciones y devuelve el estado actual de la región crítica
    __disable_irq();
    return(m_in_critical_region++);
}

void svc_SC_salir_enable_irq(void){
    // Restaura el estado previo de las interrupciones
    m_in_critical_region--;
    if (m_in_critical_region == 0) {
        __enable_irq();
    }
}

