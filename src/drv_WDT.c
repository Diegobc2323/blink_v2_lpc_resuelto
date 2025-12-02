#include "drv_WDT.h"
#include "hal_WDT.h" // Necesario para ver las funciones hal_WDT_...

void drv_WDT_iniciar(uint32_t sec) {
    // Simplemente delegamos en el hardware específico
    hal_WDT_iniciar(sec);
}

void drv_WDT_alimentar(void) {
    // Delegamos la alimentación al hardware específico
    hal_WDT_feed();
}
