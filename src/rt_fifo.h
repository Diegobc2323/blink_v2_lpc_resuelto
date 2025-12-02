/* *****************************************************************************
 * P.H.2025: Runtime FIFO - Cola circular de eventos
 * Implementación thread-safe con protecciones de sección crítica.
 */
#ifndef RT_FIFO_H
#define RT_FIFO_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "rt_evento_t.h"
#include "drv_monitor.h"
#include "drv_tiempo.h"

/**
 * @brief Inicializa la cola FIFO circular de eventos.
 *
 * Configura los índices de lectura/escritura a 0 y prepara la estructura
 * para almacenar eventos. Debe llamarse antes de usar rt_FIFO_encolar o rt_FIFO_extraer.
 *
 * @param monitor_overflow ID del monitor a marcar en caso de overflow de la cola.
 */
void rt_FIFO_inicializar(uint32_t monitor_overflow);

/**
 * @brief Encola un nuevo evento en la cola FIFO.
 *
 * Añade un evento al final de la cola circular con timestamp automático.
 * Si la cola está llena (overflow), marca el monitor y detiene el sistema.
 * Esta función es thread-safe gracias a secciones críticas.
 *
 * @param ID_evento Tipo de evento a encolar (ver rt_evento_t.h).
 * @param auxData Datos auxiliares asociados al evento (payload).
 */
void rt_FIFO_encolar(uint32_t ID_evento, uint32_t auxData);

/**
 * @brief Extrae el siguiente evento de la cola FIFO.
 *
 * Obtiene y elimina el evento más antiguo (FIFO). Si la cola está vacía,
 * retorna 0 inmediatamente. Esta función es thread-safe.
 *
 * @param ID_evento Puntero donde almacenar el tipo de evento extraído (puede ser NULL).
 * @param auxData Puntero donde almacenar los datos auxiliares (puede ser NULL).
 * @param TS Puntero donde almacenar el timestamp del evento en microsegundos (puede ser NULL).
 * @return 0 si la cola está vacía, 1 si quedan 0 eventos tras extraer, o el número de eventos restantes.
 */
uint8_t rt_FIFO_extraer(EVENTO_T *ID_evento, uint32_t *auxData, Tiempo_us_t *TS);

/**
 * @brief Obtiene estadísticas de eventos encolados por tipo.
 *
 * Retorna el número total de veces que un tipo de evento específico
 * ha sido encolado desde el inicio del sistema (solo en modo DEBUG).
 *
 * @param ID_evento Tipo de evento del cual obtener estadísticas.
 * @return Número de veces que el evento ha sido encolado (0 si DEBUG no está activo).
 */
uint32_t rt_FIFO_estadisticas(EVENTO_T ID_evento);

#endif /* RT_FIFO_H */
