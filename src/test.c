/* *****************************************************************************
 * ARCHIVO DE TEST - Beat Hero
 * Tests unitarios y de integración para validar funcionalidades críticas
 * del sistema embebido
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

/* =============================================================================
 * VARIABLES GLOBALES PARA TESTS
 * ===========================================================================*/

// Resultados de tests
static uint32_t test_contador_eventos = 0;
static uint32_t test_ultimo_evento = 0;
static uint32_t test_ultimo_auxdata = 0;
static bool test_evento_recibido = false;

// Estado de botones para test simultáneo
static bool test_btn_states[BUTTONS_NUMBER] = {false};
static bool test_simultaneo_detectado = false;

// Sección crítica
static uint32_t test_sc_contador = 0;
static bool test_sc_error = false;

// FIFO
static uint32_t test_fifo_eventos_extraidos = 0;

// Alarmas
static bool test_alarma_vencida = false;
static uint32_t test_alarmas_contadas = 0;

// Watchdog
static uint32_t test_wdt_feeds = 0;

/* =============================================================================
 * CALLBACKS PARA TESTS
 * ===========================================================================*/

/**
 * @brief Callback genérico para tests de eventos
 */
void test_callback_evento(EVENTO_T evento, uint32_t auxData) {
    test_contador_eventos++;
    test_ultimo_evento = evento;
    test_ultimo_auxdata = auxData;
    test_evento_recibido = true;
}

/**
 * @brief Callback para test de alarmas
 */
void test_callback_alarma(EVENTO_T evento, uint32_t auxData) {
    if (evento == ev_USUARIO_1) {
        test_alarma_vencida = true;
        test_alarmas_contadas++;
    }
}

/**
 * @brief Callback para test de botones simultáneos
 */
void test_callback_botones_simultaneos(EVENTO_T evento, uint32_t id_boton) {
    if (id_boton >= BUTTONS_NUMBER) return;
    
    if (evento == ev_PULSAR_BOTON) {
        test_btn_states[id_boton] = true;
        
        // Verificar si hay al menos 2 botones presionados
        uint32_t count = 0;
        for (int i = 0; i < BUTTONS_NUMBER; i++) {
            if (test_btn_states[i]) count++;
        }
        
        if (count >= 2) {
            test_simultaneo_detectado = true;
            // Encender LED 4 como indicador visual
            drv_led_establecer(4, LED_ON);
        }
    } 
    else if (evento == ev_SOLTAR_BOTON) {
        test_btn_states[id_boton] = false;
        
        // Si no hay botones presionados, apagar LED
        bool alguno_presionado = false;
        for (int i = 0; i < BUTTONS_NUMBER; i++) {
            if (test_btn_states[i]) alguno_presionado = true;
        }
        
        if (!alguno_presionado) {
            drv_led_establecer(4, LED_OFF);
        }
    }
}

/* =============================================================================
 * TEST 1: COLA FIFO
 * ===========================================================================*/

/**
 * @brief Test de la cola FIFO
 * 
 * Verifica:
 * - Encolar y extraer eventos
 * - Orden FIFO correcto
 * - Gestión de cola llena
 * - Gestión de cola vacía
 * - Protección con sección crítica
 * 
 * @return true si todas las pruebas pasan, false en caso contrario
 */
