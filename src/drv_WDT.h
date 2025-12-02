#ifndef DRV_WDT_H
#define DRV_WDT_H

#include <stdint.h>

/**
 * @brief Inicializa el sistema de Watchdog.
 * Llama internamente a la HAL correspondiente.
 * @param sec Tiempo en segundos antes del reinicio.
 */
void drv_WDT_iniciar(uint32_t sec);

/**
 * @brief "Alimenta" al perro guardián para evitar el reinicio.
 * Debe llamarse periódicamente (o al pulsar botón).
 */
void drv_WDT_alimentar(void);

#endif
