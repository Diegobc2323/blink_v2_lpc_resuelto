#include <stdbool.h>
#include <rt_GE.h>
#include <stdint.h>
#include <stddef.h>
#include "rt_evento_t.h"
#include "svc_alarmas.h"
#include "drv_consumo.h"
#include "rt_fifo.h"
#include "drv_botones.h"
#include "drv_SC.h"
#include "drv_WDT.h"
#include "rt_GE.h"


#define sec 1
#define prioridad_alta 0
#define prioridad_baja 1

#define INACTIVITY_TIME_MS 10000 // Tiempo de inactividad antes de entrar en sueño profundo (ms)
static uint32_t g_M_overflow_monitor_id;
#define rt_GE_MAX_SUSCRITOS 4 // Número máximo de tareas que pueden suscribirse a un mismo evento
// Definición del tipo puntero a función para un callback (manejador de evento)
// Estructura que define una tarea suscrita
typedef struct {
    f_callback_GE callback;
    uint8_t prioridad;
} TareaSuscrita_t;

// Estructura global para almacenar todas las suscripciones (extern definida en svc_GE.c)
static TareaSuscrita_t TareasSuscritas[EVENT_TYPES][rt_GE_MAX_SUSCRITOS];

// Contador de cuántas tareas hay suscritas a cada evento (extern definido en svc_GE.c)
static uint8_t numSuscritos[EVENT_TYPES];



/**
 * Inicializa el Gestor de Eventos.
 */
void rt_GE_iniciar(uint32_t monitor_overflow){ 
		drv_WDT_iniciar(sec);	
    // Asignar el ID del monitor de overflow
    g_M_overflow_monitor_id = monitor_overflow;
    
    // Limpiar toda la tabla de suscripciones
    for (int i = 0; i < EVENT_TYPES; i++) {
        numSuscritos[i] = 0;
        for (int j = 0; j < rt_GE_MAX_SUSCRITOS; j++) {
            TareasSuscritas[i][j].callback = NULL;
            // Inicializar prioridad con un valor máximo (255 indica la menor prioridad posible)
            TareasSuscritas[i][j].prioridad = 255; 
        }
    }
	
    // Suscribir tarea a eventos de control:
    // ev_INACTIVIDAD con prioridad alta (0)
    rt_GE_suscribir(ev_INACTIVIDAD,prioridad_alta,rt_GE_actualizar);
    // ev_PULSAR_BOTON (y otros eventos de usuario) con prioridad menor (1)
    rt_GE_suscribir(ev_PULSAR_BOTON,prioridad_baja,rt_GE_actualizar);
		rt_GE_suscribir(ev_JUEGO_NUEVO_LED, prioridad_baja, rt_GE_actualizar);
    rt_GE_suscribir(ev_JUEGO_TIMEOUT, prioridad_baja, rt_GE_actualizar);

}

/**
 * @brief Bucle principal del sistema (dispatcher).
 */
void rt_GE_lanzador(void) {
    EVENTO_T evento;
    uint32_t auxData;
    
    // Programar la alarma de inactividad por primera vez
    uint32_t alarma_inactividad_flags = svc_alarma_codificar(false, INACTIVITY_TIME_MS, 0);
    svc_alarma_activar(alarma_inactividad_flags, ev_INACTIVIDAD, 0);

    // Bucle infinito del planificador
    while(1) {
        
        // Entrar en sección crítica para manipular la FIFO
        uint32_t sc = drv_SC_entrar_disable_irq();
				drv_WDT_alimentar();
			  drv_SC_salir_enable_irq();
        // 1. Intentar extraer un evento de la cola FIFO
			  sc = drv_SC_entrar_disable_irq();
        if (rt_FIFO_extraer(&evento, &auxData, NULL)) {
						
            // Salir de la sección crítica inmediatamente después de extraer
            drv_SC_salir_enable_irq();
            
            // --- INICIO DE LÓGICA DE DESPACHO (Dispatch) ---
            if (evento < EVENT_TYPES) { // Comprobar que es un evento válido
                
                // Recorrer la lista de suscritos para este evento
                for (int i = 0; i < numSuscritos[evento]; i++) {
                
                    if (TareasSuscritas[evento][i].callback != NULL) {
                        // Llamar al callback de la tarea suscrita
                        TareasSuscritas[evento][i].callback(evento, auxData);
                    }
                }
            }
            // --- FIN DE LÓGICA DE DESPACHO ---
            
        } else {
            // Salir de la sección crítica antes de entrar en bajo consumo
            drv_SC_salir_enable_irq();
            
            // 3. Si no hay eventos, entrar en modo IDLE (bajo consumo, esperando interrupción)
            drv_consumo_esperar();
        }
    }
}

