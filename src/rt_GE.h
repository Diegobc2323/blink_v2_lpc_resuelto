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

/**
 * @brief Suscribe un callback a un tipo de evento especifico.
 *
 * Registra una funcion callback para ser invocada cuando ocurra un evento
 * del tipo indicado. Los callbacks se almacenan ordenados por prioridad
 * (menor numero = mayor prioridad, se ejecutan primero).
 *
 * Si se alcanza el maximo de suscriptores (rt_GE_MAX_SUSCRITOS), el sistema
 * marca el monitor de overflow y se detiene con un bucle infinito.
 *
 * @param ID_evento Tipo de evento al que suscribirse (ver rt_evento_t.h).
 * @param prioridad Prioridad de ejecucion (0=alta, 255=baja). Los callbacks 
 *                  con menor numero se ejecutan primero.
 * @param f_callback Puntero a la funcion callback con firma: 
 *                   void callback(EVENTO_T evento, uint32_t auxData).
 */
void rt_GE_suscribir(EVENTO_T ID_evento, uint8_t prioridad, f_callback_GE f_callback);

/**
 * @brief Desuscribe un callback de un tipo de evento.
 *
 * Elimina una suscripcion previamente registrada con rt_GE_suscribir().
 * Busca el callback en la lista de suscritos y lo elimina, compactando
 * el array para mantener la lista contigua.
 *
 * Si el callback no esta suscrito al evento, la funcion no hace nada.
 *
 * @param ID_evento Tipo de evento del que desuscribirse.
 * @param f_callback Puntero al callback que se desea eliminar.
 */
void rt_GE_cancelar(EVENTO_T ID_evento, f_callback_GE f_callback);

#endif /* RT_GE_H */
