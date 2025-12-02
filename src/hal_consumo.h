/* *****************************************************************************
 * P.H.2025: HAL de consumo para nRF52 (System ON sleep)
 * Implementación de modos de bajo consumo usando instrucciones WFI.
 *****************************************************/
#ifndef HAL_CONSUMO_H
#define HAL_CONSUMO_H

#include <stdint.h>
#include <stdbool.h>
extern void switch_to_PLL();
/**
 * Inicializa el subsistema de consumo (si procede).
 * Para nRF52 no es necesario configurar nada especial para WFI.
 */
void hal_consumo_iniciar(void);

/**
 * Modo "esperar": duerme hasta la próxima interrupción manteniendo estado
 * (System ON sleep). Sale cuando entra cualquier IRQ habilitada.
 */
void hal_consumo_esperar(void);

/**
 * Modo "dormir": igual que esperar en esta versión (WFI).
 * En versiones futuras podría añadir medidas más agresivas.
 */
void hal_consumo_dormir(void);

#endif /* HAL_CONSUMO_H */
