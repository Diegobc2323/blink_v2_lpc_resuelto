#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "rt_GE.h"
#include "rt_evento_t.h"
#include "rt_fifo.h"
#include "svc_alarmas.h"
#include "drv_consumo.h"
#include "drv_SC.h"
#include "drv_WDT.h"
#include "drv_tiempo.h"
#include "drv_monitor.h" 

#define DEBUG

#define sec 1
#define prioridad_alta 0      // Menor numero = mayor prioridad
#define prioridad_baja 1

#define INACTIVITY_TIME_MS 10000  // Timeout para entrar en modo sleep (10s)
static uint32_t g_M_overflow_monitor_id;
#define rt_GE_MAX_SUSCRITOS 4     // Maximo de callbacks por tipo de evento

// Estructura que asocia un callback con su prioridad
typedef struct {
    f_callback_GE callback;   // Funcion a llamar cuando ocurra el evento
    uint8_t prioridad;        // Prioridad de ejecucion (0=alta, 255=baja)
} TareaSuscrita_t;

// Tabla de suscripciones: [tipo_evento][indice_suscriptor]
// Permite hasta rt_GE_MAX_SUSCRITOS callbacks por cada tipo de evento
static TareaSuscrita_t TareasSuscritas[EVENT_TYPES][rt_GE_MAX_SUSCRITOS];
static uint8_t numSuscritos[EVENT_TYPES];  // Contador de suscritos por evento


#ifdef DEBUG
// Variables globales de depuracion (sin static para inspeccion externa)
volatile uint32_t dbg_ge_latencia_min = 0xFFFFFFFF;
volatile uint32_t dbg_ge_latencia_max = 0;
// Histograma de latencias: [0]:<1ms, [1]:<10ms, [2]:<50ms, [3]:<100ms, [4]:>100ms
volatile uint32_t dbg_ge_hist[5] = {0,0,0,0,0};

// Registra la latencia entre timestamp del evento y su procesamiento
static void registrar_latencia(Tiempo_us_t ts_evento) {
    Tiempo_us_t ahora = drv_tiempo_actual_us();
    uint32_t latencia = (uint32_t)(ahora - ts_evento); 

    if (latencia < dbg_ge_latencia_min) dbg_ge_latencia_min = latencia;
    if (latencia > dbg_ge_latencia_max) dbg_ge_latencia_max = latencia;

    uint32_t latencia_ms = latencia / 1000;
    
    // Clasificar en bucket segun magnitud
    if (latencia_ms < 1)       dbg_ge_hist[0]++;
    else if (latencia_ms < 10) dbg_ge_hist[1]++;
    else if (latencia_ms < 50) dbg_ge_hist[2]++;
    else if (latencia_ms < 100)dbg_ge_hist[3]++;
    else                       dbg_ge_hist[4]++;
}
#endif

void rt_GE_iniciar(uint32_t monitor_overflow){ 
		drv_WDT_iniciar(sec);	
    g_M_overflow_monitor_id = monitor_overflow;
    
    // Limpiar todas las suscripciones
    for (int i = 0; i < EVENT_TYPES; i++) {
        numSuscritos[i] = 0;
        for (int j = 0; j < rt_GE_MAX_SUSCRITOS; j++) {
            TareasSuscritas[i][j].callback = NULL;
            TareasSuscritas[i][j].prioridad = 255; 
        }
    }
	
    // Suscribir callbacks internos para gestion de energia
    rt_GE_suscribir(ev_INACTIVIDAD,prioridad_alta,rt_GE_actualizar);
    rt_GE_suscribir(ev_PULSAR_BOTON,prioridad_baja,rt_GE_actualizar);
		rt_GE_suscribir(ev_JUEGO_NUEVO_LED, prioridad_baja, rt_GE_actualizar);
    rt_GE_suscribir(ev_JUEGO_TIMEOUT, prioridad_baja, rt_GE_actualizar);
}

