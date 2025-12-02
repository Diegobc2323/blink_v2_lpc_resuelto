/* *****************************************************************************
 * P.H.2025: Implementación del tipo de dato rt_fifo
 * Con protecciones de Sección Crítica habilitadas
 */
#include "drv_consumo.h"
#include "drv_monitor.h"
#include "rt_fifo.h"
#include "drv_SC.h" 
#include <stdbool.h>
#include <stddef.h>

// Activa el modo debug (asegúrate de que esto esté descomentado)
#define DEBUG 

#define TAMCOLA 32
typedef uint32_t indice_cola_t;

typedef struct {
    EVENTO cola[TAMCOLA];
    MONITOR_id_t monitor;
    indice_cola_t ultimo_tratado;
    indice_cola_t siguiente_a_tratar;
    volatile uint32_t eventos_a_tratar; 
} RT_FIFO;

static bool s_iniciado = false;
static RT_FIFO s_rt_fifo;

#ifdef DEBUG
// --- VARIABLES GLOBALES DE DEPURACIÓN (Sin static, con volatile) ---
volatile uint32_t dbg_fifo_stats[EVENT_TYPES + 1]; 
volatile uint32_t dbg_fifo_uso_actual = 0;
volatile uint32_t dbg_fifo_uso_max = 0; 
volatile uint32_t dbg_fifo_total_encolados = 0;
#endif

void rt_FIFO_inicializar(uint32_t monitor_overflow){
  s_rt_fifo.ultimo_tratado = 0;
  s_rt_fifo.siguiente_a_tratar = 0; 
  s_rt_fifo.monitor=monitor_overflow;
  s_rt_fifo.eventos_a_tratar=0;
  s_iniciado = true;
  
  #ifdef DEBUG
  dbg_fifo_uso_actual = 0;
  dbg_fifo_uso_max = 0;
  #endif
}

void rt_FIFO_encolar(uint32_t ID_evento, uint32_t auxData){
  if (!s_iniciado) return;
  
  EVENTO ev;
  ev.ID_EVENTO = (EVENTO_T)ID_evento;
  ev.auxData = auxData;
  ev.TS = drv_tiempo_actual_us();

  // --- SECCIÓN CRÍTICA: INICIO ---
  uint32_t estado_anterior = drv_SC_entrar_disable_irq();

  s_rt_fifo.eventos_a_tratar++;
  
  #ifdef DEBUG
  // Actualizar estadísticas 
  dbg_fifo_uso_actual = s_rt_fifo.eventos_a_tratar;
  if (dbg_fifo_uso_actual > dbg_fifo_uso_max) {
      dbg_fifo_uso_max = dbg_fifo_uso_actual;
  }
  dbg_fifo_total_encolados++;
  
  if (ID_evento < EVENT_TYPES) {
      dbg_fifo_stats[ID_evento]++;
  } else {
      dbg_fifo_stats[EVENT_TYPES]++; 
  }
  #endif
  
  if (s_rt_fifo.eventos_a_tratar > TAMCOLA) 
  {
    drv_monitor_marcar(s_rt_fifo.monitor);
    drv_SC_salir_enable_irq();
    while (1);
  }
  
  uint32_t indice = (s_rt_fifo.siguiente_a_tratar + s_rt_fifo.eventos_a_tratar - 1) % TAMCOLA;
  s_rt_fifo.cola[indice] = ev;

  // --- SECCIÓN CRÍTICA: FIN ---
  drv_SC_salir_enable_irq(); 
}

uint8_t rt_FIFO_extraer(EVENTO_T *ID_evento, uint32_t *auxData, Tiempo_us_t *TS){
  // --- SECCIÓN CRÍTICA: INICIO ---
  uint32_t estado_anterior = drv_SC_entrar_disable_irq();

  if(s_rt_fifo.eventos_a_tratar==0) {
      drv_SC_salir_enable_irq();
      return 0;
  }
  
  EVENTO ev = s_rt_fifo.cola[s_rt_fifo.siguiente_a_tratar];
  
  s_rt_fifo.ultimo_tratado = s_rt_fifo.siguiente_a_tratar;
  s_rt_fifo.siguiente_a_tratar = (s_rt_fifo.siguiente_a_tratar + 1) % TAMCOLA;

  s_rt_fifo.eventos_a_tratar--;
  
  #ifdef DEBUG
  dbg_fifo_uso_actual = s_rt_fifo.eventos_a_tratar;
  #endif
  
  uint8_t ret = s_rt_fifo.eventos_a_tratar == 0 ? 1 : s_rt_fifo.eventos_a_tratar;

  // --- SECCIÓN CRÍTICA: FIN ---
  drv_SC_salir_enable_irq();

  if (ID_evento) *ID_evento = ev.ID_EVENTO;
  if (auxData)   *auxData = ev.auxData;
  if (TS)        *TS = ev.TS;

  return ret; 
}

uint32_t rt_FIFO_estadisticas(EVENTO_T ID_evento){
  #ifdef DEBUG
  if (ID_evento < EVENT_TYPES) return dbg_fifo_stats[ID_evento];
  #endif
  return 0;
}