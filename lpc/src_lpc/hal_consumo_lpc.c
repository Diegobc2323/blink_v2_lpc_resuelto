#include "hal_consumo.h"
 
#include <LPC21xx.H>

/**
 * Inicializa el subsistema de consumo (si procede).
 * Para nRF52 no es necesario configurar nada especial para WFI.
 */
void hal_consumo_iniciar(void){
	/* En esta práctica no es necesario configurar registros de energía.
		 Dejamos la función para homogeneizar la API entre plataformas. */
    EXTWAKE = 0x0F;
}

/**
 * Modo "esperar": duerme hasta la próxima interrupción manteniendo estado
 * (System ON sleep). Sale cuando entra cualquier IRQ habilitada.
 */
void hal_consumo_esperar(void){
	EXTWAKE = 0x0F;

	PCON |= 0X01;
}

/**
 * Modo "dormir": igual que esperar en esta versión (WFI).
 * En versiones futuras podría añadir medidas más agresivas.
 */
void hal_consumo_dormir(void){
	EXTWAKE = 0x0F;
	PCON |= 0X02;
	switch_to_PLL();
}
