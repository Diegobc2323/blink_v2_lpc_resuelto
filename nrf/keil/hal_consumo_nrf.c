/* *****************************************************************************
 * HAL de consumo: implementación de las funciones para gestionar el bajo consumo
 * del procesador entre parpadeos del LED en lugar de la espera activa del bucle.
 * PARA NRF
 * ****************************************************************************/

#include "hal_consumo.h"
#include "board.h"
#include <nrf.h> // Fichero CMSIS de Nordic, que define __WFI()

/* *****************************************************************************
* Inicializa el subsistema de consumo
*/
void hal_consumo_iniciar(void) {
    /* YA LO HACEMOS EN DORMIR, AQUÍ NO HARÍA FALTA!!!!
    // 1. Configurar los 4 botones como pines de "despertar" (wake-up).
    //    Para despertar de System OFF, el nRF usa la detección de nivel (SENSE).
    
    #if (BUTTON_PULL == 1)
        #define _PULL_CONFIG (GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos)
    #else
        #define _PULL_CONFIG (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos)
    #endif

    const uint32_t WAKEUP_PIN_CNF = 
        (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
        (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) |
        _PULL_CONFIG |
        (GPIO_PIN_CNF_SENSE_Low << GPIO_PIN_CNF_SENSE_Pos); // <-- Clave: despertar por nivel bajo

    // Asumimos que todos los botones están en el Puerto 0 (como en la DK)
    NRF_P0->PIN_CNF[BUTTON_1] = WAKEUP_PIN_CNF;
    NRF_P0->PIN_CNF[BUTTON_2] = WAKEUP_PIN_CNF;
    NRF_P0->PIN_CNF[BUTTON_3] = WAKEUP_PIN_CNF;
    NRF_P0->PIN_CNF[BUTTON_4] = WAKEUP_PIN_CNF;
    */
}

/* *****************************************************************************
* Detiene la CPU hasta la próxima interrupción
*/
void hal_consumo_esperar(void) {
    //Instrucción Wait For Interrupt, para que el procesador se duerma hasta recibir una interrupción
    //Al despertar, continua la ehecución en la siguiente línea de código
    __WFI();
	}

/* *****************************************************************************
* Detiene la CPU hasta la próxima interrupción (dormir profundo).
*/
/* *****************************************************************************
* Detiene la CPU hasta la próxima interrupción (dormir profundo).
*/
void hal_consumo_dormir(void) {
    
    // --- NUEVO: APAGAR LOS LEDS PARA AHORRAR ENERGÍA ---
    // Como en tu board.h los LEDs son Active Low (0 encendido, 1 apagado),
    // escribir un '1' en el registro OUTSET los apaga.
    // También podríamos ponerlos como entrada (DIRCLR), pero esto es más seguro
    // para mantener el nivel lógico correcto.
    
    // Creamos una máscara con los bits de los 4 LEDs
    uint32_t led_mask = (1UL << LED_1) | (1UL << LED_2) | (1UL << LED_3) | (1UL << LED_4);
    
    // Escribimos 1s en esos pines para apagarlos (Active Low)
    NRF_P0->OUTSET = led_mask; 
    
    // Opcional: Si prefieres desconectarlos totalmente (Alta impedancia)
    // NRF_P0->DIRCLR = led_mask; 
    // ----------------------------------------------------

    // 1. Configurar los 4 botones como pines de "despertar" (wake-up).
    #if (BUTTON_PULL == 1)
        #define _PULL_CONFIG (GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos)
    #else
        #define _PULL_CONFIG (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos)
    #endif

    const uint32_t WAKEUP_PIN_CNF = 
        (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
        (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) |
        _PULL_CONFIG |
        (GPIO_PIN_CNF_SENSE_Low << GPIO_PIN_CNF_SENSE_Pos);

    NRF_P0->PIN_CNF[BUTTON_1] = WAKEUP_PIN_CNF;
    NRF_P0->PIN_CNF[BUTTON_2] = WAKEUP_PIN_CNF;
    NRF_P0->PIN_CNF[BUTTON_3] = WAKEUP_PIN_CNF;
    NRF_P0->PIN_CNF[BUTTON_4] = WAKEUP_PIN_CNF;

    // 2. Limpiar el LATCH
    NRF_P0->LATCH = (1UL << BUTTON_1) | (1UL << BUTTON_2) | (1UL << BUTTON_3) | (1UL << BUTTON_4);

    // 3. Activar el modo System OFF
    NRF_POWER->SYSTEMOFF = 1;

    // 4. Esperar
    __WFE();
    
    while(1);
}
