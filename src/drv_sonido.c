/* *****************************************************************************
 * P.H.2025: Driver de Sonido
 *
 * Driver portable de sonido que delega la implementación al HAL correspondiente.
 * ****************************************************************************/

#include "drv_sonido.h"
#include "hal_sonido.h"

/**
 * @brief Inicializa el driver de sonido.
 * Configura el pin del buzzer como salida mediante el HAL.
 */
void drv_sonido_iniciar(void) {
    hal_sonido_iniciar();
}

/**
 * @brief Genera un tono con la frecuencia y duración especificadas.
 * 
 * NOTA: Esta implementación es BLOQUEANTE (detiene la ejecución durante 'ms').
 * Se usa así por simplicidad. Dado que los sonidos son cortos (<500ms) 
 * y el Watchdog suele ser de 1s o más, es seguro en este contexto.
 *
 * @param frecuencia_hz Frecuencia del tono en Hz (ej. 1000 para 1kHz).
 * @param duracion_ms Duración del tono en milisegundos.
 */
void drv_sonido_tocar(uint32_t frecuencia_hz, uint32_t duracion_ms) {
    hal_sonido_tocar(frecuencia_hz, duracion_ms);
}
