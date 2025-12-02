/* *****************************************************************************
 * P.H.2025: Driver/Manejador de los Monitores
 * 
 *
 * - API idempotente: drv_monitor_establecer(estado), drv_monitor_estado, drv_monitor_conmutar
 * - Independiente de hardware v�a HAL + board.h
 *
 * Requiere de board.h:
 *   - MONITOR_NUMBER           : n� de monitores disponibles (0 si ninguno)
 *   - MONITOR_LIST             : lista de pines/ids GPIO (ej: {MONITOR1_GPIO, MONITOR2_GPIO, ...})
 *   - MONITOR_ACTIVE_STATE     : 1 si activo a nivel alto, 0 si activo a nivel bajo
 *
 * Requiere de hal_gpio.h:
 *   - hal_gpio_sentido(pin, HAL_GPIO_PIN_DIR_*)
 *   - hal_gpio_escribir(pin, valor)
 *   - hal_gpio_leer(pin)            => 0/1
 */

#include "hal_gpio.h"
#include "drv_monitor.h"
#include "board.h"


#if MONITOR_NUMBER > 0
	static const HAL_GPIO_PIN_T s_monitor_list[MONITOR_NUMBER] = MONITOR_LIST;
#endif

/* Helpers ------------------------------------------------------------------ */
static inline int monitor_id_valido(MONITOR_id_t id) {
#if MONITOR_NUMBER > 0
    return (id >= 1 && id <= (MONITOR_id_t)MONITOR_NUMBER);
#else
    (void)id;
    return 0;
#endif
}

static inline uint32_t hw_level_from_status(MONITOR_status_t st) {
#if MONITOR_NUMBER > 0
    /* Si activo-alto: ON->1, OFF->0; si activo-bajo: ON->0, OFF->1 */
    return (MONITOR_ACTIVE_STATE ? (st == MONITOR_ON) : (st == MONITOR_OFF));
#else
    (void)st;
    return 0;
#endif
}

static inline MONITOR_status_t status_from_hw_level(uint32_t level) {
#if MONITOR_NUMBER > 0
    /* Inversa del mapeo anterior */
    int on = MONITOR_ACTIVE_STATE ? (level != 0u) : (level == 0u);
    return on ? MONITOR_ON : MONITOR_OFF;
#else
    (void)level;
    return MONITOR_OFF;
#endif
}

/* API ---------------------------------------------------------------------- */

unsigned int drv_monitor_iniciar(){
	#if MONITOR_NUMBER > 0
    for (MONITOR_id_t i = 1; i <= (MONITOR_id_t)MONITOR_NUMBER; ++i) {
        hal_gpio_sentido(s_monitor_list[i-1], HAL_GPIO_PIN_DIR_OUTPUT);
        /* Apagar por defecto respetando activo-alto/bajo */
        hal_gpio_escribir(s_monitor_list[i-1], hw_level_from_status(MONITOR_OFF));
    }
  #endif //MONITOR_NUMBER > 0	
	
	return (unsigned int)MONITOR_NUMBER;  //definido en board_xxx.h en cada placa... 
}

int drv_monitor_establecer(MONITOR_id_t id, MONITOR_status_t estado) {
#if MONITOR_NUMBER > 0
    if (!monitor_id_valido(id)) return 0;
    hal_gpio_escribir(s_monitor_list[id-1], hw_level_from_status(estado));
    return 1;
#else
    (void)id; (void)estado;
    return 0;
#endif
}

int drv_monitor_estado(MONITOR_id_t id, MONITOR_status_t *out_estado) {
#if MONITOR_NUMBER > 0
    if (!monitor_id_valido(id) || !out_estado) return 0;
    int lvl = hal_gpio_leer(s_monitor_list[id-1]);
    *out_estado = status_from_hw_level(lvl);
    return 1;
#else
    (void)id; (void)out_estado;
    return 0;
#endif
}

void drv_monitor_marcar(MONITOR_id_t id) {
#if MONITOR_NUMBER > 0
    if (!monitor_id_valido(id)) return;
    /* Leemos estado l�gico, invertimos, escribimos (idempotente y consistente con activo-bajo/alto) */
    drv_monitor_establecer(id, MONITOR_ON);
#else
    (void)id;
    return;
#endif
}

void drv_monitor_desmarcar(MONITOR_id_t id) {
#if MONITOR_NUMBER > 0
    if (!monitor_id_valido(id)) return;
    /* Leemos estado l�gico, invertimos, escribimos (idempotente y consistente con activo-bajo/alto) */
    drv_monitor_establecer(id,MONITOR_OFF);
#else
    (void)id;
    return;
#endif
}

//otras???
