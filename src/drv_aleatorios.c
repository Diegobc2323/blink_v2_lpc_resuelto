#include "drv_aleatorios.h"
#include "hal_aleatorios.h" 

void drv_aleatorios_iniciar(uint32_t semilla) {
    hal_aleatorios_iniciar(semilla);
}

uint32_t drv_aleatorios_obtener_32bits(void) {
    // Delegamos la obtención del entero de 32 bits al HAL directamente
    return hal_aleatorios_obtener_32bits();
}

uint32_t drv_aleatorios_rango(uint32_t rango) {
    if (rango == 0) return 0;
    uint32_t aleatorio = drv_aleatorios_obtener_32bits();
    return (aleatorio % rango);
}
