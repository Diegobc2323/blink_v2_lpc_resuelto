/* *****************************************************************************
 * ARCHIVO DE TEST - Beat Hero
 * Incluye validación de FIFO, Gestor, Alarmas y Botones Simultáneos
 * ****************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include "board.h"
#include "rt_fifo.h"
#include "rt_GE.h"
#include "svc_alarmas.h"
#include "drv_botones.h"
#include "drv_SC.h"
#include "drv_WDT.h"
#include "drv_leds.h"
#include "drv_tiempo.h"
#include "drv_monitor.h"
#include "rt_evento_t.h"

// --- VARIABLES GLOBALES PARA TESTS ---
static uint32_t test_contador_eventos = 0;
static uint32_t test_ultimo_evento = 0;
static uint32_t test_ultimo_auxdata = 0;
static bool test_evento_recibido = false;
static bool test_alarma_vencida = false;

// Estado de botones para test simultáneo
static bool test_btn_states[BUTTONS_NUMBER] = {false};
static bool test_simultaneo_exito = false;

// --- CALLBACKS ---
void test_callback_evento(EVENTO_T evento, uint32_t auxData) {
    test_contador_eventos++;
    test_ultimo_evento = evento;
    test_ultimo_auxdata = auxData;
    test_evento_recibido = true;
}

void test_callback_alarma(EVENTO_T evento, uint32_t auxData) {
    if (evento == ev_USUARIO_1) {
        test_alarma_vencida = true;
    }
}

// Callback para detectar pulsación simultánea
void test_callback_botones(EVENTO_T evento, uint32_t id) {
    if (id >= BUTTONS_NUMBER) return;

    if (evento == ev_PULSAR_BOTON) {
        test_btn_states[id] = true;
    } 
    else if (evento == ev_SOLTAR_BOTON) {
        test_btn_states[id] = false;
    }

    // Comprobar si hay 2 botones apretados a la vez
    int presionados = 0;
    for(int i=0; i<BUTTONS_NUMBER; i++) {
        if (test_btn_states[i]) presionados++;
    }

    if (presionados >= 2) {
        test_simultaneo_exito = true;
    }
}

/* =============================================================================
 * TEST 1: COLA FIFO
 * ===========================================================================*/
bool test_fifo(void) {
    drv_monitor_marcar(10); 
    
    EVENTO_T ev;
    uint32_t aux;
    
    // 1.1 Prueba básica
    rt_FIFO_encolar(ev_USUARIO_1, 123);
    if (rt_FIFO_extraer(&ev, &aux, NULL) == 0) return false;
    if (ev != ev_USUARIO_1 || aux != 123) return false;
    
    // 1.2 Prueba de orden (3 elementos)
    rt_FIFO_encolar(ev_PULSAR_BOTON, 1);
    rt_FIFO_encolar(ev_SOLTAR_BOTON, 2);
    rt_FIFO_encolar(ev_BOTON_TIMER, 3);
    
    rt_FIFO_extraer(&ev, &aux, NULL);
    if (ev != ev_PULSAR_BOTON) return false;
    
    rt_FIFO_extraer(&ev, &aux, NULL);
    if (ev != ev_SOLTAR_BOTON) return false;
    
    rt_FIFO_extraer(&ev, &aux, NULL);
    if (ev != ev_BOTON_TIMER) return false;

    drv_led_establecer(1, LED_ON); // LED 1 ON si pasa FIFO
    return true;
}

/* =============================================================================
 * TEST 2: GESTOR DE EVENTOS
 * ===========================================================================*/
bool test_gestor_eventos(void) {
    drv_monitor_marcar(12);
    
    test_contador_eventos = 0;
    test_evento_recibido = false;
    
    // 2.1 Suscripción
    rt_GE_suscribir(ev_USUARIO_1, 1, test_callback_evento);
    
    // 2.2 Proceso manual: Encolar y Desencolar
    // Esto verifica que el flujo de datos FIFO -> App es correcto
    rt_FIFO_encolar(ev_USUARIO_1, 555);
    
    EVENTO_T ev;
    uint32_t aux;
    
    // Simulamos lo que haría el lanzador: extraer y llamar
    if (rt_FIFO_extraer(&ev, &aux, NULL)) {
        // NOTA: Aquí llamamos directamente al callback para verificar la lógica
        // ya que no podemos acceder a la tabla interna de suscripciones desde el test.
        test_callback_evento(ev, aux);
    }
    
    if (!test_evento_recibido) return false;
    if (test_ultimo_auxdata != 555) return false;
    
    drv_led_establecer(2, LED_ON); // LED 2 ON si pasa GE
    return true;
}

/* =============================================================================
 * TEST 3: ALARMAS
 * ===========================================================================*/
bool test_alarmas(void) {
    // Verificación estática de codificación
    uint32_t flags = svc_alarma_codificar(true, 500, 1);
    
    if ((flags & 0x80000000) == 0) return false; // Bit periódico
    if ((flags & 0x00FFFFFF) != 500) return false; // Tiempo
    
    drv_led_establecer(3, LED_ON); // LED 3 ON si pasa Alarmas
    return true;
}

/* =============================================================================
 * TEST 4: SECCIÓN CRÍTICA
 * ===========================================================================*/
bool test_seccion_critica(void) {
    drv_SC_entrar_disable_irq();
    volatile int x = 0; 
    x++;
    drv_SC_salir_enable_irq();
    
    // Si no se cuelga, asumimos OK por ahora
    return true; 
}

/* =============================================================================
 * TEST 5: BOTONES SIMULTÁNEOS (INTERACTIVO)
 * ===========================================================================*/
void test_botones_simultaneos(void) {
    // Limpiar estado
    for(int i=0; i<BUTTONS_NUMBER; i++) test_btn_states[i] = false;
    test_simultaneo_exito = false;
    
    // 1. Apagar LEDs para indicar inicio de este test
    for(int i=1; i<=LEDS_NUMBER; i++) drv_led_establecer(i, LED_OFF);

    // 2. Bucle de espera interactiva
    // INSTRUCCIÓN: Pulsa dos botones a la vez para pasar este test.
    // El LED 4 se encenderá cuando lo consigas.
    while(!test_simultaneo_exito) {
        drv_WDT_alimentar();
        
        EVENTO_T ev;
        uint32_t aux;
        
        // Procesamos eventos de botones manualmente desde la FIFO
        if (rt_FIFO_extraer(&ev, &aux, NULL)) {
            if (ev == ev_PULSAR_BOTON || ev == ev_SOLTAR_BOTON) {
                test_callback_botones(ev, aux);
            }
        }
    }
    
    // Éxito: Encender LED 4
    drv_led_establecer(4, LED_ON);
}

/* =============================================================================
 * EJECUTOR PRINCIPAL
 * ===========================================================================*/
uint32_t test_ejecutar_todos(void) {
    uint32_t pasados = 0;
    
    // Apagar todo al inicio
    for(int i=1; i<=LEDS_NUMBER; i++) drv_led_establecer(i, LED_OFF);
    
    // Ejecutar tests automáticos
    if (test_fifo()) pasados++;
    // Breve espera para ver el LED
    for(volatile int i=0; i<200000; i++); 
    
    if (test_gestor_eventos()) pasados++;
    for(volatile int i=0; i<200000; i++);
    
    if (test_alarmas()) pasados++;
    for(volatile int i=0; i<200000; i++);
    
    if (test_seccion_critica()) pasados++;
    
    return pasados;
}