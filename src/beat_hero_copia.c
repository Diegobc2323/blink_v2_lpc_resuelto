/* #include "beat_hero.h"
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

// Eventos
#define EV_JUEGO_TICK           ev_JUEGO_NUEVO_LED
#define EV_REINICIO_AUTO        ev_USUARIO_1

// Frecuencias de Sonido
#define TONE_START      1000 // 1000 Hz
#define TONE_WIN        2000 // 2000 Hz
#define TONE_LOSE       200  // 200 Hz
#define TONE_HIT        800  // 800 Hz (feedback breve al acertar)

// Estructura de Estadísticas para Debugging
typedef struct {
    // Tiempo
    uint32_t tiempo_juego_s;       // Tiempo acumulado en la partida actual
    
    // Puntuación
    int32_t  score_actual;
    int32_t  high_score;
    
    // Rendimiento
    uint32_t notas_acertadas;
    uint32_t notas_falladas;
    uint32_t racha_actual;         // Notas acertadas consecutivas
    uint32_t racha_maxima;         // Mejor racha en la partida
    
    // Estado
    uint8_t  nivel_dificultad;
    uint32_t partidas_jugadas;
    
} BeatHero_Stats_t;

// Variable global/estática visible para el debugger
static volatile BeatHero_Stats_t g_stats = {0};

// Variables de Estado
static uint8_t compas[3]; 
typedef enum { e_INIT, e_JUEGO, e_RESULTADO } GameState_t;
static GameState_t s_estado = e_INIT;

static int32_t  s_score = 0;
static int32_t  s_high_score = 0; // Se mantiene en RAM (sin Flash)
static uint32_t s_compases_jugados = 0;
static uint32_t s_duracion_compas_ms = 1000;
static Tiempo_us_t s_tiempo_inicio_compas = 0;
static uint8_t  s_nivel_dificultad = 1;

static bool s_alarma_reinicio_activa = false;

// Prototipos
static void reiniciar_juego(void);
static void avanzar_compas(void);
static void actualizar_leds_display(void);
static void evaluar_jugada(uint8_t botones_pulsados);
static void programar_siguiente_tick(void);
static void finalizar_partida(bool exito);
static int calcular_puntuacion(Tiempo_us_t now);
static void activar_alarma_reinicio(void);
static void cancelar_alarma_reinicio(void);

void beat_hero_iniciar(void) {
    s_estado = e_INIT;
    s_high_score = 0; // Reset inicial (o quitar para mantener tras soft-reset)
    
    // Inicializar estadísticas globales
    g_stats.high_score = 0;
    g_stats.partidas_jugadas = 0;

    rt_GE_suscribir(EV_JUEGO_TICK, 1, beat_hero_actualizar);
    rt_GE_suscribir(ev_PULSAR_BOTON, 1, beat_hero_actualizar);
    rt_GE_suscribir(ev_SOLTAR_BOTON, 1, beat_hero_actualizar);
    rt_GE_suscribir(EV_REINICIO_AUTO, 1, beat_hero_actualizar);
    
    reiniciar_juego(); 
    programar_siguiente_tick();
}

void beat_hero_actualizar(EVENTO_T evento, uint32_t auxData) {
    
    switch (s_estado) {
        
        case e_INIT:
            if (evento == EV_JUEGO_TICK) {
                // Attract Mode
                static uint8_t led_demo = 1;
                drv_led_establecer(led_demo, LED_OFF);
                led_demo = (led_demo % LEDS_NUMBER) + 1;
                drv_led_establecer(led_demo, LED_ON);
                
                s_duracion_compas_ms = 500;
                programar_siguiente_tick();
            }
            // Empezar partida con Botón 1 o 2
            if (evento == ev_PULSAR_BOTON && (auxData == 0 || auxData == 1)) {
                // Apagar demo y Sonido START
                for(int i=1; i<=LEDS_NUMBER; i++) drv_led_establecer(i, LED_OFF);
                //drv_sonido_tocar(TONE_START, 200);

                s_estado = e_JUEGO;
                s_duracion_compas_ms = MS_POR_MINUTO / BPM_INICIAL;
                s_compases_jugados = 0;
                s_score = 0;
                compas[0]=0; compas[1]=0; compas[2]=0;
                
                // Actualizar stats al inicio real
                g_stats.score_actual = 0;
                g_stats.nivel_dificultad = 1;
                
                programar_siguiente_tick();
            }
            break;

        case e_JUEGO:
            // Abortar si pulsan 3 o 4
            if (evento == ev_PULSAR_BOTON && (auxData == 2 || auxData == 3)) {
                finalizar_partida(false); 
                return;
            }

            if (evento == EV_JUEGO_TICK) {
                if (s_compases_jugados >= MAX_COMPASES_PARTIDA) {
                    finalizar_partida(true); return;
                }
                if (s_score < SCORE_MIN_FAIL) {
                    finalizar_partida(false); return;
                }

                avanzar_compas();
                actualizar_leds_display();
                
                // Actualizar tiempo de juego (aprox, sumando duración de compás)
                g_stats.tiempo_juego_s += (s_duracion_compas_ms / 1000);
                
                // Dificultad
                if (s_compases_jugados % 5 == 0 && s_compases_jugados > 0) {
                    s_nivel_dificultad++;
                    if(s_nivel_dificultad > 4) s_nivel_dificultad = 4;
                    if (s_nivel_dificultad == 4) s_duracion_compas_ms = (uint32_t)(s_duracion_compas_ms * 0.9f);
                    
                    g_stats.nivel_dificultad = s_nivel_dificultad;
                }

                s_tiempo_inicio_compas = drv_tiempo_actual_us();
                s_compases_jugados++;
                programar_siguiente_tick();
            }
            else if (evento == ev_PULSAR_BOTON && auxData <= 1) {
                evaluar_jugada(1 << auxData);
            }
            break;

        case e_RESULTADO:
            // Reiniciar con pulsación larga en 3 o 4 usando alarmas
            if (evento == ev_PULSAR_BOTON && (auxData == 2 || auxData == 3)) {
                // Activar alarma de 3 segundos para reinicio automático
                activar_alarma_reinicio();
            }
            if (evento == ev_SOLTAR_BOTON && (auxData == 2 || auxData == 3)) {
                // Si sueltan antes de 3 segundos, cancelar la alarma
                cancelar_alarma_reinicio();
            }
            if (evento == EV_REINICIO_AUTO) {
                // La alarma se disparó - reiniciar automáticamente
                s_alarma_reinicio_activa = false;
                reiniciar_juego();
                s_estado = e_INIT;
                programar_siguiente_tick();
            }
            break;
            
        default: break;
    }
}

static void finalizar_partida(bool exito) {
    s_estado = e_RESULTADO;
    for(int i=1; i<=LEDS_NUMBER; i++) drv_led_establecer(i, LED_OFF);
    
    if (exito) {
        // Victoria
        for(int i=1; i<=LEDS_NUMBER; i++) drv_led_establecer(i, LED_ON);
        //drv_sonido_tocar(TONE_WIN, 400); // 400ms sonido
        
        if (s_score > s_high_score) s_high_score = s_score; // Guardar en RAM
    } else {
        // Derrota
        drv_led_establecer(1, LED_ON);
        drv_led_establecer(4, LED_ON);
        //drv_sonido_tocar(TONE_LOSE, 400); // 400ms sonido
    }
    
    // Actualizar High Score en stats
    if (s_score > g_stats.high_score) {
        g_stats.high_score = s_score;
    }
}

// --- Funciones auxiliares lógicas ---
static void reiniciar_juego(void) {
    s_score = 0; s_compases_jugados = 0; s_nivel_dificultad = 1; s_duracion_compas_ms = 1000;
    for(int i=1; i<=LEDS_NUMBER; i++) drv_led_establecer(i, LED_OFF);
    
    // Resetear stats de partida
    g_stats.score_actual = 0;
    g_stats.tiempo_juego_s = 0;
    g_stats.notas_acertadas = 0;
    g_stats.notas_falladas = 0;
    g_stats.racha_actual = 0;
    g_stats.racha_maxima = 0;
    g_stats.nivel_dificultad = 1;
    g_stats.partidas_jugadas++;
}

static void avanzar_compas(void) {
    compas[0] = compas[1]; compas[1] = compas[2];
    uint32_t rnd = drv_aleatorios_rango(100);
    uint8_t p = 0;
    
    if (s_nivel_dificultad == 1)      p = (rnd > 50) ? 1 : 2;
    else if (s_nivel_dificultad == 2) p = (rnd < 20) ? 0 : ((rnd < 60) ? 1 : 2);
    else                              p = (rnd < 15) ? 0 : ((rnd < 45) ? 1 : ((rnd < 75) ? 2 : 3));
    compas[2] = p;
}

static void actualizar_leds_display(void) {
    drv_led_establecer(1, (compas[2] & 1) ? LED_ON : LED_OFF);
    drv_led_establecer(2, (compas[2] & 2) ? LED_ON : LED_OFF);
    drv_led_establecer(3, (compas[1] & 1) ? LED_ON : LED_OFF);
    drv_led_establecer(4, (compas[1] & 2) ? LED_ON : LED_OFF);
}

static void programar_siguiente_tick(void) {
    svc_alarma_activar(svc_alarma_codificar(false, s_duracion_compas_ms, 0), EV_JUEGO_TICK, 0);
}

static int calcular_puntuacion(Tiempo_us_t now) {
    uint32_t diff_ms = (uint32_t)((now - s_tiempo_inicio_compas) / 1000);
    uint32_t pct = (diff_ms * 100) / s_duracion_compas_ms;
    if (pct <= 10) return 2;
    if (pct <= 20) return 1;
    if (pct <= 40) return 0;
    return -1;
}

static void evaluar_jugada(uint8_t input_mask) {
    if (compas[0] == 0) { 
        s_score--; 
        g_stats.score_actual = s_score;
        g_stats.notas_falladas++;
        g_stats.racha_actual = 0;
        return; 
    }
    
    if (compas[0] & input_mask) {
        s_score += calcular_puntuacion(drv_tiempo_actual_us());
        compas[0] &= ~input_mask; // Consumir nota
        
        // Actualizar stats acierto
        g_stats.score_actual = s_score;
        g_stats.notas_acertadas++;
        g_stats.racha_actual++;
        if (g_stats.racha_actual > g_stats.racha_maxima) {
            g_stats.racha_maxima = g_stats.racha_actual;
        }
        
        // Feedback visual
        //if (input_mask & 1) drv_led_establecer(3, LED_OFF); 
        //if (input_mask & 2) drv_led_establecer(4, LED_OFF);
        
        // Feedback sonoro opcional (muy corto para no bloquear mucho)
         //drv_sonido_tocar(TONE_HIT, 50); 
    } else {
        s_score--;
        
        // Actualizar stats fallo
        g_stats.score_actual = s_score;
        g_stats.notas_falladas++;
        g_stats.racha_actual = 0;
    }
}

static void activar_alarma_reinicio(void) {
    if (!s_alarma_reinicio_activa) {
        // Activar alarma de 3 segundos (no periódica)
        svc_alarma_activar(svc_alarma_codificar(false, TIEMPO_REINICIO_MS, 0), EV_REINICIO_AUTO, 0);
        s_alarma_reinicio_activa = true;
    }
}

static void cancelar_alarma_reinicio(void) {
    if (s_alarma_reinicio_activa) {
        // Cancelar la alarma pasando 0 como flags
        svc_alarma_activar(0, EV_REINICIO_AUTO, 0);
        s_alarma_reinicio_activa = false;
    }
} */