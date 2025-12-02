#include <stdint.h>
#include <stdbool.h>
#include "hal_gpio.h"
#include "drv_leds.h"
#include "drv_tiempo.h"
#include "drv_consumo.h"
#include "drv_monitor.h"
#include "rt_fifo.h"
#include "drv_botones.h"
#include "rt_ge.h"
#include "svc_alarmas.h"
#include "rt_evento_t.h"
#include "board.h"
#include "drv_aleatorios.h" 
#include "beat_hero.h"
#include "app_jugar.h"//eliminarlo 

#include "drv_sonido.h"

// Estado global de los botones
static bool btn_states[BUTTONS_NUMBER] = {false};

//void app_test_simultaneo(EVENTO_T evento, uint32_t id_boton) {
//    // Solo nos interesan los 2 primeros botones para esta prueba
//    if (id_boton >= BUTTONS_NUMBER) return;

//    // 1. Actualizar estado
//    if (evento == ev_PULSAR_BOTON) {
//        btn_states[id_boton] = true;
//        // DEBUG VISUAL: LED 1 para Botón 1, LED 2 para Botón 2
//        if (id_boton == 0) drv_led_establecer(1, LED_ON);
//        if (id_boton == 1) drv_led_establecer(2, LED_ON);
//    } 
//    else if (evento == ev_SOLTAR_BOTON) {
//        btn_states[id_boton] = false;
//        // DEBUG VISUAL: Apagar al soltar
//        if (id_boton == 0) drv_led_establecer(1, LED_OFF);
//        if (id_boton == 1) drv_led_establecer(2, LED_OFF);
//    }

//    // 2. Lógica simultánea (LED 4)
//    if (btn_states[0] && btn_states[1]) {
//        drv_led_establecer(4, LED_ON);
//    } else {
//        drv_led_establecer(4, LED_OFF);
//    }
//}

int main(void){
    // 1. Inicializaciones de Hardware básico
    drv_tiempo_iniciar(); 
    hal_gpio_iniciar(); 
	  drv_consumo_iniciar(4); 
    drv_monitor_iniciar();
		drv_sonido_iniciar();
    drv_leds_iniciar();
  
    // 2. Inicializaciones del Sistema (Runtime)
    // IMPORTANTE: rt_GE_iniciar DEBE ir antes de cualquier driver que use suscripciones
    rt_FIFO_inicializar(1); // Monitor ID 2
    rt_GE_iniciar(3);       // Monitor ID 3 <--- MOVIDO AQUÍ
    
    // 3. Inicializaciones de Servicios y Drivers (que usan rt_GE)
    svc_alarma_iniciar(4, rt_FIFO_encolar, ev_T_PERIODICO); // Monitor ID 4
    
    // Iniciar Botones AHORA, después de que el GE esté listo
    drv_botones_iniciar(rt_FIFO_encolar, ev_PULSAR_BOTON, ev_SOLTAR_BOTON, ev_BOTON_TIMER);
  
    // 4. Suscripciones de la Aplicación
    
		// Driver de Aleatorios (NUEVO)
    // La semilla se ignora en nRF52 (usa ruido térmico), ponemos 0
    drv_aleatorios_iniciar(0);

    // ------------------------------------------------------
    // 4. Inicialización de la Aplicación (Nivel 3)
    // ------------------------------------------------------
    
    // Inicia la máquina de estados del juego y suscribe sus funciones
    beat_hero_iniciar();
		//app_juego_iniciar();
    // Lanzar sistema
    rt_GE_lanzador();

    while (1);
}