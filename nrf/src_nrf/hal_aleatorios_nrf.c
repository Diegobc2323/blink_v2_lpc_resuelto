#include "drv_aleatorios.h"
#include <nrf.h>
#include <stddef.h>

// Definiciones locales si no están en nrf.h para tu versión de CMSIS
#ifndef NRF_RNG
    #error "Board header not included or NRF_RNG not defined"
#endif

/**
 * @brief Función interna para obtener un solo byte del hardware.
 * Utiliza __WFE() (Wait For Event) para dormir el procesador hasta que el dato esté listo.
 * Esto elimina la espera activa y reduce el consumo de energía.
 */
static uint8_t obtener_byte_hw(void) {
    // Limpiar el evento de "Valor Listo" previo
    NRF_RNG->EVENTS_VALRDY = 0;
    
    // Esperar a que el hardware genere un nuevo byte aleatorio
    // El bucle comprueba la condición por si nos despertamos por otro evento (ruido)
    while (NRF_RNG->EVENTS_VALRDY == 0) {
        // Dormir el procesador hasta que ocurra un evento.
        // El periférico RNG generará un evento VALRDY cuando termine, despertando a la CPU.
        __WFE();
    }

    // Leer el valor generado
    return (uint8_t)(NRF_RNG->VALUE & 0xFF);
}

void hal_aleatorios_iniciar(uint32_t semilla) {
    // La semilla se ignora porque el nRF52 usa un True RNG (TRNG) físico.
    (void)semilla; 

    //Detener el RNG por si estaba corriendo
    NRF_RNG->TASKS_STOP = 1;

    // Configurar la corrección de sesgo (Bias Correction)
    NRF_RNG->CONFIG = (RNG_CONFIG_DERCEN_Enabled << RNG_CONFIG_DERCEN_Pos);

    // Habilitar la interrupción es opcional. 
    // Al usar __WFE(), no necesitamos habilitar la interrupción en el NVIC,
    // el evento hardware es suficiente para despertar al procesador del sueño.
    NRF_RNG->INTENCLR = 0xFFFFFFFF; 

    // Arrancar el generador
    NRF_RNG->TASKS_START = 1;
}

uint32_t hal_aleatorios_obtener_32bits(void) {
    uint32_t resultado = 0;

    // El hardware del nRF52 da 8 bits. Para llenar 32 bits, leemos 4 veces.
    // Cada llamada dormirá el procesador brevemente hasta tener el byte.
    resultado |= obtener_byte_hw();
    resultado |= (obtener_byte_hw() << 8);
    resultado |= (obtener_byte_hw() << 16);
    resultado |= (obtener_byte_hw() << 24);

    return resultado;
}

uint32_t hal_aleatorios_rango(uint32_t rango) {
    if (rango == 0) return 0;

    // Obtenemos un numero de 32 bits y aplicamos módulo.
    uint32_t aleatorio = hal_aleatorios_obtener_32bits();
    
    return (aleatorio % rango);
}
