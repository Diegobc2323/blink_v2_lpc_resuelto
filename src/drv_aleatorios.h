#ifndef DRV_ALEATORIOS_H
#define DRV_ALEATORIOS_H

#include <stdint.h>

/**
 * @brief Inicializa el Generador de Números Aleatorios por Hardware (RNG).
 * * En el nRF52840 activa el periférico RNG y la corrección de sesgo (Bias Correction).
 * * @param semilla Un valor inicial. NOTA: En el nRF52 (TRNG) la semilla se ignora 
 * físicamente porque usa ruido térmico, pero se mantiene por 
 * compatibilidad con la interfaz solicitada.
 */
void drv_aleatorios_iniciar(uint32_t semilla);

/**
 * @brief Obtiene un número aleatorio de 32 bits completo.
 * * Construye un entero de 32 bits leyendo 4 veces el registro de 8 bits del hardware.
 * * @return Un número aleatorio entre 0 y UINT32_MAX.
 */
uint32_t drv_aleatorios_obtener_32bits(void);

/**
 * @brief Obtiene un número aleatorio dentro de un rango [0, rango-1].
 * * @param rango El límite superior (exclusivo).
 * @return Un valor entre 0 y (rango - 1).
 */
uint32_t drv_aleatorios_rango(uint32_t rango);

#endif /* DRV_ALEATORIOS_H */