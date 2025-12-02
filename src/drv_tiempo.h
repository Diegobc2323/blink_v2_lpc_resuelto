/* *****************************************************************************
 * P.H.2025: Driver/Manejador de tiempo (independiente de HW)
 * Mantiene concepto: tiempo absoluto en us/ms + espera bloqueante y hasta “deadline”.
 * Cambios 2025:
 *  - Internamente usa hal_tiempo_info_t.
 */
#ifndef DRV_TIEMPO_H
#define DRV_TIEMPO_H

#include <stdint.h>
#include <stdbool.h>

typedef uint64_t Tiempo_us_t;
typedef uint32_t Tiempo_ms_t;

/* Arranca el reloj; devuelve true si ok */
bool drv_tiempo_iniciar(void);

/* Tiempo desde inicio en micro/mili segundos */
Tiempo_us_t drv_tiempo_actual_us(void);
Tiempo_ms_t drv_tiempo_actual_ms(void);

/* Esperas bloqueantes */
void drv_tiempo_esperar_ms(Tiempo_ms_t ms);

/* Esperar hasta (deadline en ms). Devuelve el tiempo actual tras la espera. */
Tiempo_ms_t drv_tiempo_esperar_hasta_ms(Tiempo_ms_t deadline_ms);
void drv_tiempo_periodico_ms(Tiempo_ms_t ms, void(*funcion_callback_app)(), uint32_t ID_evento);
#endif // DRV_TIEMPO_H
