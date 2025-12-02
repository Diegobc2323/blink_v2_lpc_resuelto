/* *****************************************************************************
 * P.H.2025: HAL de Interrupciones Externas (Interfaz)
 *
 * Abstrae el hardware de bajo nivel para la detección de eventos en pines de entrada (EINT/GPIOTE).
 * Proporciona el mecanismo básico para que el driver de botones sea portable.
 * ****************************************************************************/

#ifndef HAL_EXT_INT_H
#define HAL_EXT_INT_H

#include <stdint.h>

/**
 * @brief Tipo de dato para la función de callback.
 *
 * Esta función es el punto de entrada al software desde la interrupción hardware (ISR).
 *
 * @param id_linea El canal/línea lógica (0..N) que ha generado la interrupción.
 */
typedef void (*hal_ext_int_callback_t)(uint8_t id_linea);


/**
 * @brief Inicializa el subsistema de interrupciones externas (HAL).
 *
 * Configura los pines (GPIO) y el periférico de interrupciones (EINT/GPIOTE) para la detección.
 * No habilita las interrupciones, solo registra la función de aviso (callback).
 *
 * @param callback Puntero a la función que debe ser invocada por la ISR.
 */
void hal_ext_int_iniciar(hal_ext_int_callback_t callback);

/**
 * @brief Habilita la generación de interrupción para una línea/canal.
 *
 * @param id_linea El canal lógico (0..N) a habilitar.
 */
void hal_ext_int_habilitar(uint32_t id_linea);

/**
 * @brief Deshabilita la generación de interrupción para una línea/canal.
 *
 * @param id_linea El canal lógico (0..N) a deshabilitar.
 */
void hal_ext_int_deshabilitar(uint32_t id_linea);

/**
 * @brief Limpia cualquier evento pendiente de interrupción en una línea.
 *
 * Esta operación debe realizarse antes de re-habilitar una interrupción.
 *
 * @param id_linea El canal lógico (0..N) a limpiar.
 */
void hal_ext_int_limpiar_pendiente(uint32_t id_linea);

/**
 * @brief Habilita el despertar del modo de bajo consumo para una línea.
 *
 * Permite que la interrupción externa despierte al microcontrolador
 * desde modos de ahorro de energía (sleep, power-down, etc.).
 *
 * @param id_linea El canal lógico (0..N) a habilitar como fuente de despertar.
 */
void hal_ext_int_habilitar_despertar(uint32_t id_linea);

/**
 * @brief Deshabilita el despertar del modo de bajo consumo para una línea.
 *
 * La interrupción no podrá despertar al microcontrolador desde
 * modos de ahorro de energía.
 *
 * @param id_linea El canal lógico (0..N) a deshabilitar como fuente de despertar.
 */
void hal_ext_int_deshabilitar_despertar(uint32_t id_linea);

#endif // HAL_EXT_INT_H
