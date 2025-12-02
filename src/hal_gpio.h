/* *****************************************************************************
 * P.H.2025: HAL GPIO
 * 
 * Capa de abstracción de hardware para el acceso a los GPIOs.
 * Nos independiza del hardware concreto de la placa.
 *
 * Define:
 *   - Tipo de dato para los pines (HAL_GPIO_PIN_T).
 *   - Direcciones de los pines (entrada / salida).
 *   - Funciones para inicializar, configurar, leer y escribir GPIOs.
 */

#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include <stdint.h>

/* Dirección de los GPIOs (E/S) */
typedef enum {
    HAL_GPIO_PIN_DIR_INPUT  = 0, 
    HAL_GPIO_PIN_DIR_OUTPUT = 1,
} hal_gpio_pin_dir_t;

/* Tipo de dato para identificar un pin GPIO */
typedef uint32_t HAL_GPIO_PIN_T;

/**
 * @brief Inicializa el subsistema GPIO.
 *
 * Debe invocarse antes de usar el resto de funciones.
 * Reconfigura todos los pines como entradas para evitar cortocircuitos.
 */
void hal_gpio_iniciar(void);

/**
 * @brief Configura la dirección (entrada/salida) de un GPIO.
 *
 * @param gpio      Pin a configurar.
 * @param direccion HAL_GPIO_PIN_DIR_INPUT o HAL_GPIO_PIN_DIR_OUTPUT.
 */
void hal_gpio_sentido(HAL_GPIO_PIN_T gpio, hal_gpio_pin_dir_t direccion);

/**
 * @brief Lee el valor lógico de un GPIO.
 *
 * @param gpio Pin a leer.
 * @return 0 si el GPIO está a nivel bajo, distinto de 0 si está a nivel alto.
 */
uint32_t hal_gpio_leer(HAL_GPIO_PIN_T gpio);

/**
 * @brief Escribe un valor lógico en un GPIO.
 *
 * @param gpio  Pin a escribir.
 * @param valor Valor a escribir: 0 ? nivel bajo, distinto de 0 ? nivel alto.
 */
void hal_gpio_escribir(HAL_GPIO_PIN_T gpio, uint32_t valor);

#endif /* HAL_GPIO_H */
