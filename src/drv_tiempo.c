/* ***************************
 * P.H.2025: Driver/Manejador de los temporizadores
 * Implementación corregida para evitar corrupción de pila en callbacks
 */

#include "drv_tiempo.h"
#include "hal_tiempo.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

/* Estado interno */
static hal_tiempo_info_t s_hal_info;
static bool s_iniciado = false;
static uint32_t s_parametro = 0;
static void(*s_funcion)(uint32_t, uint32_t) = NULL; // Corregido tipo

/**
 * inicializa el reloj y empieza a contar
 */
bool drv_tiempo_iniciar(void) {
    hal_tiempo_iniciar_tick(&s_hal_info);
    s_iniciado = true;
    return true;
}

/**
 * tiempo desde que se inicio el temporizador en microsegundos
 */
Tiempo_us_t drv_tiempo_actual_us(void) {
    uint64_t ticks;
    uint64_t us;

    if (!s_iniciado) return (Tiempo_us_t)0;

    ticks = hal_tiempo_actual_tick64();

    if (s_hal_info.ticks_per_us == 0) return (Tiempo_us_t)0;

    us = ticks / (uint64_t)s_hal_info.ticks_per_us;
    return (Tiempo_us_t)us;
}

/**
 * tiempo desde que se inicio el temporizador en milisegundos
 */
Tiempo_ms_t drv_tiempo_actual_ms(void) {
    Tiempo_us_t us = drv_tiempo_actual_us();
    return (Tiempo_ms_t)(us / 1000u);
}

/**
 * retardo: esperar un cierto tiempo en milisegundos
 */
void drv_tiempo_esperar_ms(Tiempo_ms_t ms) {
    Tiempo_ms_t inicio, ahora;
    inicio = drv_tiempo_actual_ms();
    Tiempo_ms_t deadline = inicio + ms;

    do {
        ahora = drv_tiempo_actual_ms();
    } while ((Tiempo_ms_t)(ahora - deadline) > (Tiempo_ms_t)0 && ahora != deadline);
}

/**
 * esperar hasta un determinado tiempo (en ms)
 */
Tiempo_ms_t drv_tiempo_esperar_hasta_ms(Tiempo_ms_t deadline_ms) {
    Tiempo_ms_t ahora = drv_tiempo_actual_ms();

    if ((Tiempo_ms_t)(ahora - deadline_ms) <= (Tiempo_ms_t)0) {
       // Ya paso
    }

    while (1) {
        ahora = drv_tiempo_actual_ms();
        if (ahora >= deadline_ms) break;
    }
    return ahora;
}

// --- CORRECCIÓN IMPORTANTE AQUÍ ---
// Definimos el tipo de puntero compatible con rt_FIFO_encolar (2 argumentos)
typedef void (*f_callback_fifo_t)(uint32_t, uint32_t);

void drv_funcion_callback_app(){
    // Casteamos para asegurar que se pasan los argumentos correctos
    if (s_funcion != NULL) {
        // Pasamos el ID del evento (s_parametro) y 0 como dato auxiliar
        // Esto evita que rt_FIFO lea basura de la memoria
        ((f_callback_fifo_t)s_funcion)(s_parametro, 0);
    }
}

void drv_tiempo_periodico_ms(Tiempo_ms_t ms, void(*funcion_callback_app)(), uint32_t ID_evento) {
    if (!s_iniciado || funcion_callback_app == NULL) return;

    // Convertir ms -> ticks
    uint32_t periodo_en_tick = (uint32_t)ms * (s_hal_info.ticks_per_us * 1000u);
    s_parametro = ID_evento;
    // Guardamos la función casteada
    s_funcion = (void(*)(uint32_t, uint32_t))funcion_callback_app;

    // Registrar el callback de aplicación directamente en el HAL
    hal_tiempo_reloj_periodico_tick(periodo_en_tick, drv_funcion_callback_app);
}
