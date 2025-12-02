/* *****************************************************************************
 * P.H.2025: Driver/Manejador de los Leds
 * suministra los servicios de iniciar, encender, apagar, conmutar... independientemente del hardware
 *
 * - API idempotente: drv_led_establecer(id, estado), drv_led_estado, drv_led_conmutar
 * - IDs 1..LEDS_NUMBER
 * - Independiente de hardware vía HAL + board.h
 *
 * Requiere de board.h:
 *   - LEDS_NUMBER           : nº de leds disponibles (0 si ninguno)
 *   - LEDS_LIST             : lista de pines/ids GPIO (ej: {LED1_GPIO, LED2_GPIO, ...})
 *   - LEDS_ACTIVE_STATE     : 1 si activo a nivel alto, 0 si activo a nivel bajo
 *
 * Requiere de hal_gpio.h:
 *   - hal_gpio_sentido(pin, HAL_GPIO_PIN_DIR_*)
 *   - hal_gpio_escribir(pin, valor)
 *   - hal_gpio_leer(pin)            => 0/1
 */

#include "hal_gpio.h"
#include "drv_leds.h"
#include "board.h"


#if LEDS_NUMBER > 0
	static const HAL_GPIO_PIN_T s_led_list[LEDS_NUMBER] = LEDS_LIST;
#endif

/* Helpers ------------------------------------------------------------------ */
static inline int led_id_valido(LED_id_t id) {
#if LEDS_NUMBER > 0
    return (id >= 1 && id <= (LED_id_t)LEDS_NUMBER);
#else
    (void)id;
    return 0;
#endif
}

static inline uint32_t hw_level_from_status(LED_status_t st) {
#if LEDS_NUMBER > 0
    /* Si activo-alto: ON->1, OFF->0; si activo-bajo: ON->0, OFF->1 */
    return (LEDS_ACTIVE_STATE ? (st == LED_ON) : (st == LED_OFF));
#else
    (void)st;
    return 0;
#endif
}

static inline LED_status_t status_from_hw_level(uint32_t level) {
#if LEDS_NUMBER > 0
    /* Inversa del mapeo anterior */
    int on = LEDS_ACTIVE_STATE ? (level != 0u) : (level == 0u);
    return on ? LED_ON : LED_OFF;
#else
    (void)level;
    return LED_OFF;
#endif
}

/* API ---------------------------------------------------------------------- */

unsigned int drv_leds_iniciar(){
	#if LEDS_NUMBER > 0
    for (LED_id_t i = 1; i <= (LED_id_t)LEDS_NUMBER; ++i) {
        hal_gpio_sentido(s_led_list[i-1], HAL_GPIO_PIN_DIR_OUTPUT);
        /* Apagar por defecto respetando activo-alto/bajo */
        hal_gpio_escribir(s_led_list[i-1], hw_level_from_status(LED_OFF));
    }
  #endif //LEDS_NUMBER > 0	
	
	return (unsigned int)LEDS_NUMBER;  //definido en board_xxx.h en cada placa... 
}

int drv_led_establecer(LED_id_t id, LED_status_t estado) {
#if LEDS_NUMBER > 0
    if (!led_id_valido(id)) return 0;
    hal_gpio_escribir(s_led_list[id-1], hw_level_from_status(estado));
    return 1;
#else
    (void)id; (void)estado;
    return 0;
#endif
}

int drv_led_estado(LED_id_t id, LED_status_t *out_estado) {
#if LEDS_NUMBER > 0
    if (!led_id_valido(id) || !out_estado) return 0;
    int lvl = hal_gpio_leer(s_led_list[id-1]);
    *out_estado = status_from_hw_level(lvl);
    return 1;
#else
    (void)id; (void)out_estado;
    return 0;
#endif
}

int drv_led_conmutar(LED_id_t id) {
#if LEDS_NUMBER > 0
    if (!led_id_valido(id)) return 0;
    /* Leemos estado lógico, invertimos, escribimos (idempotente y consistente con activo-bajo/alto) */
    LED_status_t st;
    if (!drv_led_estado(id, &st)) return 0;
    return drv_led_establecer(id, (st == LED_ON) ? LED_OFF : LED_ON);
#else
    (void)id;
    return 0;
#endif
}

//otras???
