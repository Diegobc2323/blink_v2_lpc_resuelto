#ifndef BEAT_HERO_H
#define BEAT_HERO_H

#include <stdint.h>
#include "rt_evento_t.h"

/**
 * @brief Inicializa el subsistema del juego Beat Hero.
 * * Configura la máquina de estados inicial, suscribe los callbacks al 
 * Gestor de Eventos (rt_GE) e inicializa variables de juego.
 * * @note Requiere que drv_leds, drv_botones, svc_alarmas y drv_aleatorios 
 * estén inicializados previamente.
 */
void beat_hero_iniciar(void);

/**
 * @brief Máquina de Estados Finita (FSM) del juego.
 * * Esta función debe suscribirse al Gestor de Eventos. Maneja la lógica 
 * principal: transiciones de estado, evaluación de pulsaciones (scoring),
 * gestión de compases y control de tiempos.
 * * @param evento Evento recibido (ej: ev_PULSAR_BOTON, ev_JUEGO_TICK).
 * @param auxData Datos auxiliares del evento (ej: ID del botón).
 */
void beat_hero_actualizar(EVENTO_T evento, uint32_t auxData);

#endif /* BEAT_HERO_H */