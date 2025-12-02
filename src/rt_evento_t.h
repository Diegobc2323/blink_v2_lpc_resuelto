/* *****************************************************************************
 * P.H.2025: Driver/Manejador de consumo
 * Interfaz independiente del hardware. Usa hal_consumo_nrf en esta placa.
 */
#ifndef RT_EVENTO_H
#define RT_EVENTO_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "drv_tiempo.h"

typedef enum {
    ev_VOID = 0,
    ev_T_PERIODICO = 1,
    ev_PULSAR_BOTON = 2,
    ev_INACTIVIDAD = 3,
    ev_BOTON_TIMER = 4,
    ev_USUARIO_1 = 5,
    ev_JUEGO_NUEVO_LED = 6,
    ev_JUEGO_TIMEOUT = 7,
    ev_SOLTAR_BOTON = 8  
} EVENTO_T;
#define EVENT_TYPES 9


typedef struct {
    EVENTO_T ID_EVENTO;
    uint32_t auxData;
    Tiempo_us_t TS;
} EVENTO;



#endif /* RT_EVENTO_H */

