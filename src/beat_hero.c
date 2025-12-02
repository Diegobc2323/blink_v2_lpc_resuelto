#include "beat_hero.h"
#include <stdbool.h>
#include <stdlib.h>

// Drivers
#include "board.h"
#include "rt_GE.h"
#include "svc_alarmas.h"
#include "drv_leds.h"
#include "drv_botones.h"
#include "drv_tiempo.h"
#include "drv_aleatorios.h"
#include "drv_sonido.h"

// Configuración
#define MAX_COMPASES_PARTIDA    30
#define SCORE_MIN_FAIL          -5
#define BPM_INICIAL             60
#define MS_POR_MINUTO           60000
#define TIEMPO_REINICIO_MS      3000

// IDs Mágicos para alarmas (en auxData)
#define ID_ALARMA_RESET         999
#define ID_ALARMA_TICK          100
#define ID_ALARMA_DEMO          200

// Variables de Estado
static uint8_t compas[3]; 
typedef enum { e_INIT, e_JUEGO, e_RESULTADO } GameState_t;
static GameState_t s_estado = e_INIT;

static int32_t  s_score = 0;
static int32_t  s_high_score = 0; 
static uint32_t s_compases_jugados = 0;
static uint32_t s_duracion_compas_ms = 1000;
static Tiempo_us_t s_tiempo_inicio_compas = 0;
static uint8_t  s_nivel_dificultad = 1;

// Prototipos
static void reiniciar_variables_juego(void);
static void avanzar_compas(void);
static void actualizar_leds_display(void);
static void evaluar_jugada(uint8_t botones_pulsados);
static void programar_siguiente_tick(void);
static void finalizar_partida(bool exito);
static int calcular_puntuacion(Tiempo_us_t now);

// Función de actualización principal
void beat_hero_actualizar(EVENTO_T evento, uint32_t auxData);

void beat_hero_iniciar(void) {
    s_estado = e_INIT;
    s_high_score = 0; 

    // Suscripciones
    rt_GE_suscribir(ev_JUEGO_NUEVO_LED, 1, beat_hero_actualizar);
    rt_GE_suscribir(ev_PULSAR_BOTON, 1, beat_hero_actualizar);
    rt_GE_suscribir(ev_SOLTAR_BOTON, 1, beat_hero_actualizar);
    // Reutilizamos ev_JUEGO_TIMEOUT para el timer de reset
    rt_GE_suscribir(ev_JUEGO_TIMEOUT, 1, beat_hero_actualizar); 
    
    reiniciar_variables_juego(); 
    // Iniciar parpadeo attract mode (ID_ALARMA_DEMO)
    svc_alarma_activar(svc_alarma_codificar(false, 500, ID_ALARMA_DEMO), ev_JUEGO_NUEVO_LED, ID_ALARMA_DEMO);
}

void beat_hero_actualizar(EVENTO_T evento, uint32_t auxData) {
    
    // FIX: Si estamos en partida, cualquier interacción reinicia el timer de inactividad global
    // Esto evita el "apagón" si el usuario juega muy rápido o mantiene botones
    if (s_estado == e_JUEGO && (evento == ev_PULSAR_BOTON || evento == ev_SOLTAR_BOTON)) {
        // Reiniciar alarma de inactividad del sistema (10s típicamente)
        svc_alarma_activar(svc_alarma_codificar(false, 10000, 0), ev_INACTIVIDAD, 0);
    }

    switch (s_estado) {
        
        case e_INIT:
            if (evento == ev_JUEGO_NUEVO_LED && auxData == ID_ALARMA_DEMO) {
                // Attract Mode (Luces rotatorias)
                static uint8_t led_demo = 1;
                drv_led_establecer(led_demo, LED_OFF);
                led_demo = (led_demo % LEDS_NUMBER) + 1;
                drv_led_establecer(led_demo, LED_ON);
                
                // Reprogramar siguiente tick de demo
                svc_alarma_activar(svc_alarma_codificar(false, 500, ID_ALARMA_DEMO), ev_JUEGO_NUEVO_LED, ID_ALARMA_DEMO);
            }
            
            // FIX: Ignorar explícitamente botones 3 y 4 en e_INIT para evitar resets fantasma
            if (evento == ev_PULSAR_BOTON && (auxData == 2 || auxData == 3)) {
                return; 
            }

            // Empezar partida con Botón 1 o 2 (Indices 0 o 1)
            if (evento == ev_PULSAR_BOTON && (auxData == 0 || auxData == 1)) {
                // Apagar demo: cancelamos alarma de demo
                svc_alarma_activar(0, ev_JUEGO_NUEVO_LED, ID_ALARMA_DEMO);
                for(int i=1; i<=LEDS_NUMBER; i++) drv_led_establecer(i, LED_OFF);
                
                // Configurar inicio
                s_estado = e_JUEGO;
                reiniciar_variables_juego();
                s_duracion_compas_ms = MS_POR_MINUTO / BPM_INICIAL;
                
                // Arrancar el juego
                programar_siguiente_tick();
            }
            break;

        case e_JUEGO:
            // Abortar con botones 3 o 4
            if (evento == ev_PULSAR_BOTON && (auxData == 2 || auxData == 3)) {
                finalizar_partida(false); 
                return;
            }

            if (evento == ev_JUEGO_NUEVO_LED && auxData == ID_ALARMA_TICK) {
                // 1. Verificar fin por tiempo (compases)
                if (s_compases_jugados >= MAX_COMPASES_PARTIDA) {
                    finalizar_partida(true); 
                    return;
                }
                // 2. Verificar fin por puntuación baja (por si acaso falló mucho)
                if (s_score < SCORE_MIN_FAIL) {
                    finalizar_partida(false); 
                    return;
                }

                // 3. Lógica de juego
                avanzar_compas();
                actualizar_leds_display();
                
                // Dificultad
                if (s_compases_jugados % 5 == 0 && s_compases_jugados > 0) {
                    s_nivel_dificultad++;
                    if(s_nivel_dificultad > 4) s_nivel_dificultad = 4;
                    if (s_nivel_dificultad == 4) s_duracion_compas_ms = (uint32_t)(s_duracion_compas_ms * 0.9f);
                }

                s_tiempo_inicio_compas = drv_tiempo_actual_us();
                s_compases_jugados++; 
                programar_siguiente_tick();
            }
            else if (evento == ev_PULSAR_BOTON && auxData <= 1) {
                // Evaluar botones de juego (1 y 2)
                evaluar_jugada(1 << auxData);
                
                // IMPORTANTE: Verificar Game Over INMEDIATAMENTE tras pulsar
                if (s_score < SCORE_MIN_FAIL) {
                    finalizar_partida(false);
                }
            }
            break;

        case e_RESULTADO:
            // INICIO RESET: Pulsar botón 3 o 4
            if (evento == ev_PULSAR_BOTON && (auxData == 2 || auxData == 3)) {
                // Programar alarma de 3 segundos (ID_ALARMA_RESET)
                uint32_t flags = svc_alarma_codificar(false, TIEMPO_REINICIO_MS, ID_ALARMA_RESET);
                svc_alarma_activar(flags, ev_JUEGO_TIMEOUT, ID_ALARMA_RESET);
            }
            
            // CANCELAR RESET: Soltar botón antes de tiempo
            if (evento == ev_SOLTAR_BOTON && (auxData == 2 || auxData == 3)) {
                svc_alarma_activar(0, ev_JUEGO_TIMEOUT, ID_ALARMA_RESET);
            }
            
            // EJECUTAR RESET: La alarma venció
            if (evento == ev_JUEGO_TIMEOUT && auxData == ID_ALARMA_RESET) {
                reiniciar_variables_juego();
                s_estado = e_INIT;
                // Reiniciar demo
                svc_alarma_activar(svc_alarma_codificar(false, 500, ID_ALARMA_DEMO), ev_JUEGO_NUEVO_LED, ID_ALARMA_DEMO);
            }
            break;
            
        default: break;
    }
}

