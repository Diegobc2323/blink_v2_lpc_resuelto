/* *****************************************************************************
 * P.H.2025: HAL de Sonido (Interfaz)
 *
 * Abstrae el hardware de bajo nivel para la generación de tonos mediante PWM o buzzer.
 * Proporciona el mecanismo básico para que el driver de sonido sea portable.
 * ****************************************************************************/

#ifndef HAL_SONIDO_H
#define HAL_SONIDO_H

#include <stdint.h>

/**
 * @brief Inicializa el subsistema de sonido (HAL).
 * 
 * Configura el hardware necesario (PWM, timers, pines GPIO) para
 * la generación de tonos.
 */
void hal_sonido_iniciar(void);

/**
 * @brief Genera un tono con la frecuencia y duración especificadas.
 * 
 * NOTA: Esta implementación es BLOQUEANTE (detiene la ejecución durante 'ms').
 * 
 * @param frecuencia_hz Frecuencia del tono en Hz (ej. 1000 para 1kHz).
 * @param duracion_ms Duración del tono en milisegundos.
 */
void hal_sonido_tocar(uint32_t frecuencia_hz, uint32_t duracion_ms);

#endif // HAL_SONIDO_H
