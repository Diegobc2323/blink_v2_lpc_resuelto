#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "board.h"          
#include "drv_botones.h"
#include "drv_monitor.h"
#include "drv_tiempo.h"
#include "svc_alarmas.h"
#include "rt_GE.h"

#include "hal_gpio.h"
#include "hal_ext_int.h"

#define TRP_MS 80  
#define TEP_MS 50  
#define TRD_MS 50  

#define NUM_BOTONES BUTTONS_NUMBER

static EVENTO_T m_ev_confirmado; 
static EVENTO_T m_ev_soltado;    
static EVENTO_T m_ev_retardo;    

typedef enum {
    e_esperando,
    e_rebotes,
    e_muestreo,
    e_salida
} FsmEstado_t;

static volatile FsmEstado_t s_estado_botones[BUTTONS_NUMBER];

static const HAL_GPIO_PIN_T s_pins_botones[BUTTONS_NUMBER] = {
    BUTTON_1,

    BUTTON_2,

    BUTTON_3,

#if BUTTONS_NUMBER > 3
    BUTTON_4
#endif
};

static f_callback_GE drv_botones_isr_callback;

// --- CORRECCIÓN IMPORTANTE AQUÍ ---
static void drv_cb(uint8_t id_boton) {
    // 1. Deshabilitamos la interrupción INMEDIATAMENTE al detectar el flanco.
    // Esto evita que los rebotes físicos sigan disparando esta función y
    // llenen la cola de eventos, lo que causaba el reinicio (Watchdog/Overflow).
    hal_ext_int_deshabilitar(id_boton);

    // 2. Notificamos al gestor de eventos
    drv_botones_isr_callback(ev_PULSAR_BOTON, id_boton);
}

void drv_botones_iniciar (void(*funcion_callback_app)(uint32_t, uint32_t), 
                          EVENTO_T ev_pulsar, 
                          EVENTO_T ev_soltar, 
                          EVENTO_T ev_tiempo) 
{
    drv_botones_isr_callback = funcion_callback_app; 
    m_ev_confirmado = ev_pulsar; 
    m_ev_soltado = ev_soltar;     
    m_ev_retardo = ev_tiempo;    
    
    for (int i = 0; i < NUM_BOTONES; i++) {
        s_estado_botones[i] = e_esperando;
    }
    
    // Suscribimos la FSM a los eventos necesarios
    rt_GE_suscribir(ev_pulsar, 0, drv_botones_actualizar);
    rt_GE_suscribir(ev_tiempo, 0, drv_botones_actualizar);

    hal_ext_int_iniciar(drv_cb);
    for (int i = 0; i < NUM_BOTONES; i++) {
        hal_ext_int_habilitar(i);
    }
}

void drv_botones_actualizar (EVENTO_T evento, uint32_t auxiliar){
    
    uint8_t button_id = (uint8_t)auxiliar; 
    if (button_id >= NUM_BOTONES) return; 

    if (evento == ev_PULSAR_BOTON) {
        if (s_estado_botones[button_id] == e_esperando) {
            // NOTA: Ya no llamamos a hal_ext_int_deshabilitar aquí
            // porque ya lo hicimos en la ISR (drv_cb) para protegernos.
            
            // Alarma rebotes (TRP)
            uint32_t m_alarma_flags_trp = svc_alarma_codificar(false, TRP_MS, button_id);
            svc_alarma_activar(m_alarma_flags_trp, m_ev_retardo, button_id);
            
            s_estado_botones[button_id] = e_rebotes;
        }
        return; 
    }
    
    switch (s_estado_botones[button_id]) {
        
        case e_rebotes: 
            // Comprobamos si sigue pulsado tras el tiempo de rebote
            // (Asumiendo activo a nivel bajo 0)
            if (hal_gpio_leer(s_pins_botones[button_id]) == 0) { 
                
                // Confirmamos pulsación real
                drv_botones_isr_callback(m_ev_confirmado, button_id); 
                
                // Pasamos a muestreo periódico para detectar cuándo se suelta
                uint32_t m_alarma_flags_tep = svc_alarma_codificar(true, TEP_MS, button_id);
                svc_alarma_activar(m_alarma_flags_tep, m_ev_retardo, button_id);
                
                s_estado_botones[button_id] = e_muestreo;
                
            } else {
                // Falsa alarma (ruido): el botón ya no está pulsado.
                // IMPORTANTE: Avisar a la APP de que "soltamos" por si acaso
                drv_botones_isr_callback(m_ev_soltado, button_id); 

                hal_ext_int_limpiar_pendiente(button_id);
                hal_ext_int_habilitar(button_id);
                s_estado_botones[button_id] = e_esperando;
            }
            break;
            
        case e_muestreo: 
            // Si devuelve != 0 es que está en ALTO (Soltado)
            if (hal_gpio_leer(s_pins_botones[button_id]) != 0) { 
                
                // 1. Avisar soltado
                drv_botones_isr_callback(m_ev_soltado, button_id);

                // 2. Cancelar la alarma periódica
                svc_alarma_activar(0, m_ev_retardo, button_id);
                
                // 3. Esperar un poco antes de rehabilitar IRQ (TRD)
                uint32_t m_alarma_flags_trd = svc_alarma_codificar(false, TRD_MS, button_id);
                svc_alarma_activar(m_alarma_flags_trd, m_ev_retardo, button_id);
                
                s_estado_botones[button_id] = e_salida;
            }
            break;
            
        case e_salida: 
            hal_ext_int_limpiar_pendiente(button_id);
            hal_ext_int_habilitar(button_id);
            s_estado_botones[button_id] = e_esperando;
            break;
            
        case e_esperando:
        default:
            hal_ext_int_habilitar(button_id);
            break;
    }
}
