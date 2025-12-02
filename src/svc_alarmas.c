#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "rt_evento_t.h"
#include "drv_monitor.h"
#include "drv_tiempo.h"
#include "svc_alarmas.h"
#include "rt_fifo.h"
#include "rt_GE.h" 
#define DEBUG

// Numero maximo de alarmas que pueden estar activas simultaneamente
#define svc_ALARMAS_MAX 8 

// Periodo de actualizacion de las alarmas en milisegundos
#define tiempo_periodico 1

// Mascaras de bits para codificar/decodificar los flags de alarma:
#define MASK_RETARDO    0x00FFFFFF	// - Bits 0-23:  Retardo en milisegundos (16.7 millones ms max ~= 4.6 horas)
#define MASK_FLAGS      0x7F000000	// - Bits 24-30: Flags adicionales disponibles para uso futuro
#define MASK_PERIODICA  0x80000000	// - Bit 31:     Flag de alarma periodica (1=periodica, 0=un solo disparo)

/**
 * Estructura que representa una alarma individual
 */
typedef struct {
    bool activa;          // Indica si esta alarma esta en uso
    bool periodica;       // true = se repite automaticamente, false = un solo disparo
    uint32_t retardo_ms;  // Valor inicial del retardo en milisegundos
    uint32_t contador;    // Contador regresivo actual en milisegundos
    EVENTO_T ID_evento;   // Identificador del evento asociado a esta alarma
    uint32_t auxData;     // Datos auxiliares para identificar alarmas con el mismo evento
} Alarma_t;

// Pool de alarmas disponibles
static Alarma_t m_alarmas[svc_ALARMAS_MAX];

// Callback que se invoca cuando una alarma expira
static void (*m_cb_a_llamar)(uint32_t, uint32_t); 

// Evento que se usa para notificar actualizaciones periodicas
static EVENTO_T m_ev_a_notificar;

// ID del monitor para marcar overflow de alarmas
static uint32_t g_M_overflow_monitor_id;

#ifdef DEBUG
// Variables de depuracion para monitorear el uso de alarmas
// (volatile para que sean visibles desde el debugger)
volatile uint32_t dbg_alarmas_activas = 0;  // Numero actual de alarmas activas
volatile uint32_t dbg_alarmas_max_uso = 0;  // Maximo numero de alarmas simultaneas alcanzado
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
    // Guardar parametros de configuracion
    g_M_overflow_monitor_id = monitor_overflow;
    m_cb_a_llamar = funcion_callback_app; 
    m_ev_a_notificar = ev_a_notificar;      
    
    // Inicializar todas las alarmas como inactivas
    for (int i = 0; i < svc_ALARMAS_MAX; i++) {
        m_alarmas[i].activa = false;
    }
    
    #ifdef DEBUG
    // Inicializar variables de depuracion
    dbg_alarmas_activas = 0;
    dbg_alarmas_max_uso = 0;
    #endif

    // Suscribirse al evento de actualizacion periodica
    rt_GE_suscribir(m_ev_a_notificar, 0, svc_alarma_actualizar);
    
    // Configurar timer periodico de 1ms que generara el evento de actualizacion
    drv_tiempo_periodico_ms(tiempo_periodico, m_cb_a_llamar, m_ev_a_notificar);
}


uint32_t svc_alarma_codificar(bool periodico, uint32_t retardo_ms, uint8_t flags) {
    uint32_t alarma_flags = 0;
    
    // Codificar retardo (24 bits inferiores)
    alarma_flags |= (retardo_ms & MASK_RETARDO);
    
    // Codificar flags adicionales (bits 24-30)
    alarma_flags |= ((uint32_t)flags << 24) & MASK_FLAGS;
    
    // Codificar flag de alarma periodica (bit 31)
    if (periodico) {
        alarma_flags |= MASK_PERIODICA;
    }
    
    return alarma_flags;
}


void svc_alarma_activar(uint32_t alarma_flags, EVENTO_T ID_evento, uint32_t auxData) {
    
    // Buscar si ya existe una alarma con este evento y auxData
    Alarma_t* alarma = buscar_alarma(ID_evento, auxData);
    
    // Si flags = 0, desprogramamos la alarma que hubiera
    if (alarma_flags == 0) {
        if (alarma != NULL) {
            alarma->activa = false;
            #ifdef DEBUG
            if (dbg_alarmas_activas > 0) dbg_alarmas_activas--;
            #endif
        }
        return;
    }
    
    // Si la alarma no existe, buscar un slot libre
    if (alarma == NULL) {
        alarma = buscar_slot_libre();
        
        // Si no hay slots libres, se ha producido overflow
        if (alarma == NULL) {
            // Marcar el error en el monitor (si esta configurado)
            if (g_M_overflow_monitor_id) { 
                drv_monitor_marcar(g_M_overflow_monitor_id); 
            }
            // Entrar en bucle infinito (comportamiento critico de fallo)
            while(1);
        }
        
        // Actualizar contador de alarmas activas (solo para nuevas alarmas)
        #ifdef DEBUG
        dbg_alarmas_activas++;
        if (dbg_alarmas_activas > dbg_alarmas_max_uso) {
            dbg_alarmas_max_uso = dbg_alarmas_activas;
        }
        #endif
    }
    
    // Si llegamos aqui es que vamos a programar o reconfigurar la alarma que nos han pasado
    alarma->activa = true;
    alarma->periodica = decodificar_periodica(alarma_flags);
    alarma->retardo_ms = decodificar_retardo(alarma_flags);
    alarma->contador = alarma->retardo_ms; // Iniciar contador regresivo
    alarma->ID_evento = ID_evento;
    alarma->auxData = auxData; 
}


void svc_alarma_actualizar(EVENTO_T evento, uint32_t aux) { 
    // Verificar que el evento recibido es el de actualizacion periodica
    if (evento != m_ev_a_notificar) { 
        return;
    }
    
    // Iterar sobre todas las alarmas
    for (int i = 0; i < svc_ALARMAS_MAX; i++) {
        if (m_alarmas[i].activa) {
            // Decrementar contador si no es cero
            if (m_alarmas[i].contador > 0) {
                m_alarmas[i].contador--;
            }
            
            // Si el contador llego a cero, la alarma ha expirado
            if (m_alarmas[i].contador == 0) {
                // Invocar callback con el evento y datos de la alarma
                if (m_cb_a_llamar) { 
                    m_cb_a_llamar(m_alarmas[i].ID_evento, m_alarmas[i].auxData);
                }
                
                // Determinar si la alarma debe repetirse o desactivarse
                if (m_alarmas[i].periodica) {
                    // Alarma periodica: reiniciar contador para el proximo ciclo
                    m_alarmas[i].contador = m_alarmas[i].retardo_ms;
                } else {
                    // Alarma de un solo disparo: desactivar
                    m_alarmas[i].activa = false;
                    #ifdef DEBUG
                    if (dbg_alarmas_activas > 0) dbg_alarmas_activas--;
                    #endif
                }
            }
        }
    }
}