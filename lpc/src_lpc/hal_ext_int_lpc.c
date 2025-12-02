/* *****************************************************************************
 * P.H.2025: HAL EXT INT para LPC2105
 */

#include <LPC210x.H>
#include <stdint.h>
#include "hal_ext_int.h"
#include "hal_gpio.h"
#include "board.h"

//Mapeo de botones a GPIO y a líneas EINT
#if BUTTONS_NUMBER > 0
static const uint8_t s_buttons[BUTTONS_NUMBER] = BUTTONS_LIST;
#endif

typedef enum {
  pin_EINT0 = 16,  // Pin para interrupción externa 0
  pin_EINT1 = 14,  // Pin para interrupción externa 1
  pin_EINT2 = 15   // Pin para interrupción externa 2
} pin_EINT;

//Declaraciones de handlers
static __irq void EINT0_IRQHandler(void);
static __irq void EINT1_IRQHandler(void);
static __irq void EINT2_IRQHandler(void);

/* Función de Callback */
static void (*f_callback_drv)() = 0;

//Función auxiliar: pin → EINT
static uint8_t pin_to_eint(uint8_t pin)
{
    if (pin == pin_EINT1) return 1; // EINT1
    if (pin == pin_EINT2) return 2; // EINT2
    if (pin == pin_EINT0) return 0; // EINT0
    return 0xFF;
}

//Función auxiliar: pin → ID de botón (0-based)
static uint8_t pin_to_button_id(uint8_t pin)
{
    for (uint8_t i = 0; i < BUTTONS_NUMBER; i++) {
        if (s_buttons[i] == pin) {
            return i;
        }
    }
    return 0xFF;
}

//Inicialización 
void hal_ext_int_iniciar(hal_ext_int_callback_t callback)
{
    f_callback_drv = callback;

    for(int i = 0; i < BUTTONS_NUMBER; i++) {
        uint8_t pin = s_buttons[i];
        
        // Solo hay tres pines que se pueden configurar para que generen
        // interrupciones externas, si no es alguno de ellos no se hace nada.
        if (pin == pin_EINT1){
            // P0.14 -> EINT1 (bits 29:28 = 10)
            PINSEL0 = (PINSEL0 & ~(3UL << 28)) | (2UL << 28);
            // Limpia posibles interrupciones pendientes antes de habilitarlas.
            EXTINT = (1 << 1);
            // Se configura para que despierte al procesador en modo bajo consumo.
            EXTWAKE |= (1 << 1);
            // Configurar el VIC (Vectored Interrupt Controller).
            VICVectCntl2 = 0x20 | 15;
            VICVectAddr2 = (unsigned long)EINT1_IRQHandler;
            VICIntEnable = (1 << 15);
        
        } else if (pin == pin_EINT2){
            // P0.15 -> EINT2 (bits 31:30 = 10)
            PINSEL0 = (PINSEL0 & ~(3UL << 30)) | (2UL << 30);
            // Limpia posibles interrupciones pendientes antes de habilitarlas.
            EXTINT = (1 << 2);
            // Se configura para que despierte al procesador en modo bajo consumo.
            EXTWAKE |= (1 << 2);
            // Configurar el VIC (Vectored Interrupt Controller).
            VICVectCntl3 = 0x20 | 16;
            VICVectAddr3 = (unsigned long)EINT2_IRQHandler;
            VICIntEnable = (1 << 16);

        } else if (pin == pin_EINT0){
            // P0.16 -> EINT0 (bits 1:0 = 01)
            PINSEL1 = (PINSEL1 & ~(3 << 0)) | (1 << 0);
            // Limpia posibles interrupciones pendientes antes de habilitarlas.
            EXTINT = (1 << 0);
            // Se configura para que despierte al procesador en modo bajo consumo.
            EXTWAKE |= (1 << 0);
            // Configurar el VIC (Vectored Interrupt Controller).
            VICVectCntl4 = 0x20 | 14;
            VICVectAddr4 = (unsigned long)EINT0_IRQHandler;
            VICIntEnable = (1 << 14);
        }

    }
}

