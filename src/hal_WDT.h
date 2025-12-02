/* *****************************************************************************
 * P.H.2025: HAL del Watchdog Timer (WDT)
 *
 * Implementa la interfaz de bajo nivel para el Perro Guardián,
 * un temporizador hardware crucial para detectar y recuperar el sistema
 * de cuelgues o bloqueos.
 * ****************************************************************************/

#ifndef HAL_WDT_H
#define HAL_WDT_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/**
 * @brief Inicializa el WDT y comienza su cuenta.
 *
 * Configura el temporizador con el tiempo límite especificado por 'sec'
 *
 * @param sec El tiempo de espera (timeout) en segundos antes de que ocurra un reset.
 */
void hal_WDT_iniciar(uint32_t sec);

/**
 * @brief Alimenta al WDT (feed).
 *
 * Reinicia el contador del temporizador. Debe llamarse periódicamente
 * antes de que el tiempo límite se agote para evitar un reset.
 */
void hal_WDT_feed(void);


#endif /* HAL_WDT_H */