/**
 * @brief Función de actualización para eventos de control del RunTime.
 */
void rt_GE_actualizar(EVENTO_T ID_evento, uint32_t auxiliar){
    switch (ID_evento) {
        
        case ev_INACTIVIDAD:
            drv_consumo_dormir();
            break;
        
        // CASO A: Solo reiniciamos si es el USUARIO quien actúa
        case ev_PULSAR_BOTON: 
        {
            uint32_t alarma_flags = svc_alarma_codificar(false, INACTIVITY_TIME_MS, 0);
            svc_alarma_activar(alarma_flags, ev_INACTIVIDAD, 0);
            break;
        }

        // CASO B: Los eventos internos del juego NO deben reiniciar el timer
        //case ev_JUEGO_NUEVO_LED:
        //case ev_JUEGO_TIMEOUT:
            // No hacemos nada aquí para la inactividad.
            // Dejamos que el contador de 20s siga bajando.
            //break;
        
        default:
            break;
    }
}
void rt_GE_suscribir(EVENTO_T ID_evento, uint8_t prioridad, f_callback_GE f_callback){
    
    if (ID_evento >= EVENT_TYPES || f_callback == NULL) return;

    
    if (numSuscritos[ID_evento] >= rt_GE_MAX_SUSCRITOS) {
        // Desbordamiento de la capacidad de suscripción para este evento
        if (g_M_overflow_monitor_id) {
            drv_monitor_marcar(g_M_overflow_monitor_id);
        }
        while(1){
            ;
        }// Bucle infinito de parada por error
    }

    
    int i = numSuscritos[ID_evento];
    
    // Desplazar elementos de menor prioridad para hacer sitio al nuevo suscriptor.
    while (i > 0 && TareasSuscritas[ID_evento][i-1].prioridad > prioridad) {
        TareasSuscritas[ID_evento][i] = TareasSuscritas[ID_evento][i-1];
        i--;
    }
    
    // Insertar el nuevo callback en su posición ordenada por prioridad
    TareasSuscritas[ID_evento][i].callback = f_callback;
    TareasSuscritas[ID_evento][i].prioridad = prioridad;
    numSuscritos[ID_evento]++;
}

/**
 * @brief Cancela la suscripción de una función de callback a un evento.
 *
 * Si se encuentra la tarea, compacta el array de suscripciones.
 */
void rt_GE_cancelar(EVENTO_T ID_evento, f_callback_GE f_callback){
    if (ID_evento >= EVENT_TYPES || f_callback == NULL) return;

    int i = 0;
    // 1. Buscar la tarea (callback) a cancelar
    while (i < numSuscritos[ID_evento] && 
           TareasSuscritas[ID_evento][i].callback != f_callback) {
        i++;
    }
    
    // 2. Si se encontró, compactar el array
    if (i < numSuscritos[ID_evento]) {
        // Mover todos los elementos siguientes una posición hacia arriba
        for (int j = i; j < numSuscritos[ID_evento] - 1; j++) {
            TareasSuscritas[ID_evento][j] = TareasSuscritas[ID_evento][j+1];
        }
        
        // Limpiar el último slot y decrementar contador
        numSuscritos[ID_evento]--;
        // Limpiar el slot liberado (nuevo último elemento)
        TareasSuscritas[ID_evento][numSuscritos[ID_evento]].callback = NULL;
        TareasSuscritas[ID_evento][numSuscritos[ID_evento]].prioridad = 255;
    }
}
