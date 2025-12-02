/* *****************************************************************************
 * P.H.2025: Temporizadores en LPC2105, Timer 0 y Timer 1
 * implementacion para cumplir el hal_tiempo.h
 */
 
#include <LPC210x.H>
#include "hal_tiempo.h"
#include <stdlib.h>
#define PCLK_MHZ          15u
#define TICKS_PER_US      PCLK_MHZ
#define COUNTER_BITS      32u
#define COUNTER_MAX       (0xFFFFFFFFu)

/* ---- Tick libre con T1 --------------------------------------------------- */
static volatile uint32_t s_overflows_t1 = 0;  /* cuenta desbordes de T1 */
static hal_tiempo_info_t s_info;

void T1_ISR(void) __irq {
    /* Match0 al máximo para provocar overflow controlado */
    T1IR = 1;                  /* clear MR0 int */
    s_overflows_t1++;
    VICVectAddr = 0;
}

/* *****************************************************************************
 * Programa un contador de tick sobre Timer1, con maxima precisión y minimas interrupciones
 */
void hal_tiempo_iniciar_tick(hal_tiempo_info_t *out_info) {
    /* Timer1 como free-running: cuenta hasta MAX y se “resetea” por match */
    T1MR0 = COUNTER_MAX;
    T1MCR = 3;                 /* int + reset on MR0 */
    s_overflows_t1 = 0;

    VICVectAddr1 = (unsigned long)T1_ISR;
    VICVectCntl1 = 0x20 | 5;   /* IRQ slot, fuente 5 = Timer1 */
    VICIntEnable |= (1u << 5);

    T1TCR = 2;  /* reset */
    T1TCR = 1;  /* start */

    s_info.ticks_per_us   = TICKS_PER_US;
    s_info.counter_bits   = COUNTER_BITS;
    s_info.counter_max    = COUNTER_MAX;

    if (out_info) *out_info = s_info;
}

/* Lectura 64-bit sin carrera: lee TC, luego overflows, re-lee TC si hizo wrap */
uint64_t hal_tiempo_actual_tick64(void) {
    uint32_t hi1, lo1, hi2, lo2;

    lo1 = T1TC;
    hi1 = s_overflows_t1;
    lo2 = T1TC;
    if (lo2 < lo1) {
        /* ocurrió wrap entre lecturas: usar hi actualizado */
        hi2 = s_overflows_t1;
        return (((uint64_t)hi2) * ((uint64_t)(COUNTER_MAX + 1u))) + lo2;
    }
    return (((uint64_t)hi1) * ((uint64_t)(COUNTER_MAX + 1u))) + lo2;
}

/* ***************************************************************************** */

/* ---- Reloj periódico con T0 ---------------------------------------------- */
static void (*s_cb)() = 0;		//puntero a funcion a llamar cuando salte la RSI (en modo irq)

/* *****************************************************************************
 * Timer Interrupt Service Routine
 * llama a la funcion de callbak que se ejecutara en modo irq
 */

void T0_ISR(void) __irq {
    if (s_cb) s_cb();						// Llamo a la función desde la RSI.
    T0IR = 1;										// Clear interrupt flag
    VICVectAddr = 0;						// Acknowledge Interrupt
}

void hal_tiempo_periodico_config_tick(uint32_t periodo_en_tick) {
    /* Config sin arrancar */
    T0TCR = 0;
    if (periodo_en_tick == 0) return;
    T0MR0 = periodo_en_tick - 1u;
    T0MCR = 3; /* int + reset */
    VICVectAddr0 = (unsigned long)T0_ISR;
    VICVectCntl0 = 0x20 | 4;  /* fuente 4 = Timer0 */
}

void hal_tiempo_periodico_set_callback(void (*cb)()) {
    s_cb = cb;
}

void hal_tiempo_periodico_enable(bool enable) {
    if (enable) {
        VICIntEnable |= (1u << 4);
        T0TCR = 2; T0TCR = 1;
    } else {
        T0TCR = 0;
        VICIntEnClr = (1u << 4);
    }
}

void hal_tiempo_reloj_periodico_tick(uint32_t periodo_en_tick, void (*funcion_callback_drv)(void)){
    /* 1) Deshabilitar antes de tocar config/callback (evita ISR en mitad de cambios) */
    hal_tiempo_periodico_enable(false);

    /* 2) Registrar callback (puede ser NULL si vamos a deshabilitar) */
    hal_tiempo_periodico_set_callback(funcion_callback_drv);

    /* 3) Configurar periodo (no arranca el temporizador) */
    hal_tiempo_periodico_config_tick(periodo_en_tick);

    /* 4) Habilitar si hay periodo válido y callback no nulo; deshabilitar en caso contrario */
    if (periodo_en_tick != 0u && funcion_callback_drv != NULL) {
        hal_tiempo_periodico_enable(true);
    } else {
        hal_tiempo_periodico_enable(false);
    }
}
