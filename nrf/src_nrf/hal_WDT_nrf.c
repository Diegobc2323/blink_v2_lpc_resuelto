#include "hal_WDT.h"
#include "nrf.h" // Cabecera estándar del CMSIS para registros nRF

/**
 * @brief Inicializa el WDT en el nRF52840.
 *
 * Configura el tiempo de timeout y habilita el primer canal de recarga.
 */
void hal_WDT_iniciar(uint32_t sec) {
    
    // El WDT del nRF usa el reloj de baja frecuencia (LFCLK, 32.768 kHz).
    // CRV (Counter Reload Value) = (Tiempo en segundos * Frecuencia) - 1.
    NRF_WDT->CRV = (sec * 32768) - 1;
    
    // Habilitar el primer canal de recarga (RR[0]).
    NRF_WDT->RREN = (WDT_RREN_RR0_Enabled << WDT_RREN_RR0_Pos);
    
    // Configurar el WDT para que siga corriendo en modo Sleep (necesario para el IDLE/esperar).
    NRF_WDT->CONFIG = (WDT_CONFIG_SLEEP_Run << WDT_CONFIG_SLEEP_Pos);
    
    // Iniciar el temporizador Watchdog
    NRF_WDT->TASKS_START = 1;
}

/**
 * @brief Alimenta el WDT en el nRF52840.
 *
 * Escribe el valor de recarga específico en el registro de la sub-tarea de recarga (RR[0]).
 */
void hal_WDT_feed(void) {
    // 0x6E524635 es el valor fijo (WDT_RR_RR_Reload) requerido para recargar el contador RR[0].
    NRF_WDT->RR[0] = 0x6E524635;
}


