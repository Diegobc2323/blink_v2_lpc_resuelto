#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "rt_evento_t.h"
#include "drv_monitor.h"
#include "drv_tiempo.h"
#include "svc_alarmas.h"
#include "rt_fifo.h"
#include "rt_GE.h" 


#define svc_ALARMAS_MAX 8 
#define tiempo_periodico 1

#define MASK_RETARDO    0x00FFFFFF
#define MASK_FLAGS      0x7F000000
#define MASK_PERIODICA  0x80000000

typedef struct {
    bool activa;
    bool periodica;
    uint32_t retardo_ms;
    uint32_t contador;
    EVENTO_T ID_evento;
    uint32_t auxData;
} Alarma_t;

static Alarma_t m_alarmas[svc_ALARMAS_MAX];
static void (*m_cb_a_llamar)(uint32_t, uint32_t); 
static EVENTO_T m_ev_a_notificar;
static uint32_t g_M_overflow_monitor_id;

#ifdef DEBUG
// --- VARIABLES GLOBALES DE DEPURACIÓN (Sin static, con volatile) ---
volatile uint32_t dbg_alarmas_activas = 0;
volatile uint32_t dbg_alarmas_max_uso = 0;
#endif

static inline uint32_t decodificar_retardo(uint32_t flags) {
    return flags & MASK_RETARDO;
}

static inline bool decodificar_periodica(uint32_t flags) {
    return (flags & MASK_PERIODICA) != 0;
}

static Alarma_t* buscar_alarma(EVENTO_T ID_evento, uint32_t auxData) {
    for (int i = 0; i < svc_ALARMAS_MAX; i++) {
        if (m_alarmas[i].activa && 
            m_alarmas[i].ID_evento == ID_evento && 
            m_alarmas[i].auxData == auxData) { 
            return &m_alarmas[i];
        }
    }
    return NULL;
}

static Alarma_t* buscar_slot_libre(void) {
    for (int i = 0; i < svc_ALARMAS_MAX; i++) {
        if (!m_alarmas[i].activa) {
            return &m_alarmas[i];
        }
    }
    return NULL;
}

void svc_alarma_iniciar(uint32_t monitor_overflow, void(*funcion_callback_app)(uint32_t, uint32_t), EVENTO_T ev_a_notificar) {
    g_M_overflow_monitor_id = monitor_overflow;
    m_cb_a_llamar = funcion_callback_app; 
    m_ev_a_notificar = ev_a_notificar;      
    
    for (int i = 0; i < svc_ALARMAS_MAX; i++) {
        m_alarmas[i].activa = false;
    }
    
    #ifdef DEBUG
    dbg_alarmas_activas = 0;
    dbg_alarmas_max_uso = 0;
    #endif

    rt_GE_suscribir(m_ev_a_notificar, 0, svc_alarma_actualizar);
    drv_tiempo_periodico_ms(tiempo_periodico, m_cb_a_llamar, m_ev_a_notificar);
}

uint32_t svc_alarma_codificar(bool periodico, uint32_t retardo_ms, uint8_t flags) {
    uint32_t alarma_flags = 0;
    alarma_flags |= (retardo_ms & MASK_RETARDO);
    alarma_flags |= ((uint32_t)flags << 24) & MASK_FLAGS;
    if (periodico) {
        alarma_flags |= MASK_PERIODICA;
    }
    return alarma_flags;
}

void svc_alarma_activar(uint32_t alarma_flags, EVENTO_T ID_evento, uint32_t auxData) {
    
    Alarma_t* alarma = buscar_alarma(ID_evento, auxData);
    
    // Caso 1: Desprogramar
    if (alarma_flags == 0) {
        if (alarma != NULL) {
            alarma->activa = false;
            #ifdef DEBUG
            if (dbg_alarmas_activas > 0) dbg_alarmas_activas--;
            #endif
        }
        return;
    }
    
    // Caso 2: Programar o Reprogramar
    if (alarma == NULL) {
        alarma = buscar_slot_libre();
        if (alarma == NULL) {
            if (g_M_overflow_monitor_id) { 
                drv_monitor_marcar(g_M_overflow_monitor_id); 
            }
            while(1);
        }
        // Nueva alarma ocupada
        #ifdef DEBUG
        dbg_alarmas_activas++;
        if (dbg_alarmas_activas > dbg_alarmas_max_uso) {
            dbg_alarmas_max_uso = dbg_alarmas_activas;
        }
        #endif
    }
    
    alarma->activa = true;
    alarma->periodica = decodificar_periodica(alarma_flags);
    alarma->retardo_ms = decodificar_retardo(alarma_flags);
    alarma->contador = alarma->retardo_ms; 
    alarma->ID_evento = ID_evento;
    alarma->auxData = auxData; 
}

void svc_alarma_actualizar(EVENTO_T evento, uint32_t aux) { 
    if (evento != m_ev_a_notificar) { 
        return;
    }
    
    for (int i = 0; i < svc_ALARMAS_MAX; i++) {
        if (m_alarmas[i].activa) {
            if (m_alarmas[i].contador > 0) {
                m_alarmas[i].contador--;
            }
            
            if (m_alarmas[i].contador == 0) {
                if (m_cb_a_llamar) { 
                    m_cb_a_llamar(m_alarmas[i].ID_evento, m_alarmas[i].auxData);
                }
                
                if (m_alarmas[i].periodica) {
                    m_alarmas[i].contador = m_alarmas[i].retardo_ms;
                } else {
                    m_alarmas[i].activa = false;
                    #ifdef DEBUG
                    if (dbg_alarmas_activas > 0) dbg_alarmas_activas--;
                    #endif
                }
            }
        }
    }
}
