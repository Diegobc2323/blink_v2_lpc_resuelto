#ifndef DRV_SONIDO_H
#define DRV_SONIDO_H

#include <stdint.h>

/**
 * @brief Inicializa el driver de sonido.
 * Configura el pin del buzzer como salida.
 */
void drv_sonido_iniciar(void);

/**
 * @brief Genera un tono con la frecuencia y duración especificadas.
 * * NOTA: Esta implementación es BLOQUEANTE (detiene la ejecución durante 'ms').
 * Se usa así por simplicidad. Dado que los sonidos son cortos (<500ms) 
 * y el Watchdog suele ser de 1s o más, es seguro en este contexto.
 *
 * @param frecuencia_hz Frecuencia del tono en Hz (ej. 1000 para 1kHz).
 * @param duracion_ms Duración del tono en milisegundos.
 */
void drv_sonido_tocar(uint32_t frecuencia_hz, uint32_t duracion_ms);

#endif /* DRV_SONIDO_H */