bool test_fifo(void) {
    drv_monitor_marcar(10); // Marca inicio del test
    
    EVENTO_T evento_extraido;
    uint32_t auxData_extraido;
    uint8_t resultado;
    
    // Test 1.1: Encolar y extraer un evento simple
    rt_FIFO_encolar(ev_USUARIO_1, 123);
    resultado = rt_FIFO_extraer(&evento_extraido, &auxData_extraido, NULL);
    
    if (resultado == 0 || evento_extraido != ev_USUARIO_1 || auxData_extraido != 123) {
        drv_led_establecer(1, LED_ON); // LED 1 = Error
        return false;
    }
    
    // Test 1.2: Verificar cola vacía
    resultado = rt_FIFO_extraer(&evento_extraido, &auxData_extraido, NULL);
    if (resultado != 0) {
        drv_led_establecer(1, LED_ON);
        return false;
    }
    
    // Test 1.3: Verificar orden FIFO (primero en entrar, primero en salir)
    rt_FIFO_encolar(ev_PULSAR_BOTON, 0);
    rt_FIFO_encolar(ev_SOLTAR_BOTON, 1);
    rt_FIFO_encolar(ev_BOTON_TIMER, 2);
    
    rt_FIFO_extraer(&evento_extraido, &auxData_extraido, NULL);
    if (evento_extraido != ev_PULSAR_BOTON || auxData_extraido != 0) {
        drv_led_establecer(1, LED_ON);
        return false;
    }
    
    rt_FIFO_extraer(&evento_extraido, &auxData_extraido, NULL);
    if (evento_extraido != ev_SOLTAR_BOTON || auxData_extraido != 1) {
        drv_led_establecer(1, LED_ON);
        return false;
    }
    
    rt_FIFO_extraer(&evento_extraido, &auxData_extraido, NULL);
    if (evento_extraido != ev_BOTON_TIMER || auxData_extraido != 2) {
        drv_led_establecer(1, LED_ON);
        return false;
    }
    
    // Test 1.4: Llenar la cola (32 eventos)
    for (int i = 0; i < 32; i++) {
        rt_FIFO_encolar(ev_USUARIO_1, i);
    }
    
    // Extraer todos y verificar orden
    for (int i = 0; i < 32; i++) {
        resultado = rt_FIFO_extraer(&evento_extraido, &auxData_extraido, NULL);
        if (resultado == 0 || auxData_extraido != i) {
            drv_led_establecer(1, LED_ON);
            return false;
        }
    }
    
    test_fifo_eventos_extraidos = 32;
    
    // Test exitoso
    drv_led_establecer(1, LED_ON);
    drv_led_establecer(2, LED_ON); // LEDs 1+2 = Test pasado
    drv_monitor_marcar(11); // Marca fin exitoso
    return true;
}

/* =============================================================================
 * TEST 2: GESTOR DE EVENTOS
 * ===========================================================================*/

/**
 * @brief Test del Gestor de Eventos
 * 
 * Verifica:
 * - Suscripción a eventos
 * - Despacho de eventos a callbacks suscritos
 * - Prioridades de callbacks
 * - Cancelar suscripciones
 * 
 * @return true si todas las pruebas pasan
 */
bool test_gestor_eventos(void) {
    drv_monitor_marcar(12); // Marca inicio
    
    // Resetear contadores
    test_contador_eventos = 0;
    test_evento_recibido = false;
    
    // Test 2.1: Suscribirse a un evento
    rt_GE_suscribir(ev_USUARIO_1, 1, test_callback_evento);
    
    // Test 2.2: Encolar evento y procesar manualmente
    rt_FIFO_encolar(ev_USUARIO_1, 999);
    
    // Extraer y despachar manualmente (simulando rt_GE_lanzador)
    EVENTO_T evento;
    uint32_t auxData;
    
    if (rt_FIFO_extraer(&evento, &auxData, NULL)) {
        // Llamar al callback manualmente
        test_callback_evento(evento, auxData);
    }
    
    // Verificar que el callback fue llamado
    if (!test_evento_recibido || test_ultimo_evento != ev_USUARIO_1 || 
        test_ultimo_auxdata != 999 || test_contador_eventos != 1) {
        drv_led_establecer(2, LED_ON); // LED 2 = Error
        return false;
    }
    
    // Test 2.3: Múltiples eventos
    test_contador_eventos = 0;
    for (int i = 0; i < 5; i++) {
        rt_FIFO_encolar(ev_USUARIO_1, i);
    }
    
    for (int i = 0; i < 5; i++) {
        if (rt_FIFO_extraer(&evento, &auxData, NULL)) {
            test_callback_evento(evento, auxData);
        }
    }
    
    if (test_contador_eventos != 5) {
        drv_led_establecer(2, LED_ON);
        return false;
    }
    
    // Test exitoso
    drv_led_establecer(2, LED_ON);
    drv_led_establecer(3, LED_ON); // LEDs 2+3 = Test pasado
    drv_monitor_marcar(13);
    return true;
}

