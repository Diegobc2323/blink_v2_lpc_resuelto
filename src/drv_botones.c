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

// Tiempos para el filtrado de rebotes (Debounce)
#define TRP_MS 80   // Tiempo Rebote Pulsado: Espera inicial tras detectar flanco
#define TEP_MS 50   // Tiempo Entre Pulsaciones: Periodo de muestreo para detectar soltado
#define TRD_MS 50   // Tiempo Rebote Despulsado: Espera final antes de rehabilitar IRQ

#define NUM_BOTONES BUTTONS_NUMBER

// Eventos que usaremos para notificar a la aplicacion
static EVENTO_T m_ev_confirmado; 
static EVENTO_T m_ev_soltado;    
static EVENTO_T m_ev_retardo;    

// Estados de la Maquina de Estados (FSM) de cada boton
typedef enum {
    e_esperando,  // IRQ habilitada, esperando pulsacion
    e_rebotes,    // Esperando a que pasen los rebotes mecanicos iniciales
    e_muestreo,   // Boton validado como pulsado, comprobando periodicamente si se suelta
    e_salida      // Boton soltado, esperando tiempo de seguridad antes de rehabilitar IRQ
} FsmEstado_t;

// Estado individual para cada boton
static volatile FsmEstado_t s_estado_botones[BUTTONS_NUMBER];

// Mapeo de pines hardware
static const HAL_GPIO_PIN_T s_pins_botones[BUTTONS_NUMBER] = {
    BUTTON_1,
    BUTTON_2,
    BUTTON_3,
#if BUTTONS_NUMBER > 3
    BUTTON_4
#endif
};

static f_callback_GE drv_botones_isr_callback;

// Callback que se ejecuta desde la ISR (Interrupcion Hardware)
static void drv_cb(uint8_t id_boton) {
    // deshabilitamos interrupcion para que los rebotes no disparen la ISR constantemente
    hal_ext_int_deshabilitar(id_boton);

    // Notificamos al gestor de eventos que ha ocurrido algo
    // La logica real se procesara en drv_botones_actualizar (nivel usuario)
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
    
    // Inicializar estados
    for (int i = 0; i < NUM_BOTONES; i++) {
        s_estado_botones[i] = e_esperando;
    }
    
    // Suscribir la FSM a los eventos del sistema
    // ev_pulsar: viene de la ISR (inicio de pulsacion)
    // ev_tiempo: viene de las alarmas (timeouts de rebotes)
    rt_GE_suscribir(ev_pulsar, 0, drv_botones_actualizar);
    rt_GE_suscribir(ev_tiempo, 0, drv_botones_actualizar);

    // Configurar hardware
    hal_ext_int_iniciar(drv_cb);
    for (int i = 0; i < NUM_BOTONES; i++) {
        hal_ext_int_habilitar(i);
    }
}

// Maquina de Estados Principal
void drv_botones_actualizar (EVENTO_T evento, uint32_t auxiliar){
    
    uint8_t button_id = (uint8_t)auxiliar; 
    if (button_id >= NUM_BOTONES) return; 

    // Caso 1: Evento de pulsacion inicial (desde ISR)
    if (evento == ev_PULSAR_BOTON) {
        if (s_estado_botones[button_id] == e_esperando) {
            // La IRQ ya se deshabilito en la ISR.
            // Programamos alarma para esperar a que la señal se estabilice (TRP)
            uint32_t m_alarma_flags_trp = svc_alarma_codificar(false, TRP_MS, button_id);
            svc_alarma_activar(m_alarma_flags_trp, m_ev_retardo, button_id);
            
            s_estado_botones[button_id] = e_rebotes;
        }
        return; 
    }
    
    // Caso 2: Eventos de temporizacion (timeouts de alarmas)
    switch (s_estado_botones[button_id]) {
        
        case e_rebotes: 
            // Ha pasado el tiempo de rebote inicial.
            // Leemos el pin para ver si sigue pulsado (Nivel Bajo = Activo)
            if (hal_gpio_leer(s_pins_botones[button_id]) == 0) { 
                
                // Confirmado: Es una pulsacion real y estable
                drv_botones_isr_callback(m_ev_confirmado, button_id); 
                
                // Pasamos a modo muestreo periodico para detectar cuando se suelta
                uint32_t m_alarma_flags_tep = svc_alarma_codificar(true, TEP_MS, button_id);
                svc_alarma_activar(m_alarma_flags_tep, m_ev_retardo, button_id);
                
                s_estado_botones[button_id] = e_muestreo;
                
            } else {
                // Falsa alarma (ruido): el boton ya no esta pulsado.
                // Avisamos que se ha soltado (por si la app esperaba algo)
                drv_botones_isr_callback(m_ev_soltado, button_id); 

                // Restauramos estado inicial
                hal_ext_int_limpiar_pendiente(button_id);
                hal_ext_int_habilitar(button_id);
                s_estado_botones[button_id] = e_esperando;
            }
            break;
            
        case e_muestreo: 
            // Comprobamos periodicamente si se ha soltado
            // Si devuelve != 0 es que esta en ALTO (Soltado)
            if (hal_gpio_leer(s_pins_botones[button_id]) != 0) { 
                
                // 1. Notificar a la app
                drv_botones_isr_callback(m_ev_soltado, button_id);

                // 2. Cancelar el muestreo periodico
                svc_alarma_activar(0, m_ev_retardo, button_id);
                
                // 3. Esperar tiempo de seguridad (TRD) antes de reactivar IRQ
                // Esto evita que los rebotes al soltar disparen una nueva pulsacion
                uint32_t m_alarma_flags_trd = svc_alarma_codificar(false, TRD_MS, button_id);
                svc_alarma_activar(m_alarma_flags_trd, m_ev_retardo, button_id);
                
                s_estado_botones[button_id] = e_salida;
            }
            break;
            
        case e_salida: 
            // Ha pasado el tiempo de seguridad final.
            // Limpiamos flags pendientes y rehabilitamos la interrupcion
            hal_ext_int_limpiar_pendiente(button_id);
            hal_ext_int_habilitar(button_id);
            s_estado_botones[button_id] = e_esperando;
            break;
            
        case e_esperando:
        default:
            // Recuperacion de errores
            hal_ext_int_habilitar(button_id);
            break;
    }
}
