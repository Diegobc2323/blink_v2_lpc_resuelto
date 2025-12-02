/* *****************************************************************************
 * P.H.2025: Driver/Manejador de consumo
 * Interfaz independiente del hardware. Usa hal_consumo_nrf en esta placa.
 */
#ifndef DRV_CONSUMO_H
#define DRV_CONSUMO_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/** Inicializa el gestor de consumo. Devuelve true si OK. */
bool drv_consumo_iniciar(uint32_t mon_id);

/** Entra en modo "esperar" (System ON sleep) hasta la próxima IRQ. */
void drv_consumo_esperar(void);

/** Entra en modo "dormir" (igual que esperar en esta versión). */
void drv_consumo_dormir(void);

#endif /* DRV_CONSUMO_H */
