#ifndef HAL_SC_H
#define HAL_SC_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>


/**
 * @brief Entra en una sección crítica y deshabilita las interrupciones.
 *
 * Esta función anida llamadas, manteniendo las interrupciones deshabilitadas
 * hasta que la última llamada a drv_SC_salir_enable_irq se ejecute.
 *
 * 
 */
void deshabilitar_irq(void);

/**
 * @brief Sale de una sección crítica y habilita las interrupciones.
 *
 * Si es la última salida anidada, las interrupciones se habilitan.
 */
void habilitar_irq(void);


#endif /* HAL_SC_H */
