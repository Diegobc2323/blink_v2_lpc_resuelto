#ifndef DRV_SC_H
#define DRV_SC_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>


/**
 * @brief Entra en una sección crítica y deshabilita las interrupciones.
 *
 * Esta función anida llamadas, manteniendo las interrupciones deshabilitadas
 * hasta que la última llamada a drv_SC_salir_enable_irq se ejecute.
 *
 * @return El contador de nivel de anidamiento de la sección crítica (para depuración).
 */
uint32_t drv_SC_entrar_disable_irq(void);

/**
 * @brief Sale de una sección crítica y habilita las interrupciones.
 *
 * Si es la última salida anidada, las interrupciones se habilitan.
 */
void drv_SC_salir_enable_irq(void);


#endif /* DRV_SC_H */
