#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "hal_SC.h"


void deshabilitar_irq(void){
    // Deshabilita las interrupciones y devuelve el estado actual de la región crítica
    __disable_irq();
		
    
}

void habilitar_irq(void){
    // Restaura el estado previo de las interrupciones
    __enable_irq();
    
}
