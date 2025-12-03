/* *****************************************************************************
 * P.H.2025: HAL EXT INT para LPC2105
 */

#include <LPC210x.H>
#include <stdint.h>
#include "hal_ext_int.h"
#include "hal_gpio.h"
#include "board.h"

// Lista de pines asociados a los botones definidos en board.h
#if BUTTONS_NUMBER > 0
static const uint8_t g_lista_pines[BUTTONS_NUMBER] = BUTTONS_LIST;
#endif

typedef enum {
  PIN_EINT_0 = 16,  // Pin para interrupción externa 0
  PIN_EINT_1 = 14,  // Pin para interrupción externa 1
  PIN_EINT_2 = 15   // Pin para interrupción externa 2
} pin_EINT;

__irq void interrupcion_EINT_0(void) {
    VICIntEnClr = (1 << 14); 
    
    //limpliamos flag antes del callback
    EXTINT = (1 << 0);
    
    // casteamos el pin para devolverselo a la funcion callback
    if (f_callback_drv) {
        uint8_t id_boton = cast_boton_pin(PIN_EINT_0);
        if (id_boton != 0x4) {
            f_callback_drv(id_boton);
        }
    }

    VICVectAddr = 0;
}

__irq void interrupcion_EINT_1(void) {
        VICIntEnClr = (1 << 15); 
    // limpliamos flag antes del callback
    EXTINT = (1 << 1);
    
    // casteamos el pin para devolverselo a la funcion callback
    if (f_callback_drv) {
        uint8_t id_boton = cast_boton_pin(PIN_EINT_1);
        if (id_boton != 0x4) {
            f_callback_drv(id_boton);
        }
    }
    
    VICVectAddr = 0;
}

__irq void interrupcion_EINT_2(void) {
    VICIntEnClr = (1 << 16); 
    
    // limpliamos flag antes del callback
    EXTINT = (1 << 2);
    
    // casteamos el pin para devolverselo a la funcion callback
    if (f_callback_drv) {
        uint8_t id_boton = cast_boton_pin(PIN_EINT_2);
        if (id_boton != 0x4) {
            f_callback_drv(id_boton);
        }
    }
    
    VICVectAddr = 0;
}


/* Función de Callback */
static void (*f_callback_drv)() = 0;


static uint8_t pin_entero(uint8_t pin)
{
    //devolvemos un entero que representa el pin
    if (pin == PIN_EINT_1) return 1
    if (pin == PIN_EINT_2) return 2;
    if (pin == PIN_EINT_0) return 0;
    return 4;
}


static uint8_t cast_boton_pin(uint8_t pin)
{
    //devolvemos un entero que representa el boton
    for (uint8_t i = 0; i < BUTTONS_NUMBER; i++) {
        if (s_buttons[i] == pin) {
            return i;
        }
    }
    return 0x4;//error
}

//Inicialización 
void hal_ext_int_iniciar(hal_ext_int_callback_t callback)
{
    f_callback_drv = callback;

    for(int i = 0; i < BUTTONS_NUMBER; i++) {
        uint8_t pin = s_buttons[i];
        
        // Verificamos si el pin actual coincide con alguna de las lineas de interrupcion externa disponibles
        if (pin == PIN_EINT_1){
            // Configuramos P0.14 para que funcione como EINT_1 (bits 29:28 a 10)
            PINSEL0 = (PINSEL0 & ~(3UL << 28)) | (2UL << 28);
            
            // Nos aseguramos de borrar cualquier flag pendiente
            EXTINT = (1 << 1);
            
            // Activamos la capacidad de despertar al micro desde Power Down
            EXTWAKE |= (1 << 1);
            
            // Configuramos el controlador de interrupciones (VIC)
            // Asignamos al slot 2, canal 15 (EINT_1) y habilitamos
            VICVectCntl2 = 0x20 | 15;
            VICVectAddr2 = (unsigned long)interrupcion_EINT_1;
            VICIntEnable = (1 << 15);
        
        } else if (pin == PIN_EINT_2){
            // Configuramos P0.15 para que funcione como EINT_2 (bits 31:30 a 10)
            PINSEL0 = (PINSEL0 & ~(3UL << 30)) | (2UL << 30);
            
            // Borrado de flags previos
            EXTINT = (1 << 2);
            
            // Habilitar wakeup
            EXTWAKE |= (1 << 2);
            
            // Configuracion VIC: Slot 3, canal 16 (EINT_2)
            VICVectCntl3 = 0x20 | 16;
            VICVectAddr3 = (unsigned long)interrupcion_EINT_2;
            VICIntEnable = (1 << 16);

        } else if (pin == PIN_EINT_0){
            // Configuramos P0.16 para que funcione como EINT_0 (bits 1:0 de PINSEL1 a 01)
            PINSEL1 = (PINSEL1 & ~(3 << 0)) | (1 << 0);
            
            // Limpieza de interrupciones pendientes
            EXTINT = (1 << 0);
            
            // Habilitar wakeup
            EXTWAKE |= (1 << 0);
            
            // Configuracion VIC: Slot 4, canal 14 (EINT_0)
            VICVectCntl4 = 0x20 | 14;
            VICVectAddr4 = (unsigned long)interrupcion_EINT_0;
            VICIntEnable = (1 << 14);
        }

    }
}

