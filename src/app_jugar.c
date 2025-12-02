#include "app_jugar.h"     

// --- INCLUDES NECESARIOS PARA LA LÓGICA DEL JUEGO ---
#include <stdlib.h>         // Para rand() y srand()
#include "rt_GE.h"         // Para svc_GE_suscribir
#include "svc_alarmas.h"    // Para svc_alarma_codificar y svc_alarma_activar
#include "drv_leds.h"       // Para drv_led_establecer
#include "drv_tiempo.h"     // Para drv_tiempo_actual_ms
#include "drv_WDT.h"
#include "drv_SC.h"
#include "board.h"


// --- Estados de la Máquina de Estados (FSM) del Juego ---
typedef enum {
    e_JUEGO_ESPERA,   // Esperando para mostrar un nuevo LED
    e_JUEGO_JUGANDO     // LED encendido, esperando pulsación
} JuegoEstado_t;


// --- Variables estáticas del módulo (privadas) ---
static JuegoEstado_t s_estado = e_JUEGO_ESPERA;
static uint32_t s_led_actual = 0;           // LED (1 a N) que está activo
static uint32_t s_retardo_actual_ms = 2000; // 2 segundos iniciales
static const uint32_t RETARDO_ENTRE_LEDS = 500; // 0.5s de espera tras acierto
static uint32_t leds_juego;
/**
 * @brief Función interna (privada) para programar el siguiente LED.
 */
static void programar_siguiente_led(uint32_t retardo_ms) {
    uint32_t alarma_flags = svc_alarma_codificar(false, retardo_ms, 0);
    svc_alarma_activar(alarma_flags, ev_JUEGO_NUEVO_LED, 0);
    s_estado = e_JUEGO_ESPERA;
}

/**
 * @brief Inicializa el módulo del juego.
*/
void app_juego_iniciar(void) {
    // Inicializar el generador de números aleatorios
    srand(drv_tiempo_actual_ms()); 
    leds_juego = (LEDS_NUMBER < BUTTONS_NUMBER) ? LEDS_NUMBER : BUTTONS_NUMBER;
    s_estado = e_JUEGO_ESPERA;
    s_retardo_actual_ms = 2000; // Reiniciar retardo

    // Suscribir la FSM a los eventos que le importan
    rt_GE_suscribir(ev_JUEGO_NUEVO_LED, 1, app_juego_actualizar);
    rt_GE_suscribir(ev_JUEGO_TIMEOUT, 1, app_juego_actualizar);
    rt_GE_suscribir(ev_PULSAR_BOTON, 1, app_juego_actualizar);
    
    // Programar el primer LED para que empiece el juego
    programar_siguiente_led(1000); // Empezar en 1 segundo
}

/**
 * @brief Máquina de estados del juego (FSM).
 */
void app_juego_actualizar(EVENTO_T evento, uint32_t auxData) {
    
    switch (evento) {
        
        case ev_JUEGO_NUEVO_LED:
            if (s_estado == e_JUEGO_ESPERA) {
                // 1. Elegir y encender un LED aleatorio
                s_led_actual = (rand() % leds_juego) ;
                drv_led_establecer(s_led_actual+1, LED_ON);
                
                // 2. Programar el timeout (alarma de fallo)
                uint32_t alarma_timeout = svc_alarma_codificar(false, s_retardo_actual_ms, 0);
                svc_alarma_activar(alarma_timeout, ev_JUEGO_TIMEOUT, 0);
                
                s_estado = e_JUEGO_JUGANDO;
            }
            break;

        case ev_JUEGO_TIMEOUT:
            if (s_estado == e_JUEGO_JUGANDO) {
                // Fallo. El usuario no pulsó a tiempo
                drv_led_establecer(s_led_actual+1, LED_OFF);
                
                // Reiniciar dificultad
                s_retardo_actual_ms = 2000; 
                
                // Programar el siguiente LED
                programar_siguiente_led(RETARDO_ENTRE_LEDS);
            }
            break;

        case ev_PULSAR_BOTON:
				{
						int sc = drv_SC_entrar_disable_irq();

					  drv_WDT_alimentar(); //
				    drv_SC_salir_enable_irq();

            if (s_estado == e_JUEGO_JUGANDO) {
                // auxData contiene el ID del botón pulsado
                if (auxData == s_led_actual) {
                    // Acierto
                    // 1. Cancelar la alarma de timeout (programándola a 0)
                    svc_alarma_activar(0, ev_JUEGO_TIMEOUT, 0);
                    
                    // 2. Apagar el LED
                    drv_led_establecer(s_led_actual+1, LED_OFF);
                    
                    // 3. Aumentar dificultad (reducir retardo)
                    s_retardo_actual_ms = (uint32_t)(s_retardo_actual_ms * 0.95);
                    if (s_retardo_actual_ms < 300) { // Poner un límite
                        s_retardo_actual_ms = 300;
                    }
                    
                    // 4. Programar el siguiente LED
                    programar_siguiente_led(RETARDO_ENTRE_LEDS);
                    
                } else {
                    // Fallo. Pulsó el botón incorrecto
                    // *** NO HACEMOS NADA ***
                    // El LED sigue encendido y el timer de timeout sigue corriendo.
                    // Simplemente ignoramos la pulsación incorrecta.
                }
            }
            break;
					}
        default:
            // Ignorar otros eventos (como ev_T_PERIODICO)
            break;
    }
}
