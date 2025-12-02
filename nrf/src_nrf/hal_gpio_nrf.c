/* *****************************************************************************
 * P.H.2025: HAL GPIO Corregido para NRF52840 (Soporte Puerto 0 y 1)
 */
#include <nrf.h>
#include "hal_gpio.h"
#include <stdlib.h>

void hal_gpio_iniciar(void){
  // Reiniciamos pines a entrada (seguro)
  NRF_P0->DIR = 0x00; 
  NRF_P1->DIR = 0x00; 
}

/* *****************************************************************************
 * Acceso a los GPIOs 
 */

void hal_gpio_sentido(HAL_GPIO_PIN_T gpio, hal_gpio_pin_dir_t direccion){
    // Seleccionamos el puerto en base al número de pin (0-31 = P0, 32-63 = P1)
    NRF_GPIO_Type * port = (gpio > 31) ? NRF_P1 : NRF_P0;
    uint32_t pin = gpio & 0x1F; // Nos quedamos con los 5 bits bajos (0-31)
    uint32_t masc = (1UL << pin);

    if (direccion == HAL_GPIO_PIN_DIR_INPUT){
        port->DIRCLR = masc;
    }
    else if (direccion == HAL_GPIO_PIN_DIR_OUTPUT){
        port->DIRSET = masc;
    }       
}

uint32_t hal_gpio_leer(HAL_GPIO_PIN_T gpio){
    NRF_GPIO_Type * port = (gpio > 31) ? NRF_P1 : NRF_P0;
    uint32_t pin = gpio & 0x1F;
    uint32_t masc = (1UL << pin);

    // Leemos el registro IN del puerto correcto
    return ((port->IN & masc) != 0);
}


void hal_gpio_escribir(HAL_GPIO_PIN_T gpio, uint32_t valor){
    NRF_GPIO_Type * port = (gpio > 31) ? NRF_P1 : NRF_P0;
    uint32_t pin = gpio & 0x1F;
    uint32_t masc = (1UL << pin);

    if (valor == 0 ){
        port->OUTCLR = masc; // Apagamos (o ponemos a 0)
    }else{
        port->OUTSET = masc; // Encendemos (o ponemos a 1)
    }
}