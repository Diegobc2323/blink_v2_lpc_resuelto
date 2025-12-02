/* *****************************************************************************
 * P.H.2025: Driver/Manejador de los Leds
 * 
 * Suministra los servicios de iniciar, establecer estado, conmutar y consultar,
 * independientemente del hardware (v�a HAL + board.h).
 *
 * - API idempotente: drv_monitor_establecer(id, estado), drv_monitor_estado, drv_monitor_conmutar
 * - IDs v�lidos: 1..MONITOR_NUMBER (0 no es v�lido)
 *
 * Requiere de board.h:
 *   - MONITOR_NUMBER           : n�mero de monitor disponibles (0 si ninguno)
 *   - MONITOR_LIST             : lista de pines GPIO (ej: {MONITOR1_GPIO, MONITOR2_GPIO, ...})
 *   - MONITOR_ACTIVE_STATE     : 1 si el MONITOR est� encendido con nivel alto, 0 si con nivel bajo
 *
 * Requiere de hal_gpio.h:
 *   - hal_gpio_sentido(HAL_GPIO_PIN_T gpio, hal_gpio_pin_dir_t dir)
 *   - hal_gpio_escribir(HAL_GPIO_PIN_T gpio, uint32_t valor)   // void
 *   - hal_gpio_leer(HAL_GPIO_PIN_T gpio) -> uint32_t (0/1)
 */

#ifndef DRV_MONITOR_H
#define DRV_MONITOR_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Tipo fuerte para IDs de MONITOR (1..N) */
typedef uint8_t MONITOR_id_t;
#define MONITOR_ID_INVALID ((MONITOR_id_t)0xFF)

/* Estado l�gico del MONITOR */
typedef enum {
    MONITOR_OFF = 0,
    MONITOR_ON  = 1,
} MONITOR_status_t;

/**
 * @brief Inicializa el subsistema de MONITORs.
 *
 * Configura los GPIO como salida, apaga todos los MONITORs y devuelve
 * el n�mero de MONITORs disponibles en la plataforma.
 */
unsigned int drv_monitor_iniciar(void);

/**
 * @brief Establece el estado de un MONITOR (idempotente).
 *
 * @param id     Identificador del MONITOR (1..N).
 * @param estado MONITOR_ON o MONITOR_OFF.
 * @return 1 si ok, 0 si id fuera de rango.
 */
int drv_monitor_establecer(MONITOR_id_t id, MONITOR_status_t estado);

/**
 * @brief Obtiene el estado l�gico de un MONITOR.
 *
 * @param id         Identificador del MONITOR (1..N).
 * @param out_estado Puntero de salida: se escribe MONITOR_ON o MONITOR_OFF.
 * @return 1 si ok (out_estado v�lido), 0 si id fuera de rango.
 */
int drv_monitor_estado(MONITOR_id_t id, MONITOR_status_t *out_estado);

/**
 * @brief Pone el estado de un MONITOR a ON.
 *
 * @param id Identificador del MONITOR (1..N).
 */
void drv_monitor_marcar(MONITOR_id_t id);

/**
 * @brief Pone el estado de un MONITOR a OFF.
 *
 * @param id Identificador del MONITOR (1..N).
 */
void drv_monitor_desmarcar(MONITOR_id_t id);


#ifdef __cplusplus
}
#endif

#endif /* DRV_MONITOR_H */
