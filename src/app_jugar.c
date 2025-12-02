#include "app_jugar.h"     

#include <stdlib.h>         // rand() y srand()
#include "rt_GE.h"         
#include "svc_alarmas.h"    
#include "drv_leds.h"       
#include "drv_tiempo.h"     
#include "drv_WDT.h"
#include "drv_SC.h"
#include "board.h"


// Estados posibles del juego
typedef enum {
    e_JUEGO_ESPERA,   // Esperando para mostrar el siguiente LED
    e_JUEGO_JUGANDO     // LED encendido, esperando que pulsen
} JuegoEstado_t;


// Variables privadas
static JuegoEstado_t s_estado = e_JUEGO_ESPERA;
static uint32_t s_led_actual = 0;           // Que LED esta encendido (1 a N)
static uint32_t s_retardo_actual_ms = 2000; // Empieza en 2 segundos
static const uint32_t RETARDO_ENTRE_LEDS = 500; // Medio segundo entre rondas
static uint32_t leds_juego;


static void programar_siguiente_led(uint32_t retardo_ms) {
    uint32_t alarma_flags = svc_alarma_codificar(false, retardo_ms, 0);
    svc_alarma_activar(alarma_flags, ev_JUEGO_NUEVO_LED, 0);
    s_estado = e_JUEGO_ESPERA;
}


void app_juego_iniciar(void) {
    // Semilla aleatoria basada en el tiempo actual
    srand(drv_tiempo_actual_ms()); 
    leds_juego = (LEDS_NUMBER < BUTTONS_NUMBER) ? LEDS_NUMBER : BUTTONS_NUMBER;
    s_estado = e_JUEGO_ESPERA;
    s_retardo_actual_ms = 2000; 

    // Suscribirse a los eventos
    rt_GE_suscribir(ev_JUEGO_NUEVO_LED, 1, app_juego_actualizar);
    rt_GE_suscribir(ev_JUEGO_TIMEOUT, 1, app_juego_actualizar);
    rt_GE_suscribir(ev_PULSAR_BOTON, 1, app_juego_actualizar);
    
    // Empezar el juego en 1 segundo
    programar_siguiente_led(1000);
}

void app_juego_actualizar(EVENTO_T evento, uint32_t auxData) {
    
    switch (evento) {
        
        case ev_JUEGO_NUEVO_LED:
            if (s_estado == e_JUEGO_ESPERA) {
                // Elegir un LED al azar y encenderlo
                s_led_actual = (rand() % leds_juego) ;
                drv_led_establecer(s_led_actual+1, LED_ON);
                
                // Programar alarma de timeout
                uint32_t alarma_timeout = svc_alarma_codificar(false, s_retardo_actual_ms, 0);
                svc_alarma_activar(alarma_timeout, ev_JUEGO_TIMEOUT, 0);
                
                s_estado = e_JUEGO_JUGANDO;
            }
            break;

        case ev_JUEGO_TIMEOUT:
            if (s_estado == e_JUEGO_JUGANDO) {
                // Se acabo el tiempo, han fallado
                drv_led_establecer(s_led_actual+1, LED_OFF);
                
                // Volver a la dificultad inicial
                s_retardo_actual_ms = 2000; 
                
                // Siguiente LED
                programar_siguiente_led(RETARDO_ENTRE_LEDS);
            }
            break;

        case ev_PULSAR_BOTON:
			{
				int sc = drv_SC_entrar_disable_irq();

			  drv_WDT_alimentar(); 
		    drv_SC_salir_enable_irq();

            if (s_estado == e_JUEGO_JUGANDO) {
                // auxData tiene el ID del boton que se ha pulsado
                if (auxData == s_led_actual) {
                    // Acierto cancelamos el timeout
                    svc_alarma_activar(0, ev_JUEGO_TIMEOUT, 0);
                    
                    // Apagamos el LED
                    drv_led_establecer(s_led_actual+1, LED_OFF);
                    
                    // Miramos si reducimos dificultad
                    s_retardo_actual_ms = (uint32_t)(s_retardo_actual_ms * 0.95);
                    if (s_retardo_actual_ms < 300) { 
                        s_retardo_actual_ms = 300; // minimo 300ms
                    }
                    
                    // Siguiente LED
                    programar_siguiente_led(RETARDO_ENTRE_LEDS);
                    
                }
            }
            break;
			}
        default:
            // Eventos que no nos interesan
            break;
    }
}
