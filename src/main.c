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
#include "drv_sonido.h"
#include "test.h"

volatile uint32_t testsPasados = 0;

int main(void){
    // 1. Inicializaciones de Hardware básico
    drv_tiempo_iniciar(); 
    hal_gpio_iniciar(); 
    drv_consumo_iniciar(4); 
    drv_monitor_iniciar();
    drv_sonido_iniciar();
    drv_leds_iniciar();
  
    // 2. Inicializaciones del Sistema (Runtime)
    // Inicializamos la FIFO y el Gestor, pero AÚN NO los generadores de eventos
    rt_FIFO_inicializar(1); // Monitor ID 2
    rt_GE_iniciar(3);       // Monitor ID 3
    
    // ------------------------------------------------------
    // 3. EJECUCIÓN DE TESTS AUTOMÁTICOS (Zona Segura)
    // ------------------------------------------------------
    // Los ejecutamos AHORA, antes de que las alarmas o botones 
    // empiecen a llenar la cola y causen overflow durante el test.
    testsPasados = test_ejecutar_todos();

    // ------------------------------------------------------
    // 4. Inicialización de Servicios y Drivers (El "Grifo" de eventos)
    // ------------------------------------------------------
    // Ahora que los tests pasaron, activamos el resto del sistema
    svc_alarma_iniciar(4, rt_FIFO_encolar, ev_T_PERIODICO); 
    drv_botones_iniciar(rt_FIFO_encolar, ev_PULSAR_BOTON, ev_SOLTAR_BOTON, ev_BOTON_TIMER);
    drv_aleatorios_iniciar(0);

    // ------------------------------------------------------
    // 5. Test Interactivo (Opcional)
    // ------------------------------------------------------
    // Este test SÍ necesita los botones activados, por eso va aquí.
    // Descomentar si quieres probar los botones antes de jugar:
    // test_botones_simultaneos(); 

    // ------------------------------------------------------
    // 6. Arranque del Juego
    // ------------------------------------------------------
    beat_hero_iniciar();
    
    // Lanzar el despachador de eventos (Bucle infinito)
    rt_GE_lanzador();

    while (1);
}