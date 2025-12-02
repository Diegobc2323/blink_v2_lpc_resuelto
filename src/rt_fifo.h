/* *****************************************************************************
 * P.H.2025: Driver/Manejador de consumo
 * Interfaz independiente del hardware. Usa hal_consumo_nrf en esta placa.
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
 * inicializamos la estructura de la cola
 */
void rt_FIFO_inicializar(uint32_t monitor_overflow);

/**
 * añade ts
 * si si esta llena, marcamos overflow y nos quedamos en un bucle infinito
 */
void rt_FIFO_encolar(uint32_t ID_evento, uint32_t auxData);

/**
 * 
 */
uint8_t rt_FIFO_extraer(EVENTO_T *ID_evento, uint32_t *auxData, Tiempo_us_t *TS);

/**
 * 
 */
uint32_t rt_FIFO_estadisticas(EVENTO_T ID_evento);

#endif /* RT_FIFO_H */