//Habilitamos interrupciones
void hal_ext_int_habilitar(uint32_t id_linea)
{
    if (id_linea >= BUTTONS_NUMBER) return;
    
    uint8_t pin = s_buttons[id_linea];

    if (pin == PIN_EINT_1){
        // Configuracion de P0.14 para EINT_1 (bits 29:28 a 10)
        PINSEL0 = (PINSEL0 & ~(3UL << 28)) | (2UL << 28);
        
        // Limpiamos flags pendientes antes de activar
        EXTINT = (1 << 1);
        
        // Habilitamos EINT_1 en el VIC (bit 15)
        VICIntEnable = (1 << 15);
    
    } else if (pin == PIN_EINT_2){
        // Configuracion de P0.15 para EINT_2 (bits 31:30 a 10)
        PINSEL0 = (PINSEL0 & ~(3UL << 30)) | (2UL << 30);
        
        // Limpiamos flags pendientes antes de activar
        EXTINT = (1 << 2);
        
        // Habilitamos EINT_2 en el VIC (bit 16)
        VICIntEnable = (1 << 16);

    } else if (pin == PIN_EINT_0){
        // Configuracion de P0.16 para EINT_0 (bits 1:0 PINSEL1 a 01)
        PINSEL1 = (PINSEL1 & ~(3 << 0)) | (1 << 0);
        
        // Limpiamos flags pendientes antes de activar
        EXTINT = (1 << 0);
        
        // Habilitamos EINT_0 en el VIC (bit 14)
        VICIntEnable = (1 << 14);
    }
}

//Deshabilitamos las interrupciones
void hal_ext_int_deshabilitar(uint32_t id_linea)
{
    if (id_linea >= BUTTONS_NUMBER) return;
    
    uint8_t pin = s_buttons[id_linea];
    
    if (pin == PIN_EINT_1){
        // Desactivamos EINT_1 en el VIC
        VICIntEnClr = (1 << 15); 
        
        // Limpiamos el flag de interrupcion externa
        EXTINT = (1 << 1);
        
        // Revertimos P0.14 a funcion GPIO (bits 29:28 a 00)
        PINSEL0 &= ~(3UL << 28); 
    
    } else if (pin == PIN_EINT_2){
        // Desactivamos EINT_2 en el VIC
        VICIntEnClr = (1 << 16); 
        
        // Limpiamos el flag de interrupcion externa
        EXTINT = (1 << 2);
        
        // Revertimos P0.15 a funcion GPIO (bits 31:30 a 00)
        PINSEL0 &= ~(3UL << 30); 

    } else if (pin == PIN_EINT_0){
        // Desactivamos EINT_0 en el VIC
        VICIntEnClr = (1 << 14); 
        
        // Limpiamos el flag de interrupcion externa
        EXTINT = (1 << 0);
        
        // Revertimos P0.16 a funcion GPIO (bits 1:0 PINSEL1 a 00)
        PINSEL1 &= ~(3UL << 0);
    }
}

//Limpiamos interrupciones pendientes
void hal_ext_int_limpiar_pendiente(uint32_t id_linea)
{
    if (id_linea >= BUTTONS_NUMBER) return;
    
    uint8_t pin = s_buttons[id_linea];
    uint8_t eint = pin_entero(pin);
    
    if (eint != 0x4) {
        EXTINT = (1 << eint);
    }
}