void rt_GE_lanzador(void) {
    EVENTO_T evento;
    uint32_t auxData;
    Tiempo_us_t timestamp; 
    
    // Programar alarma de inactividad inicial
    uint32_t alarma_inactividad_flags = svc_alarma_codificar(false, INACTIVITY_TIME_MS, 0);
    svc_alarma_activar(alarma_inactividad_flags, ev_INACTIVIDAD, 0);

    while(1) {
        
        // Alimentar watchdog en seccion critica
        uint32_t sc = drv_SC_entrar_disable_irq();
				drv_WDT_alimentar();
			  drv_SC_salir_enable_irq();
        
        // Intentar extraer evento de la FIFO (en seccion critica)
			  sc = drv_SC_entrar_disable_irq();
        if (rt_FIFO_extraer(&evento, &auxData, &timestamp)) {
						
            drv_SC_salir_enable_irq();
            
            #ifdef DEBUG
            registrar_latencia(timestamp);
            #endif

            // Despachar evento: llamar todos los callbacks suscritos
            if (evento < EVENT_TYPES) { 
                for (int i = 0; i < numSuscritos[evento]; i++) {
                    if (TareasSuscritas[evento][i].callback != NULL) {
                        TareasSuscritas[evento][i].callback(evento, auxData);
                    }
                }
            }
            
        } else {
            // FIFO vacia: entrar en modo sleep hasta proxima interrupcion
            drv_SC_salir_enable_irq();
            drv_consumo_esperar();
        }
    }
}

void rt_GE_actualizar(EVENTO_T ID_evento, uint32_t auxiliar){
    switch (ID_evento) {
        case ev_INACTIVIDAD:
            // Timeout de inactividad: entrar en modo bajo consumo
            drv_consumo_dormir();
            break;
        case ev_PULSAR_BOTON: 
        {
            // Actividad detectada: resetear timeout de inactividad
            uint32_t alarma_flags = svc_alarma_codificar(false, INACTIVITY_TIME_MS, 0);
            svc_alarma_activar(alarma_flags, ev_INACTIVIDAD, 0);
            break;
        }
        default:
            break;
    }
}

void rt_GE_suscribir(EVENTO_T ID_evento, uint8_t prioridad, f_callback_GE f_callback){
    
    // Validacion de parametros
    if (ID_evento >= EVENT_TYPES || f_callback == NULL) return;

    // Overflow protection: demasiados suscritos para este evento
    if (numSuscritos[ID_evento] >= rt_GE_MAX_SUSCRITOS) {
        if (g_M_overflow_monitor_id) {
            drv_monitor_marcar(g_M_overflow_monitor_id);
        }
        while(1);  // Sistema detenido por error fatal
    }

    // Insercion ordenada por prioridad (insertion sort)
    // Desplazar callbacks de menor prioridad hacia la derecha
    int i = numSuscritos[ID_evento];
    
    while (i > 0 && TareasSuscritas[ID_evento][i-1].prioridad > prioridad) {
        TareasSuscritas[ID_evento][i] = TareasSuscritas[ID_evento][i-1];
        i--;
    }
    
    // Insertar nuevo callback en la posicion correcta
    TareasSuscritas[ID_evento][i].callback = f_callback;
    TareasSuscritas[ID_evento][i].prioridad = prioridad;
    numSuscritos[ID_evento]++;
}

void rt_GE_cancelar(EVENTO_T ID_evento, f_callback_GE f_callback){
    if (ID_evento >= EVENT_TYPES || f_callback == NULL) return;

    // Buscar el callback en la lista de suscritos
    int i = 0;
    while (i < numSuscritos[ID_evento] && 
           TareasSuscritas[ID_evento][i].callback != f_callback) {
        i++;
    }
    
    // Si se encontro, eliminarlo desplazando los siguientes hacia la izquierda
    if (i < numSuscritos[ID_evento]) {
        for (int j = i; j < numSuscritos[ID_evento] - 1; j++) {
            TareasSuscritas[ID_evento][j] = TareasSuscritas[ID_evento][j+1];
        }
        numSuscritos[ID_evento]--;
        
        // Limpiar el ultimo slot (ahora sin usar)
        TareasSuscritas[ID_evento][numSuscritos[ID_evento]].callback = NULL;
        TareasSuscritas[ID_evento][numSuscritos[ID_evento]].prioridad = 255;
    }
}