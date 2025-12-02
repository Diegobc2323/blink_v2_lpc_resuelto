/* *****************************************************************************
 * P.H.2025: Driver/Manejador de los Leds
 * 
 * Suministra los servicios de iniciar, establecer estado, conmutar y consultar,
 * independientemente del hardware (vía HAL + board.h).
 *
 * - API idempotente: drv_led_establecer(id, estado), drv_led_estado, drv_led_conmutar
 * - IDs válidos: 1..LEDS_NUMBER (0 no es válido)
 *
 * Requiere de board.h:
 *   - LEDS_NUMBER           : número de leds disponibles (0 si ninguno)
 *   - LEDS_LIST             : lista de pines GPIO (ej: {LED1_GPIO, LED2_GPIO, ...})
 *   - LEDS_ACTIVE_STATE     : 1 si el LED está encendido con nivel alto, 0 si con nivel bajo
 *
 * Requiere de hal_gpio.h:
 *   - hal_gpio_sentido(HAL_GPIO_PIN_T gpio, hal_gpio_pin_dir_t dir)
 *   - hal_gpio_escribir(HAL_GPIO_PIN_T gpio, uint32_t valor)   // void
 *   - hal_gpio_leer(HAL_GPIO_PIN_T gpio) -> uint32_t (0/1)
 */

#ifndef DRV_LEDS_H
#define DRV_LEDS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Tipo fuerte para IDs de LED (1..N) */
typedef uint8_t LED_id_t;
#define LED_ID_INVALID ((LED_id_t)0xFF)

/* Estado lógico del LED */
typedef enum {
    LED_OFF = 0,
    LED_ON  = 1,
} LED_status_t;

/**
 * @brief Inicializa el subsistema de LEDs.
 *
 * Configura los GPIO como salida, apaga todos los LEDs y devuelve
 * el número de LEDs disponibles en la plataforma.
 */
unsigned int drv_leds_iniciar(void);

/**
 * @brief Establece el estado de un LED (idempotente).
 *
 * @param id     Identificador del LED (1..N).
 * @param estado LED_ON o LED_OFF.
 * @return 1 si ok, 0 si id fuera de rango.
 */
int drv_led_establecer(LED_id_t id, LED_status_t estado);

/**
 * @brief Obtiene el estado lógico de un LED.
 *
 * @param id         Identificador del LED (1..N).
 * @param out_estado Puntero de salida: se escribe LED_ON o LED_OFF.
 * @return 1 si ok (out_estado válido), 0 si id fuera de rango.
 */
int drv_led_estado(LED_id_t id, LED_status_t *out_estado);

/**
 * @brief Conmuta el estado de un LED (ON <-> OFF).
 *
 * @param id Identificador del LED (1..N).
 * @return 1 si ok, 0 si id fuera de rango.
 */
int drv_led_conmutar(LED_id_t id);

#if 0
/* ---- Funciones opcionales de alto nivel ---- */

/** Devuelve de nuevo el número total de LEDs disponibles */
unsigned int drv_leds_cantidad(void);

/** Enciende todos los LEDs disponibles */
void drv_leds_encender_todos(void);

/** Apaga todos los LEDs disponibles */
void drv_leds_apagar_todos(void);

#endif /* opcionales */

#ifdef __cplusplus
}
#endif

#endif /* DRV_LEDS_H */