static void finalizar_partida(bool exito) {
    s_estado = e_RESULTADO;
    // Cancelar tick de juego pendiente
    svc_alarma_activar(0, ev_JUEGO_NUEVO_LED, ID_ALARMA_TICK);
    
    // Apagar todo primero
    for(int i=1; i<=LEDS_NUMBER; i++) drv_led_establecer(i, LED_OFF);
    
    if (exito) {
        // Victoria: Todo ON
        for(int i=1; i<=LEDS_NUMBER; i++) drv_led_establecer(i, LED_ON);
        if (s_score > s_high_score) s_high_score = s_score;
    } else {
        // Derrota: 1 y 4 ON
        drv_led_establecer(1, LED_ON);
        drv_led_establecer(4, LED_ON);
    }
}

static void reiniciar_variables_juego(void) {
    s_score = 0; 
    s_compases_jugados = 0; 
    s_nivel_dificultad = 1; 
    s_duracion_compas_ms = 1000;
    compas[0]=0; compas[1]=0; compas[2]=0;
    for(int i=1; i<=LEDS_NUMBER; i++) drv_led_establecer(i, LED_OFF);
    
    // FIX: Asegurar que no hay alarmas de reset pendientes de la partida anterior
    svc_alarma_activar(0, ev_JUEGO_TIMEOUT, ID_ALARMA_RESET);
}

static void avanzar_compas(void) {
    compas[0] = compas[1]; compas[1] = compas[2];
    uint32_t rnd = drv_aleatorios_rango(100);
    uint8_t p = 0;
    
    if (s_nivel_dificultad == 1)       p = (rnd > 50) ? 1 : 2;
    else if (s_nivel_dificultad == 2) p = (rnd < 20) ? 0 : ((rnd < 60) ? 1 : 2);
    else                              p = (rnd < 15) ? 0 : ((rnd < 45) ? 1 : ((rnd < 75) ? 2 : 3));
    compas[2] = p;
}

static void actualizar_leds_display(void) {
    // Mostrar notas (ajusta indices segun hardware)
    drv_led_establecer(1, (compas[2] & 1) ? LED_ON : LED_OFF);
    drv_led_establecer(2, (compas[2] & 2) ? LED_ON : LED_OFF);
    drv_led_establecer(3, (compas[1] & 1) ? LED_ON : LED_OFF);
    drv_led_establecer(4, (compas[1] & 2) ? LED_ON : LED_OFF);
}

static void programar_siguiente_tick(void) {
    // Usamos ID_ALARMA_TICK para distinguirlo de la demo
    svc_alarma_activar(svc_alarma_codificar(false, s_duracion_compas_ms, ID_ALARMA_TICK), ev_JUEGO_NUEVO_LED, ID_ALARMA_TICK);
}

static int calcular_puntuacion(Tiempo_us_t now) {
    uint32_t diff_ms = (uint32_t)((now - s_tiempo_inicio_compas) / 1000);
    if(s_duracion_compas_ms == 0) return 0;
    uint32_t pct = (diff_ms * 100) / s_duracion_compas_ms;
    if (pct <= 10) return 2;
    if (pct <= 20) return 1;
    if (pct <= 40) return 0;
    return -1;
}

static void evaluar_jugada(uint8_t input_mask) {
    if (compas[0] == 0) { 
        s_score--; 
        return; 
    }
    if (compas[0] & input_mask) {
        s_score += calcular_puntuacion(drv_tiempo_actual_us());
        compas[0] &= ~input_mask; 
    } else {
        s_score--;
    }
}