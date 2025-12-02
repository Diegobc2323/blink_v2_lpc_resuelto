#ifndef APP_JUEGO_H
#define APP_JUEGO_H

#include "rt_evento_t.h" // Para EVENTO_T y uint32_t

/**
 * @brief Inicializa el módulo del juego y suscribe sus tareas.
 */
void app_juego_iniciar(void);

/**
 * @brief Máquina de estados del juego (FSM). 
 * Se suscribe al Gestor de Eventos para ser llamada.
 */
void app_juego_actualizar(EVENTO_T evento, uint32_t auxData);

#endif // APP_JUEGO_H