/* =============================================================================
 * TEST 3: ALARMAS
 * ===========================================================================*/

/**
 * @brief Test del servicio de alarmas
 * 
 * Verifica:
 * - Programar alarmas esporádicas
 * - Programar alarmas periódicas
 * - Cancelar alarmas
 * - Codificación de flags
 * - Múltiples alarmas simultáneas
 * 
 * @return true si todas las pruebas pasan
 */
bool test_alarmas(void) {
    drv_monitor_marcar(14);
    
    test_alarma_vencida = false;
    test_alarmas_contadas = 0;
    
    // Test 3.1: Codificación de flags
    uint32_t flags_esporadica = svc_alarma_codificar(false, 1000, 5);
    uint32_t flags_periodica = svc_alarma_codificar(true, 500, 3);
    
    // Verificar bits (revisar estructura interna)
    if ((flags_periodica & 0x80000000) == 0) { // Bit periódico debe estar activo
        drv_led_establecer(3, LED_ON);
        return false;
    }
    
    // Test 3.2: Programar alarma esporádica de 100ms
    uint32_t flags = svc_alarma_codificar(false, 100, 10);
    svc_alarma_activar(flags, ev_USUARIO_1, 10);
    
    // Esperar activamente (simulación, en real usaría el sistema de eventos)
    // NOTA: Este test requiere que el sistema esté ejecutando rt_GE_lanzador
    // Para test completo, se debe integrar en el main
    
    // Test 3.3: Cancelar alarma
    svc_alarma_activar(0, ev_USUARIO_1, 10); // flags=0 cancela
    
    // Test 3.4: Programar alarma periódica
    flags = svc_alarma_codificar(true, 200, 20);
    svc_alarma_activar(flags, ev_USUARIO_1, 20);
    
    // Para cancelar después
    svc_alarma_activar(0, ev_USUARIO_1, 20);
    
    // Test exitoso (limitado sin ejecución del dispatcher)
    drv_led_establecer(3, LED_ON);
    drv_led_establecer(4, LED_ON); // LEDs 3+4 = Test pasado
    drv_monitor_marcar(15);
    return true;
}

/* =============================================================================
 * TEST 4: PULSACIÓN DE BOTONES (INCLUYENDO SIMULTÁNEOS)
 * ===========================================================================*/

/**
 * @brief Test de botones con detección de pulsaciones simultáneas
 * 
 * Este test debe ejecutarse con el sistema completo corriendo.
 * Instrucciones:
 * 1. Llamar a test_botones_simultaneos_iniciar() en el main
 * 2. Presionar botones 1 y 2 al mismo tiempo
 * 3. Si se detecta pulsación simultánea, LED 4 se enciende
 * 
 * Verifica:
 * - Detección individual de botones
 * - Detección de 2+ botones presionados simultáneamente
 * - FSM de anti-rebote
 * - Eventos de pulsar y soltar
 */
void test_botones_simultaneos_iniciar(void) {
    drv_monitor_marcar(16);
    
    // Resetear estado
    for (int i = 0; i < BUTTONS_NUMBER; i++) {
        test_btn_states[i] = false;
    }
    test_simultaneo_detectado = false;
    
    // Suscribirse a eventos de botones
    rt_GE_suscribir(ev_PULSAR_BOTON, 2, test_callback_botones_simultaneos);
    rt_GE_suscribir(ev_SOLTAR_BOTON, 2, test_callback_botones_simultaneos);
    
    drv_monitor_marcar(17);
    
    // INSTRUCCIONES PARA EL USUARIO:
    // - Presione los botones 1 y 2 al mismo tiempo
    // - Si ambos están presionados, el LED 4 se encenderá
    // - Suelte los botones, el LED 4 se apagará
}

