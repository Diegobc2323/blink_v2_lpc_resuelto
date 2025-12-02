#ifndef RT_GE_H
#define RT_GE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "rt_evento_t.h"
typedef void (*f_callback_GE)(EVENTO_T evento, uint32_t aux);
/**
 * @brief Inicializa el Gestor de Eventos (capa Run-Time).
 *
 * Configura las estructuras de despacho y alarma iniciales.
 *
 * @param monitor_overflow El ID del monitor a marcar si ocurre un desbordamiento de suscripciones.
 */
void rt_GE_iniciar(uint32_t monitor_overflow);

/**
 * @brief Bucle principal del sistema (dispatcher de eventos/planificador).
 *
 * Extrae eventos de la cola (FIFO) y los despacha a las tareas suscritas,
 * o pone al procesador en modo IDLE (bajo consumo) si la cola está vacía.
 */
void rt_GE_lanzador(void);

/**
 * @brief Callback del RT para gestionar eventos de control y estado del sistema.
 *
 * Esta función maneja eventos internos como el timeout de inactividad
 * para gestionar el bajo consumo (sueño profundo).
 *
 * @param ID_evento El evento que ha saltado (ej. ev_INACTIVIDAD, ev_PULSAR_BOTON).
 * @param auxiliar Datos auxiliares del evento.
 */
void rt_GE_actualizar(EVENTO_T ID_evento, uint32_t auxiliar);

void rt_GE_suscribir(EVENTO_T ID_evento, uint8_t prioridad, f_callback_GE f_callback);

void rt_GE_cancelar(EVENTO_T ID_evento, f_callback_GE f_callback);

#endif /* RT_GE_H */
