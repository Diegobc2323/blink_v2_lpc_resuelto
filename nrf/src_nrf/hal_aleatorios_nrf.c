#include "drv_aleatorios.h"
#include <nrf.h>
#include <stddef.h>

// Definiciones locales si no están en nrf.h para tu versión de CMSIS
#ifndef NRF_RNG
    #error "Board header not included or NRF_RNG not defined"
#endif

/**
 * @brief Función interna para obtener un solo byte del hardware.
 * Bloqueante (espera unos microsegundos hasta que el ruido térmico genera el dato).
 */
static uint8_t obtener_byte_hw(void) {
    // 1. Limpiar el evento de "Valor Listo" previo para poder detectar el nuevo
    NRF_RNG->EVENTS_VALRDY = 0;
    
    // 2. Esperar a que el hardware genere un nuevo byte aleatorio
    // El nRF52 tarda un tiempo variable debido al ruido térmico.
    while (NRF_RNG->EVENTS_VALRDY == 0) {
        // Espera activa (polling)
    }

    // 3. Leer el valor generado
    return (uint8_t)(NRF_RNG->VALUE & 0xFF);
}

void hal_aleatorios_iniciar(uint32_t semilla) {
    // NOTA: La 'semilla' se ignora porque el nRF52 usa un True RNG (TRNG) físico.
    (void)semilla; 

    // 1. Detener el RNG por si estaba corriendo
    NRF_RNG->TASKS_STOP = 1;

    // 2. Configurar la corrección de sesgo (Bias Correction)
    // Esto hace la generación un poco más lenta pero matemáticamente más uniforme.
    NRF_RNG->CONFIG = (RNG_CONFIG_DERCEN_Enabled << RNG_CONFIG_DERCEN_Pos);

    // 3. Habilitar la interrupción es opcional. 
    // Para este driver simple usaremos polling (espera activa) ya que es muy rápido.
    NRF_RNG->INTENCLR = 0xFFFFFFFF; // Deshabilitar interrupciones del RNG

    // 4. Arrancar el generador
    NRF_RNG->TASKS_START = 1;
}

uint32_t hal_aleatorios_obtener_32bits(void) {
    uint32_t resultado = 0;

    // El hardware del nRF52 da 8 bits. Para llenar 32 bits, leemos 4 veces.
    // Byte 0
    resultado |= obtener_byte_hw();
    // Byte 1
    resultado |= (obtener_byte_hw() << 8);
    // Byte 2
    resultado |= (obtener_byte_hw() << 16);
    // Byte 3
    resultado |= (obtener_byte_hw() << 24);

    return resultado;
}

uint32_t hal_aleatorios_rango(uint32_t rango) {
    if (rango == 0) return 0;

    // Obtenemos un numero de 32 bits y aplicamos módulo.
    // Nota: El modulo introduce un ligero sesgo si (2^32) no es múltiplo de rango,
    // pero para un juego es totalmente despreciable.
    uint32_t aleatorio = drv_aleatorios_obtener_32bits();
    
    return (aleatorio % rango);
}