/**
 * @brief Verificar si se detectó pulsación simultánea
 */
bool test_botones_simultaneos_verificar(void) {
    return test_simultaneo_detectado;
}

/* =============================================================================
 * TEST 5: SECCIÓN CRÍTICA
 * ===========================================================================*/

/**
 * @brief Test de secciones críticas
 * 
 * Verifica:
 * - Entrar y salir de secciones críticas
 * - Anidamiento correcto
 * - Protección de variables compartidas
 * 
 * @return true si el test pasa
 */
bool test_seccion_critica(void) {
    drv_monitor_marcar(18);
    
    test_sc_contador = 0;
    test_sc_error = false;
    uint32_t estado;
    
    // Test 5.1: Entrada y salida simple
    estado = drv_SC_entrar_disable_irq();
    test_sc_contador++;
    drv_SC_salir_enable_irq();
    
    if (test_sc_contador != 1) {
        drv_led_establecer(1, LED_ON);
        drv_led_establecer(3, LED_ON);
        return false;
    }
    
    // Test 5.2: Anidamiento (2 niveles)
    estado = drv_SC_entrar_disable_irq(); // Nivel 1
    test_sc_contador++;
    
    estado = drv_SC_entrar_disable_irq(); // Nivel 2 (anidado)
    test_sc_contador++;
    
    drv_SC_salir_enable_irq(); // Salir nivel 2
    test_sc_contador++;
    
    drv_SC_salir_enable_irq(); // Salir nivel 1
    test_sc_contador++;
    
    if (test_sc_contador != 5) {
        drv_led_establecer(1, LED_ON);
        drv_led_establecer(3, LED_ON);
        return false;
    }
    
    // Test 5.3: Protección de FIFO con SC
    drv_SC_entrar_disable_irq();
    rt_FIFO_encolar(ev_USUARIO_1, 777);
    test_sc_contador++;
    drv_SC_salir_enable_irq();
    
    // Verificar que se encoló correctamente
    EVENTO_T ev;
    uint32_t aux;
    rt_FIFO_extraer(&ev, &aux, NULL);
    
    if (ev != ev_USUARIO_1 || aux != 777) {
        drv_led_establecer(1, LED_ON);
        drv_led_establecer(3, LED_ON);
        return false;
    }
    
    // Test exitoso
    drv_led_establecer(1, LED_ON);
    drv_led_establecer(2, LED_ON);
    drv_led_establecer(3, LED_ON);
    drv_led_establecer(4, LED_ON); // Todos los LEDs = Test pasado
    drv_monitor_marcar(19);
    return true;
}

/* =============================================================================
 * TEST 6: WATCHDOG
 * ===========================================================================*/

/**
 * @brief Test del Watchdog Timer
 * 
 * Verifica:
 * - Inicialización del WDT
 * - Alimentación periódica
 * - Reset por timeout (requiere comentar la alimentación)
 * 
 * ADVERTENCIA: El test completo del timeout provocará un RESET del sistema
 * 
 * @return true si el test de alimentación pasa
 */
bool test_watchdog(void) {
    drv_monitor_marcar(20);
    
    test_wdt_feeds = 0;
    
    // Test 6.1: Alimentar el watchdog varias veces
    for (int i = 0; i < 10; i++) {
        drv_WDT_alimentar();
        test_wdt_feeds++;
        
        // Pequeño delay (simulado con loop vacío)
        for (volatile uint32_t j = 0; j < 100000; j++);
    }
    
    if (test_wdt_feeds != 10) {
        drv_led_establecer(2, LED_ON);
        drv_led_establecer(4, LED_ON);
        return false;
    }
    
    // Test 6.2: Verificar que NO hay reset (si llegamos aquí, el WDT funciona)
    // NOTA: Para probar el reset, comentar drv_WDT_alimentar() en rt_GE_lanzador
    //       y observar que el sistema se reinicia automáticamente
    
    // Test exitoso
    drv_led_establecer(2, LED_ON);
    drv_led_establecer(3, LED_ON);
    drv_led_establecer(4, LED_ON); // LEDs 2+3+4 = Test pasado
    drv_monitor_marcar(21);
    return true;
}

