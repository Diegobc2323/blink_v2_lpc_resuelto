/* *****************************************************************************
 * P.H.2025: Driver/Manejador de consumo
 * Implementaci�n independiente del hardware, delega en HAL.
 */
#include "drv_consumo.h"
#include "hal_consumo.h"
#include "drv_monitor.h"
#include <stdbool.h>

static bool s_iniciado = false;
static uint32_t s_monitor = 0;

//bool drv_consumo_iniciar(uint32_t mon_wait, uint32) 
bool drv_consumo_iniciar(uint32_t mon_id) {
    hal_consumo_iniciar();
		s_monitor = mon_id;
    s_iniciado = true;
    return true;
}

void drv_consumo_esperar(void) {
    if (!s_iniciado) return;
		drv_monitor_desmarcar(s_monitor);
    hal_consumo_esperar();
		drv_monitor_marcar(s_monitor);
		
}

void drv_consumo_dormir(void) {
    if (!s_iniciado) return;
		drv_monitor_desmarcar(s_monitor);
    hal_consumo_dormir();
		drv_monitor_marcar(s_monitor);

}
