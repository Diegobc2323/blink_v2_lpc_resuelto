#include "beat_hero.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h> // memset

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
#define SCORE_MIN_FAIL          -5  // Si llegas a -5 puntos, game over
#define BPM_INICIAL             60  // Empieza a 60 BPM (1 beat/segundo)
#define MS_POR_MINUTO           60000
#define TIEMPO_REINICIO_MS      3000  // Mantener pulsado 3 segundos

// IDs para identificar alarmas diferentes (auxData)
#define ID_ALARMA_RESET         50  
#define ID_ALARMA_TICK          100
#define ID_ALARMA_DEMO          200

typedef struct {
    int32_t  Score;
    int32_t  HighScore;
    uint8_t  Nivel;
    uint32_t CompasActual;
    
    uint32_t NotasAcertadas;
    uint32_t NotasFalladas;           
    uint32_t ComboActual;             
    uint32_t MaxCombo;                
    
    uint32_t AciertosPerfectos;       // 2 puntos
    uint32_t AciertosBuenos;          // 1 punto
    uint32_t AciertosNormales;        // 0 puntos (pero mantiene combo)
    
    int32_t  UltimoTiempoReaccion_ms; 
    int32_t  PromedioReaccion_ms;     
    uint64_t SumaTiemposReaccion;     
} BeatHeroStats_t;

volatile BeatHeroStats_t juego_stats = {0};

// Ventana de 3 compases: [0]=actual (lo que hay que pulsar), [1]=siguiente, [2]=futuro
static uint8_t compas[3]; 

typedef enum { e_INIT, e_JUEGO, e_RESULTADO } GameState_t;
static GameState_t s_estado = e_INIT;

static int32_t  s_score = 0;
static int32_t  s_high_score = 0; 
static uint32_t s_compases_jugados = 0;
static uint32_t s_duracion_compas_ms = 1000;
static Tiempo_us_t s_tiempo_inicio_compas = 0;  // Para calcular tiempo de reacción
static uint8_t  s_nivel_dificultad = 1;


static void reiniciar_variables_juego(void) {
    // Preservar el high score antes de resetear
    int32_t saved_high = juego_stats.HighScore;
    
    memset((void*)&juego_stats, 0, sizeof(BeatHeroStats_t));
    
    juego_stats.HighScore = saved_high;
    juego_stats.Nivel = 1;
    
    s_score = 0; 
    s_compases_jugados = 0; 
    s_nivel_dificultad = 1; 
    s_duracion_compas_ms = 1000;
    compas[0]=0; compas[1]=0; compas[2]=0;
    
    for(int i=1; i<=LEDS_NUMBER; i++) drv_led_establecer(i, LED_OFF);
    svc_alarma_activar(0, ev_JUEGO_TIMEOUT, ID_ALARMA_RESET);
}

static void finalizar_partida(bool exito) {
    s_estado = e_RESULTADO;
    svc_alarma_activar(0, ev_JUEGO_NUEVO_LED, ID_ALARMA_TICK);
    
    for(int i=1; i<=LEDS_NUMBER; i++) drv_led_establecer(i, LED_OFF);
    
    if (exito) {
        // Victoria: todos los LEDs encendidos
        for(int i=1; i<=LEDS_NUMBER; i++) drv_led_establecer(i, LED_ON);
        if (s_score > s_high_score) {
            s_high_score = s_score;
            juego_stats.HighScore = s_high_score; 
        }
    } else {
        // Derrota: LEDs 1 y 4 (patron de cruz)
        drv_led_establecer(1, LED_ON);
        drv_led_establecer(4, LED_ON);
    }
}

static void avanzar_compas(void) {
    // Si queda algo en compas[0] sin pulsar, es un fallo
    if (compas[0] != 0) {
              s_score--;
        juego_stats.Score = s_score;
        juego_stats.NotasFalladas++;
        juego_stats.ComboActual = 0;
    }

    // Avanzar ventana: [0]<-[1]<-[2], y generar nuevo [2]
    compas[0] = compas[1]; 
    compas[1] = compas[2];
    
    uint32_t rnd = drv_aleatorios_rango(100);
    uint8_t p = 0;

    // Probabilidades segun nivel:
    // p=0 (nada), p=1 (boton 0), p=2 (boton 1), p=3 (ambos)
    if (s_nivel_dificultad == 1)       
        p = (rnd > 50) ? 1 : 2;  // 50% boton 0, 50% boton 1
    else if (s_nivel_dificultad == 2) 
        p = (rnd < 20) ? 0 : ((rnd < 60) ? 1 : 2);  // 20% nada, 40% cada boton
    else                              
        p = (rnd < 15) ? 0 : ((rnd < 45) ? 1 : ((rnd < 75) ? 2 : 3)); // 15% nada, 30% cada simple, 25% ambos
    
    compas[2] = p;
}

