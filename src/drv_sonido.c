/* *****************************************************************************
 * P.H.2025: Driver de Sonido
 *
 * Driver portable de sonido que delega la implementación al HAL correspondiente.
 * ****************************************************************************/

#include "drv_sonido.h"
#include "hal_sonido.h"


void drv_sonido_iniciar(void) {
    hal_sonido_iniciar();
}


void drv_sonido_tocar(uint32_t frecuencia_hz, uint32_t duracion_ms) {
    hal_sonido_tocar(frecuencia_hz, duracion_ms);
}