//Habilitamos interrupciones
void hal_ext_int_habilitar(uint32_t id_linea)
{
    if (id_linea >= BUTTONS_NUMBER) return;
    
    uint8_t pin = s_buttons[id_linea];

    if (pin == pin_EINT1){
        // P0.14 -> EINT1 (bits 29:28 = 10)
        PINSEL0 = (PINSEL0 & ~(3UL << 28)) | (2UL << 28);
        // Limpia posibles interrupciones pendientes antes de habilitarlas.
        EXTINT = (1 << 1);
        VICIntEnable = (1 << 15);
    
    } else if (pin == pin_EINT2){
        // P0.15 -> EINT2 (bits 31:30 = 10)
        PINSEL0 = (PINSEL0 & ~(3UL << 30)) | (2UL << 30);
        // Limpia posibles interrupciones pendientes antes de habilitarlas.
        EXTINT = (1 << 2);
        VICIntEnable = (1 << 16);

    } else if (pin == pin_EINT0){
        // P0.16 -> EINT0 (bits 1:0 = 01)
        PINSEL1 = (PINSEL1 & ~(3 << 0)) | (1 << 0);
        // Limpia posibles interrupciones pendientes antes de habilitarlas.
        EXTINT = (1 << 0);
        VICIntEnable = (1 << 14);
    }
}

//Deshabilitamos las interrupciones
void hal_ext_int_deshabilitar(uint32_t id_linea)
{
    if (id_linea >= BUTTONS_NUMBER) return;
    
    uint8_t pin = s_buttons[id_linea];
    
    if (pin == pin_EINT1){
        VICIntEnClr = (1 << 15); 
        // P0.14. Bits 28 y 29 de PINSEL0 a 00.
        EXTINT = (1 << 1);
        PINSEL0 &= ~(3UL << 28); 
    
    } else if (pin == pin_EINT2){
        VICIntEnClr = (1 << 16); 
        // P0.15. Bits 30 y 31 de PINSEL0 a 00.
        EXTINT = (1 << 2);
        PINSEL0 &= ~(3UL << 30); 

    } else if (pin == pin_EINT0){
        VICIntEnClr = (1 << 14); 
        // P0.16. Bits 0 y 1 de PINSEL1 a 00.
        EXTINT = (1 << 0);
        PINSEL1 &= ~(3UL << 0);
    }
}

//Limpiamos interrupciones pendientes
void hal_ext_int_limpiar_pendiente(uint32_t id_linea)
{
    if (id_linea >= BUTTONS_NUMBER) return;
    
    uint8_t pin = s_buttons[id_linea];
    uint8_t eint = pin_to_eint(pin);
    
    if (eint != 0xFF) {
        EXTINT = (1 << eint);
    }
}


// ISR para EINT1 (P0.14)
__irq void EINT1_IRQHandler(void) {
        VICIntEnClr = (1 << 15); 
    // CRÍTICO: Limpiar flag ANTES del callback (Level-Sensitive)
    EXTINT = (1 << 1);
    
    // Traducir pin a ID de botón y ejecutar callback
    if (f_callback_drv) {
        uint8_t id_boton = pin_to_button_id(pin_EINT1);
        if (id_boton != 0xFF) {
            f_callback_drv(id_boton);
        }
    }
    
    // Acknowledge
    VICVectAddr = 0;
}

// ISR para EINT2 (P0.15)
__irq void EINT2_IRQHandler(void) {
    VICIntEnClr = (1 << 16); 
    
    // CRÍTICO: Limpiar flag ANTES del callback (Level-Sensitive)
    EXTINT = (1 << 2);
    
    // Traducir pin a ID de botón y ejecutar callback
    if (f_callback_drv) {
        uint8_t id_boton = pin_to_button_id(pin_EINT2);
        if (id_boton != 0xFF) {
            f_callback_drv(id_boton);
        }
    }
    
    // Acknowledge
    VICVectAddr = 0;
}

// ISR para EINT0 (P0.16)
__irq void EINT0_IRQHandler(void) {
    VICIntEnClr = (1 << 14); 
    
    // CRÍTICO: Limpiar flag ANTES del callback (Level-Sensitive)
    EXTINT = (1 << 0);
    
    // Traducir pin a ID de botón y ejecutar callback
    if (f_callback_drv) {
        uint8_t id_boton = pin_to_button_id(pin_EINT0);
        if (id_boton != 0xFF) {
            f_callback_drv(id_boton);
        }
    }

    // Acknowledge
    VICVectAddr = 0;
}
