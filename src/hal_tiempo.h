/* *****************************************************************************
 * P.H.2025: HAL de tiempo (tick libre + reloj periódico por IRQ)
 * Cambios 2025:
 *  - init devuelve estructura con parámetros (no un entero suelto).
 *  - API del periódico separa config/enable/callback.
 */
 
#ifndef HAL_TIEMPO
#define HAL_TIEMPO

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t ticks_per_us;   /* ticks de HW por microsegundo (p.ej., 15 a 15 MHz) */
    uint8_t  counter_bits;   /* ancho del contador libre (p.ej., 32) */
    uint32_t counter_max;    /* (1u<<counter_bits)-1 */
} hal_tiempo_info_t;

/**
 * Inicializa el contador libre (tick) y devuelve parámetros de conversión 
 * entre ellas, devuelve la constante ticks_per_us,
 * ticks_per_us permite pasar de los ticks del hardware a microsegundos
 * (tip, el driver lo necesitara para trabajar en us y ms de la app y hacer la conversion a ticks del hardware)
 */
void hal_tiempo_iniciar_tick(hal_tiempo_info_t *out_info);

/* Lectura del tick de 64 bits */
uint64_t hal_tiempo_actual_tick64(void);


/* --- Reloj periódico por IRQ --- */

/* Configura el periodo en ticks (no arranca) */
void hal_tiempo_periodico_config_tick(uint32_t periodo_en_tick);

/* Registra el callback llamado desde la IRQ */
void hal_tiempo_periodico_set_callback(void (*cb)());

/* Habilita/Deshabilita el reloj periódico */
void hal_tiempo_periodico_enable(bool enable);

void hal_tiempo_reloj_periodico_tick(uint32_t periodo_en_tick,void (*funcion_callback_drv)());

#endif // HAL_TIEMPO