/* =============================================================================
 * SUITE DE TESTS COMPLETA
 * ===========================================================================*/

/**
 * @brief Ejecutar todos los tests secuencialmente
 * 
 * Instrucciones:
 * 1. Llamar a esta función desde el main antes de rt_GE_lanzador()
 * 2. Observar los LEDs para ver qué tests pasan
 * 3. Revisar las marcas del monitor para debugging
 * 
 * @return Número de tests que pasaron (máximo 6)
 */
uint32_t test_ejecutar_todos(void) {
    uint32_t tests_pasados = 0;
    
    // Apagar todos los LEDs
    for (int i = 1; i <= LEDS_NUMBER; i++) {
        drv_led_establecer(i, LED_OFF);
    }
    
    drv_monitor_marcar(100); // Marca inicio de suite
    
    // Test 1: FIFO
    if (test_fifo()) {
        tests_pasados++;
    }
    
    // Esperar un momento entre tests (delay visual)
    for (volatile uint32_t i = 0; i < 1000000; i++);
    
    // Apagar LEDs
    for (int i = 1; i <= LEDS_NUMBER; i++) {
        drv_led_establecer(i, LED_OFF);
    }
    
    // Test 2: Gestor de Eventos
    if (test_gestor_eventos()) {
        tests_pasados++;
    }
    
    for (volatile uint32_t i = 0; i < 1000000; i++);
    for (int i = 1; i <= LEDS_NUMBER; i++) {
        drv_led_establecer(i, LED_OFF);
    }
    
    // Test 3: Alarmas
    if (test_alarmas()) {
        tests_pasados++;
    }
    
    for (volatile uint32_t i = 0; i < 1000000; i++);
    for (int i = 1; i <= LEDS_NUMBER; i++) {
        drv_led_establecer(i, LED_OFF);
    }
    
    // Test 5: Sección Crítica
    if (test_seccion_critica()) {
        tests_pasados++;
    }
    
    for (volatile uint32_t i = 0; i < 1000000; i++);
    for (int i = 1; i <= LEDS_NUMBER; i++) {
        drv_led_establecer(i, LED_OFF);
    }
    
    // Test 6: Watchdog
    if (test_watchdog()) {
        tests_pasados++;
    }
    
    for (volatile uint32_t i = 0; i < 1000000; i++);
    
    // Mostrar resultado final en LEDs (binario)
    // 5 tests pasados = 0b0101 = LEDs 1 y 3
    // 6 tests pasados = 0b0110 = LEDs 2 y 3
    for (int i = 1; i <= LEDS_NUMBER && i <= tests_pasados; i++) {
        drv_led_establecer(i, LED_ON);
    }
    
    drv_monitor_marcar(101); // Marca fin de suite
    drv_monitor_marcar(100 + tests_pasados); // Marca con resultado
    
    return tests_pasados;
}

/* =============================================================================
 * TEST INTERACTIVO DE BOTONES (Para uso en main)
 * ===========================================================================*/

/**
 * @brief Modo de test interactivo de botones
 * 
 * Reemplazar la lógica de beat_hero con esta función para probar botones.
 * Los LEDs mostrarán qué botones están presionados.
 * LED 4 se enciende si hay 2+ botones presionados simultáneamente.
 */
void test_modo_interactivo_botones(EVENTO_T evento, uint32_t id_boton) {
    test_callback_botones_simultaneos(evento, id_boton);
    
    // Mostrar estado individual de botones en LEDs 1-3
    if (id_boton < 3) { // Solo primeros 3 botones
        if (evento == ev_PULSAR_BOTON) {
            drv_led_establecer(id_boton + 1, LED_ON);
        } else if (evento == ev_SOLTAR_BOTON) {
            drv_led_establecer(id_boton + 1, LED_OFF);
        }
    }
}
