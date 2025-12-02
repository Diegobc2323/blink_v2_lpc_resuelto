#include "beat_hero.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h> // Necesario para memset

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
// IMPORTANTE: ID 999 puede causar desbordamiento en algunos RTOS (uint8_t), 
// cambiamos a 50 para asegurar estabilidad en el reinicio.
#define ID_ALARMA_RESET         50  
#define ID_ALARMA_TICK          100
#define ID_ALARMA_DEMO          200

// --- ESTRUCTURA DE ESTADÍSTICAS (Para Watch1 en Keil) ---
typedef struct {
    int32_t  Score;                   // Puntuación actual (sincronizada)
    int32_t  HighScore;               // Récord de sesión
    uint8_t  Nivel;                   // Nivel actual (1-4)
    uint32_t CompasActual;            // Progreso de la canción
    
    // Métricas de Rendimiento
    uint32_t NotasAcertadas;
    uint32_t NotasFalladas;           // Fallos de botón o notas pasadas
    uint32_t ComboActual;             // Racha actual
    uint32_t MaxCombo;                // Mejor racha de la partida
    
    // Métricas de Tiempo (Latencia humana)
    int32_t  UltimoTiempoReaccion_ms; // Tiempo desde inicio compás hasta pulsar
    int32_t  PromedioReaccion_ms;     // Promedio de tiempo de reacción
    uint64_t SumaTiemposReaccion;     // Auxiliar para cálculo de promedio
} BeatHeroStats_t;

// Variable global volatile para que el debugger la lea siempre
volatile BeatHeroStats_t juego_stats = {0};

// Variables de Estado Originales
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
    rt_GE_suscribir(ev_JUEGO_TIMEOUT, 1, beat_hero_actualizar); 
    
    reiniciar_variables_juego(); 
    // Iniciar parpadeo attract mode (ID_ALARMA_DEMO)
    svc_alarma_activar(svc_alarma_codificar(false, 500, ID_ALARMA_DEMO), ev_JUEGO_NUEVO_LED, ID_ALARMA_DEMO);
}

void beat_hero_actualizar(EVENTO_T evento, uint32_t auxData) {
    
    if (s_estado == e_JUEGO && (evento == ev_PULSAR_BOTON || evento == ev_SOLTAR_BOTON)) {
        svc_alarma_activar(svc_alarma_codificar(false, 10000, 0), ev_INACTIVIDAD, 0);
    }

    switch (s_estado) {
        
        case e_INIT:
            if (evento == ev_JUEGO_NUEVO_LED && auxData == ID_ALARMA_DEMO) {
                static uint8_t led_demo = 1;
                drv_led_establecer(led_demo, LED_OFF);
                led_demo = (led_demo % LEDS_NUMBER) + 1;
                drv_led_establecer(led_demo, LED_ON);
                svc_alarma_activar(svc_alarma_codificar(false, 500, ID_ALARMA_DEMO), ev_JUEGO_NUEVO_LED, ID_ALARMA_DEMO);
            }
            
            if (evento == ev_PULSAR_BOTON && (auxData == 2 || auxData == 3)) {
                return; 
            }

            if (evento == ev_PULSAR_BOTON && (auxData == 0 || auxData == 1)) {
                svc_alarma_activar(0, ev_JUEGO_NUEVO_LED, ID_ALARMA_DEMO);
                for(int i=1; i<=LEDS_NUMBER; i++) drv_led_establecer(i, LED_OFF);
                
                s_estado = e_JUEGO;
                reiniciar_variables_juego();
                s_duracion_compas_ms = MS_POR_MINUTO / BPM_INICIAL;
                
                programar_siguiente_tick();
            }
            break;

        case e_JUEGO:
            if (evento == ev_PULSAR_BOTON && (auxData == 2 || auxData == 3)) {
                finalizar_partida(false); 
                return;
            }

            if (evento == ev_JUEGO_NUEVO_LED && auxData == ID_ALARMA_TICK) {
                if (s_compases_jugados >= MAX_COMPASES_PARTIDA) {
                    finalizar_partida(true); 
                    return;
                }
                if (s_score < SCORE_MIN_FAIL) {
                    finalizar_partida(false); 
                    return;
                }

                avanzar_compas();
                actualizar_leds_display();
                
                // Dificultad
                if (s_compases_jugados % 5 == 0 && s_compases_jugados > 0) {
                    s_nivel_dificultad++;
                    if(s_nivel_dificultad > 4) s_nivel_dificultad = 4;
                    if (s_nivel_dificultad == 4) s_duracion_compas_ms = (uint32_t)(s_duracion_compas_ms * 0.9f);
                    
                    // STATS: Actualizar nivel
                    juego_stats.Nivel = s_nivel_dificultad;
                }

                s_tiempo_inicio_compas = drv_tiempo_actual_us();
                s_compases_jugados++; 
                
                // STATS: Actualizar compás
                juego_stats.CompasActual = s_compases_jugados;

                programar_siguiente_tick();
            }
            else if (evento == ev_PULSAR_BOTON && auxData <= 1) {
                evaluar_jugada(1 << auxData);
                
                if (s_score < SCORE_MIN_FAIL) {
                    finalizar_partida(false);
                }
            }
            break;

        case e_RESULTADO:
            if (evento == ev_PULSAR_BOTON && (auxData == 2 || auxData == 3)) {
                // Cancelamos cualquier alarma previa de reset para evitar duplicados
                svc_alarma_activar(0, ev_JUEGO_TIMEOUT, ID_ALARMA_RESET);
                
                uint32_t flags = svc_alarma_codificar(false, TIEMPO_REINICIO_MS, ID_ALARMA_RESET);
                svc_alarma_activar(flags, ev_JUEGO_TIMEOUT, ID_ALARMA_RESET);
            }
            
            if (evento == ev_SOLTAR_BOTON && (auxData == 2 || auxData == 3)) {
                // Si suelta el botón antes de tiempo, cancelamos el reset
                svc_alarma_activar(0, ev_JUEGO_TIMEOUT, ID_ALARMA_RESET);
            }
            
            if (evento == ev_JUEGO_TIMEOUT && auxData == ID_ALARMA_RESET) {
                // Se cumplieron los 3 segundos
                reiniciar_variables_juego();
                s_estado = e_INIT;
                svc_alarma_activar(svc_alarma_codificar(false, 500, ID_ALARMA_DEMO), ev_JUEGO_NUEVO_LED, ID_ALARMA_DEMO);
            }
            break;
            
        default: break;
    }
}

