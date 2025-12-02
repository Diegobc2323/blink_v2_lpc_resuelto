#ifndef SVC_ALARMAS_H
#define SVC_ALARMAS_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "rt_evento_t.h"


/**
 * @brief Inicializa el servicio de alarmas software.
 *
 * @param monitor_overflow ID del monitor a marcar en caso de desbordamiento de slots.
 * @param funcion_callback_app Puntero a la función para encolar eventos (ej. rt_FIFO_encolar).
 * @param ev_a_notificar Evento que dispara la actualización periódica (ej. ev_T_PERIODICO).
 */
void svc_alarma_iniciar(uint32_t monitor_overflow, void(*funcion_callback_app)(uint32_t, uint32_t), EVENTO_T ev_a_notificar);

/**
 * @brief Codifica los parámetros de una alarma en un flag de 32 bits.
 *
 * @param periodico true si la alarma debe ser periódica, false si es esporádica.
 * @param retardo_ms Retardo en milisegundos (máximo 24 bits).
 * @param flags 7 bits de flags de usuario (reservados para uso futuro).
 * @return Un entero de 32 bits con todos los parámetros codificados.
 */
uint32_t svc_alarma_codificar(bool periodico, uint32_t retardo_ms, uint8_t flags);

/**
 * @brief Activa, desactiva o reprograma una alarma.
 *
 * @param alarma_flags Los flags codificados (obtenidos de svc_alarma_codificar). Si es 0, cancela la alarma.
 * @param ID_evento El evento a encolar en el Gestor de Eventos cuando venza la alarma.
 * @param auxData Datos auxiliares (payload) para dicho evento.
 */
void svc_alarma_activar(uint32_t alarma_flags, EVENTO_T ID_evento, uint32_t auxData);

/**
 * @brief Función de actualización del servicio de alarmas (tick handler).
 *
 * Esta función es llamada periódicamente por el Gestor de Eventos para decrementar
 * los contadores de las alarmas y disparar los eventos vencidos.
 *
 * @param evento El evento de tick que ha saltado (debe ser el ev_a_notificar).
 * @param aux Datos auxiliares del evento (no se usan en esta función).
 */
void svc_alarma_actualizar(EVENTO_T evento, uint32_t aux);


#endif /* SVC_ALARMAS_H */
