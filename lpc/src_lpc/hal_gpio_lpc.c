/* *****************************************************************************
 * P.H.2024: GPIOs en LPC2105, Timer 0 y Timer 1
 * implementacion para cumplir el hal_gpio.h
 * interrupciones externas para los botones lo dejamos para otro modulo aparte
 */
 
#include <LPC210x.H>                       /* LPC210x definitions */

#include "hal_gpio.h"

/**
 * Permite emplear el GPIO y debe ser invocada antes
 * de poder llamar al resto de funciones de la biblioteca.
 * re-configura todos los pines como de entrada (para evitar cortocircuitos)
 */
 
void hal_gpio_iniciar(void){
  // Reiniciamos los pines todos como salida (igual al reset):
  IODIR = 0x0; // GPIO Port Direction control register.
				       // Controla la dirección de cada puerto pin
}

/* *****************************************************************************
 * Acceso a los GPIOs 
 */

/**
 * El gpio se configuran como entrada o salida según la dirección.
 */
void hal_gpio_sentido(HAL_GPIO_PIN_T gpio, hal_gpio_pin_dir_t direccion){
	uint32_t masc = (1UL << gpio);
	if (direccion == HAL_GPIO_PIN_DIR_INPUT){
			IODIR = IODIR & ~masc;
	}
	else if (direccion == HAL_GPIO_PIN_DIR_OUTPUT){
			IODIR = IODIR | masc;
	}		
}
 
/**
 * La función devuelve un entero (bool) con el valor de los bits indicados.
 */
uint32_t hal_gpio_leer(HAL_GPIO_PIN_T gpio){
	uint32_t masc = (1UL << gpio);
	return ((IOPIN & masc)!=0);
}


/**
 * Escribe en el gpio el valor
 */
void hal_gpio_escribir(HAL_GPIO_PIN_T gpio, uint32_t valor){
	uint32_t masc = (1UL << gpio);
	
	if ((valor & 0x01) == 0) IOCLR = masc;
	else IOSET = masc;
}



