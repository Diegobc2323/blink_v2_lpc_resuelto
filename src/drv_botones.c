/* *****************************************************************************
 * P.H.2025: Driver de Botones con anti-rebote por FSM
 * Implementa una máquina de estados para filtrar rebotes mecánicos
 * y detectar pulsaciones/liberaciones de forma robusta.
 */
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

// Tiempos de la máquina de estados (en milisegundos)
#define TRP_MS 80  // Tiempo de Rebote tras Pulsación: espera antes de confirmar
#define TEP_MS 50  // Tiempo de Espera Periódica: polling mientras está pulsado
#define TRD_MS 50  // Tiempo de Rebote tras Detectar liberación: espera antes de reactivar IRQ  

#define NUM_BOTONES BUTTONS_NUMBER

// Eventos utilizados por la FSM
static EVENTO_T m_ev_confirmado;  // Evento a generar cuando se confirma pulsación
static EVENTO_T m_ev_soltado;     // Evento a generar cuando se suelta el botón
static EVENTO_T m_ev_retardo;     // Evento interno para timeouts de la FSM

/**
 * Máquina de estados para anti-rebote:
 * - e_esperando: Estado inicial, esperando interrupción de flanco
 * - e_rebotes:   Esperando TRP_MS para confirmar que no es rebote
 * - e_muestreo:  Polleando cada TEP_MS para detectar cuándo se suelta
 * - e_salida:    Esperando TRD_MS antes de rehabilitar interrupciones
 */
typedef enum {
    e_esperando,   // IRQ habilitada, esperando flanco de pulsación
    e_rebotes,     // Esperando confirmación (filtro anti-rebote)
    e_muestreo,    // Botón confirmado pulsado, esperando liberación
    e_salida       // Transitorio antes de volver a e_esperando
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


static void drv_cb(uint8_t id_boton) {
    // Deshabilitamos la interrupción para evitar que rebotes mecánicos
    // generen múltiples eventos mientras procesamos este botón
    hal_ext_int_deshabilitar(id_boton);

    // Notificamos al gestor de eventos para iniciar la FSM
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

    // Manejo del evento inicial de pulsación (desde ISR)
    if (evento == ev_PULSAR_BOTON) {
        if (s_estado_botones[button_id] == e_esperando) {
            // Programar alarma TRP para confirmar pulsación tras rebotes
            uint32_t m_alarma_flags_trp = svc_alarma_codificar(false, TRP_MS, button_id);
            svc_alarma_activar(m_alarma_flags_trp, m_ev_retardo, button_id);
            
            s_estado_botones[button_id] = e_rebotes;
        }
        return; 
    }
    
    // Procesamiento de los estados de la FSM según timeouts
    switch (s_estado_botones[button_id]) {
        
        case e_rebotes: 
            // Vence TRP: comprobamos si el botón sigue pulsado
            // (Botones activos a nivel bajo: 0 = pulsado)
            if (hal_gpio_leer(s_pins_botones[button_id]) == 0) { 
                
                // Pulsación confirmada: no era rebote
                drv_botones_isr_callback(m_ev_confirmado, button_id); 
                
                // Iniciar polling periódico (TEP) para detectar liberación
                uint32_t m_alarma_flags_tep = svc_alarma_codificar(true, TEP_MS, button_id);
                svc_alarma_activar(m_alarma_flags_tep, m_ev_retardo, button_id);
                
                s_estado_botones[button_id] = e_muestreo;
                
            } else {
                //Falsa alarma: el botón ya no está pulsado (fue ruido/rebote)
                // Notificamos "soltar" para cancelar cualquier acción iniciada
                drv_botones_isr_callback(m_ev_soltado, button_id); 

                // Volver a estado inicial y rehabilitar IRQ
                hal_ext_int_limpiar_pendiente(button_id);
                hal_ext_int_habilitar(button_id);
                s_estado_botones[button_id] = e_esperando;
            }
            break;
            
        case e_muestreo: 
            // Vence TEP periódico: comprobar si el botón fue liberado
            // (Nivel alto = liberado en lógica activa-baja)
            if (hal_gpio_leer(s_pins_botones[button_id]) != 0) { 
                
                // Botón liberado: notificar evento de soltar
                drv_botones_isr_callback(m_ev_soltado, button_id);

                // Cancelar la alarma periódica (ya no es necesaria)
                svc_alarma_activar(0, m_ev_retardo, button_id);
                
                // Esperar TRD antes de rehabilitar IRQ (evitar rebotes de liberación)
                uint32_t m_alarma_flags_trd = svc_alarma_codificar(false, TRD_MS, button_id);
                svc_alarma_activar(m_alarma_flags_trd, m_ev_retardo, button_id);
                
                s_estado_botones[button_id] = e_salida;
            }
            // Si sigue pulsado, no hacemos nada (esperamos al próximo TEP)
            break;
            
        case e_salida: 
            // Vence TRD: fin del ciclo, volver a estado inicial
            // Limpiar flag de interrupción pendiente y rehabilitar IRQ
            hal_ext_int_limpiar_pendiente(button_id);
            hal_ext_int_habilitar(button_id);
            s_estado_botones[button_id] = e_esperando;
            break;
            
        case e_esperando:
        default:
            // Estado esperando o desconocido: asegurar que IRQ esté habilitada
            hal_ext_int_habilitar(button_id);
            break;
    }
}