static void actualizar_leds_display(void) {
    // Mostrar compas[2] en LEDs 1-2, compas[1] en LEDs 3-4
    // Usamos operaciones bit a bit: bit 0 = boton 0, bit 1 = boton 1
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
    juego_stats.UltimoTiempoReaccion_ms = diff_ms;
    
    if(s_duracion_compas_ms == 0) return 0;
    // Calcular % del compas que ha pasado
    uint32_t pct = (diff_ms * 100) / s_duracion_compas_ms;
    
    // Sistema de puntuacion por timing:
    if (pct <= 10) { 
        juego_stats.AciertosPerfectos++; 
        return 2;  // Perfecto: primeros 10%
    }
    if (pct <= 20) { 
        juego_stats.AciertosBuenos++;    
        return 1;  // Bueno: 10-20%
    }
    if (pct <= 40) {
        juego_stats.AciertosNormales++;  
        return 0;  // Normal: 20-40% (no suma puntos pero no rompe combo)
    }
    // Mas del 40%: muy tarde, penalizar
    return -1;
}

static void evaluar_jugada(uint8_t input_mask) {
    // Si no hay nada que pulsar, es un error
    if (compas[0] == 0) { 
        s_score--; 
        juego_stats.Score = s_score;
        juego_stats.NotasFalladas++;
        juego_stats.ComboActual = 0;
        return; 
    }
    
    // Comprobar si la mascara de entrada coincide con lo esperado
    // input_mask: 0b01 para boton 0, 0b10 para boton 1
    if (compas[0] & input_mask) {
        int puntos = calcular_puntuacion(drv_tiempo_actual_us());
        s_score += puntos;
        
        // Limpiar el bit que acaban de pulsar
        compas[0] &= ~input_mask; 
        
        juego_stats.Score = s_score;

        // Si la puntuacion no es negativa, contar como acierto
        if (puntos >= 0) {
            juego_stats.NotasAcertadas++;
            juego_stats.ComboActual++;
            if (juego_stats.ComboActual > juego_stats.MaxCombo) {
                juego_stats.MaxCombo = juego_stats.ComboActual;
            }
            
            // Actualizar promedio de tiempo de reaccion
            juego_stats.SumaTiemposReaccion += juego_stats.UltimoTiempoReaccion_ms;
            if(juego_stats.NotasAcertadas > 0) {
                juego_stats.PromedioReaccion_ms = (int32_t)(juego_stats.SumaTiemposReaccion / juego_stats.NotasAcertadas);
            }
        } else {
            // Pulsaron el boton correcto pero demasiado tarde (>40%)
            juego_stats.NotasFalladas++;
            juego_stats.ComboActual = 0;
        }

    } else {
        // Pulsaron un boton que no tocaba
        s_score--;
        juego_stats.Score = s_score;
        juego_stats.NotasFalladas++;
        juego_stats.ComboActual = 0;
    }
}


