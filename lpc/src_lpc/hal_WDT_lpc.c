#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "lpc21xx.h"
#include "hal_WDT.h"


void hal_WDT_iniciar(uint32_t sec){
    // Configura el valor de recarga (timeout) del WDT 
    WDTC = sec * (15000000 / 4); 
    
    // Habilita el WDT y lo configura para generar un reset 
    WDMOD = 0x03; 
    
    // Primera alimentación para iniciar el contador del WDT 
    hal_WDT_feed();

}
void hal_WDT_feed(void){
    // Secuencia de alimentación atómica (WDFEED = 0xAA; WDFEED = 0x55) 
    // Esta secuencia reinicia el contador del WDT 
    WDFEED = 0xAA; 
    WDFEED = 0x55; 

}


