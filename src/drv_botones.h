#ifndef DRV_BOTONES_H
#define DRV_BOTONES_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "rt_evento_t.h"
#include "drv_monitor.h"
#include "drv_tiempo.h"
#include "svc_alarmas.h"
#include "hal_gpio.h"
#include "hal_ext_int.h"


/**
 * @brief Inicializa el driver de botones.
 *
 * Configura la FSM, el HAL de interrupciones externas y suscribe
 * drv_botones_actualizar al Gestor de Eventos.
 *
 * @param funcion_callback_app Puntero a la función para encolar eventos (ej. rt_FIFO_encolar).
 * @param ev1_pulsar          Evento a encolar cuando se confirma una pulsación (ev_PULSAR_BOTON).
 * @param ev2_tiempo          Evento usado internamente por la FSM para las alarmas (ev_BOTON_TIMER).
 */
void drv_botones_iniciar(void(*funcion_callback_app)(uint32_t, uint32_t), EVENTO_T ev_pulsar, EVENTO_T ev_soltar,EVENTO_T ev2_tiempo); // <--- NUEVO ARGUMENTOEVENTO_T ev_tiempo);
/**
 * @brief Función de actualización (callback) para la máquina de estados de los botones.
 *
 * Esta función es el callback de suscripción, llamado por el dispatcher del
 * Gestor de Eventos  cuando vence una alarma de retardo (Trp, Tep, Trd).
 *
 * @param evento        El evento que ha saltado (debe ser el ev2_tiempo o ev1_pulsar).
 * @param auxiliar      Datos auxiliares (contiene el ID lógico del botón).
 */
void drv_botones_actualizar(EVENTO_T evento, uint32_t auxiliar);


#endif /* DRV_BOTONES_H */