void beat_hero_actualizar(EVENTO_T evento, uint32_t auxData) {
    
    // Resetear alarma de inactividad si hay pulsaciones durante juego
    if (s_estado == e_JUEGO && (evento == ev_PULSAR_BOTON || evento == ev_SOLTAR_BOTON)) {
        svc_alarma_activar(svc_alarma_codificar(false, 10000, 0), ev_INACTIVIDAD, 0);
    }

    switch (s_estado) {
        
        case e_INIT:
            // Animacion LED rotativo en demo
            if (evento == ev_JUEGO_NUEVO_LED && auxData == ID_ALARMA_DEMO) {
                static uint8_t led_demo = 1;
                drv_led_establecer(led_demo, LED_OFF);
                led_demo = (led_demo % LEDS_NUMBER) + 1;
                drv_led_establecer(led_demo, LED_ON);
                svc_alarma_activar(svc_alarma_codificar(false, 500, ID_ALARMA_DEMO), ev_JUEGO_NUEVO_LED, ID_ALARMA_DEMO);
            }
            
            // Botones 2 y 3 no hacen nada en el menu inicial
            if (evento == ev_PULSAR_BOTON && (auxData == 2 || auxData == 3)) {
                return; 
            }

            // Botones 0 o 1 empiezan la partida
            if (evento == ev_PULSAR_BOTON && (auxData == 0 || auxData == 1)) {
                svc_alarma_activar(0, ev_JUEGO_NUEVO_LED, ID_ALARMA_DEMO); // Parar demo
                for(int i=1; i<=LEDS_NUMBER; i++) drv_led_establecer(i, LED_OFF);
                
                s_estado = e_JUEGO;
                reiniciar_variables_juego();
                s_duracion_compas_ms = MS_POR_MINUTO / BPM_INICIAL;
                programar_siguiente_tick();
            }
            break;

        case e_JUEGO:
            // Botones 2 o 3 permiten abortar partida
            if (evento == ev_PULSAR_BOTON && (auxData == 2 || auxData == 3)) {
                finalizar_partida(false); 
                return;
            }

            // Tick del juego cada compas
            if (evento == ev_JUEGO_NUEVO_LED && auxData == ID_ALARMA_TICK) {
                // Victoria si completamos todos los compases
                if (s_compases_jugados >= MAX_COMPASES_PARTIDA) {
                    finalizar_partida(true); 
                    return;
                }
                // Derrota si la puntuacion es muy mala
                if (s_score < SCORE_MIN_FAIL) {
                    finalizar_partida(false); 
                    return;
                }

                avanzar_compas();
                actualizar_leds_display();
                
                // Cada 5 compases subir nivel
                if (s_compases_jugados % 5 == 0 && s_compases_jugados > 0) {
                    s_nivel_dificultad++;
                    if(s_nivel_dificultad > 4) s_nivel_dificultad = 4;
                    // En nivel 4 aumentamos velocidad
                    if (s_nivel_dificultad == 4) s_duracion_compas_ms = (uint32_t)(s_duracion_compas_ms * 0.9f);
                    juego_stats.Nivel = s_nivel_dificultad;
                }

                // Timestamp para calcular tiempo de reaccion
                s_tiempo_inicio_compas = drv_tiempo_actual_us();
                s_compases_jugados++; 
                juego_stats.CompasActual = s_compases_jugados;

                programar_siguiente_tick();
            }
            // Solo botones 0 y 1 juegan
            else if (evento == ev_PULSAR_BOTON && auxData <= 1) {
                // Convertir ID boton a mascara de bits (0->0b01, 1->0b10)
                evaluar_jugada(1 << auxData);
                if (s_score < SCORE_MIN_FAIL) {
                    finalizar_partida(false);
                }
            }
            break;

        case e_RESULTADO:
            // Mantener pulsado boton 2 o 3 para reiniciar
            if (evento == ev_PULSAR_BOTON && (auxData == 2 || auxData == 3)) {
                svc_alarma_activar(0, ev_JUEGO_TIMEOUT, ID_ALARMA_RESET);
                uint32_t flags = svc_alarma_codificar(false, TIEMPO_REINICIO_MS, ID_ALARMA_RESET);
                svc_alarma_activar(flags, ev_JUEGO_TIMEOUT, ID_ALARMA_RESET);
            }
            
            // Si sueltan antes de 3s, cancelar reinicio
            if (evento == ev_SOLTAR_BOTON && (auxData == 2 || auxData == 3)) {
                svc_alarma_activar(0, ev_JUEGO_TIMEOUT, ID_ALARMA_RESET);
            }
            
            // Timeout alcanzado, volver al menu
            if (evento == ev_JUEGO_TIMEOUT && auxData == ID_ALARMA_RESET) {
                reiniciar_variables_juego();
                s_estado = e_INIT;
                svc_alarma_activar(svc_alarma_codificar(false, 500, ID_ALARMA_DEMO), ev_JUEGO_NUEVO_LED, ID_ALARMA_DEMO);
            }
            break;
            
        default: break;
    }
}

void beat_hero_iniciar(void) {
    s_estado = e_INIT;
    s_high_score = 0; 

    rt_GE_suscribir(ev_JUEGO_NUEVO_LED, 1, beat_hero_actualizar);
    rt_GE_suscribir(ev_PULSAR_BOTON, 1, beat_hero_actualizar);
    rt_GE_suscribir(ev_SOLTAR_BOTON, 1, beat_hero_actualizar);
    rt_GE_suscribir(ev_JUEGO_TIMEOUT, 1, beat_hero_actualizar); 
    
    reiniciar_variables_juego(); 
    // Animacion de demo: ir encendiendo LEDs de uno en uno
    svc_alarma_activar(svc_alarma_codificar(false, 500, ID_ALARMA_DEMO), ev_JUEGO_NUEVO_LED, ID_ALARMA_DEMO);
}