static void finalizar_partida(bool exito) {
    // Aseguramos cancelar el Tick del juego para que no interfiera en e_RESULTADO
    svc_alarma_activar(0, ev_JUEGO_NUEVO_LED, ID_ALARMA_TICK);
    
    s_estado = e_RESULTADO;
    
    for(int i=1; i<=LEDS_NUMBER; i++) drv_led_establecer(i, LED_OFF);
    
    if (exito) {
        for(int i=1; i<=LEDS_NUMBER; i++) drv_led_establecer(i, LED_ON);
        if (s_score > s_high_score) {
            s_high_score = s_score;
            juego_stats.HighScore = s_high_score;
        }
    } else {
        juego_stats.ComboActual = 0;
    }

    compas[0] = compas[1]; compas[1] = compas[2];
    uint32_t rnd = drv_aleatorios_rango(100);
    uint8_t p = 0;
    
    if (s_nivel_dificultad == 1)       p = (rnd > 50) ? 1 : 2;
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
    svc_alarma_activar(svc_alarma_codificar(false, s_duracion_compas_ms, ID_ALARMA_TICK), ev_JUEGO_NUEVO_LED, ID_ALARMA_TICK);
}

static int calcular_puntuacion(Tiempo_us_t now) {
    uint32_t diff_ms = (uint32_t)((now - s_tiempo_inicio_compas) / 1000);
    
    // STATS: Guardar latencia actual
    juego_stats.UltimoTiempoReaccion_ms = diff_ms;
    
    if(s_duracion_compas_ms == 0) return 0;
    uint32_t pct = (diff_ms * 100) / s_duracion_compas_ms;
    if (pct <= 10) return 2;
    if (pct <= 20) return 1;
    if (pct <= 40) return 0;
    return -1;
}

static void evaluar_jugada(uint8_t input_mask) {
    if (compas[0] == 0) { 
        // Error: Pulsar sin nota
        s_score--; 
        
        juego_stats.Score = s_score;
        juego_stats.NotasFalladas++;
        juego_stats.ComboActual = 0;
        return; 
    }
    
    if (compas[0] & input_mask) {
        // Acierto
        s_score += calcular_puntuacion(drv_tiempo_actual_us());
        compas[0] &= ~input_mask; 
        
        juego_stats.Score = s_score;
        juego_stats.NotasAcertadas++;
        juego_stats.ComboActual++;
        if (juego_stats.ComboActual > juego_stats.MaxCombo) {
            juego_stats.MaxCombo = juego_stats.ComboActual;
        }
        
        // STATS: Promedio acumulativo
        juego_stats.SumaTiemposReaccion += juego_stats.UltimoTiempoReaccion_ms;
        if(juego_stats.NotasAcertadas > 0) {
            juego_stats.PromedioReaccion_ms = (int32_t)(juego_stats.SumaTiemposReaccion / juego_stats.NotasAcertadas);
        }

    } else {
        // Error: Botón incorrecto
        s_score--;
        
        juego_stats.Score = s_score;
        juego_stats.NotasFalladas++;
        juego_stats.ComboActual = 0;
    }
}