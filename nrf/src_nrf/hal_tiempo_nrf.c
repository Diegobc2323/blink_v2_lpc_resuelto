/* *****************************************************************************
 * P.H.2025: TODO
 * implementacion para cumplir el hal_tiempo.h
 */
 #include <nrf.h>
#include "hal_tiempo.h"
#include <stdlib.h>


#define PCLK_MHZ          16u
#define TICKS_PER_US      PCLK_MHZ
#define COUNTER_BITS      32u
#define COUNTER_MAX       (0xFFFFFFFFu)


/* ---- Tick libre con T1 --------------------------------------------------- */
static volatile uint32_t s_overflows_t1 = 0;  /* cuenta desbordes de T1 */

void TIMER1_IRQHandler(void) __irq {
		if( NRF_TIMER1->EVENTS_COMPARE[0]){
			NRF_TIMER1->EVENTS_COMPARE[0] = 0; //limpio el flag que ha causado la interrupción
			s_overflows_t1++;
		}
}

/* *****************************************************************************
 * Programa un contador de tick sobre Timer1, con maxima precisión y minimas interrupciones
 */
void hal_tiempo_iniciar_tick(hal_tiempo_info_t *out_info) {

		NRF_TIMER1->TASKS_STOP = 1;	//Detengo el timer para que deje de contar mientas configuro
		NRF_TIMER1->TASKS_CLEAR = 1;	//Reinicio el contador
	
		NRF_TIMER1->MODE = TIMER_MODE_MODE_Timer;	// Modo timer
		NRF_TIMER1->BITMODE = TIMER_BITMODE_BITMODE_32Bit;	// ancho del registro = 32b
		NRF_TIMER1->PRESCALER = 0;                        // 64 MHz
	
		NRF_TIMER1->CC[0] = COUNTER_MAX;	// Configuro el evento de comparación para que salte la interrupción cuando el counter llegue a COUNTER_MAX
		NRF_TIMER1->INTENSET = TIMER_INTENSET_COMPARE0_Msk;	//habilito la interrupción cuando suceda el evento
	
		NVIC_EnableIRQ(TIMER1_IRQn); // habilito las interrupciones del timer
		
		NRF_TIMER1->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Msk;  // Reseteo el valor de los tick automático al llegar a COUNTER_MAX
		s_overflows_t1 = 0;//inicializo el contador de overflows a cero
	
		NRF_TIMER1->TASKS_START = 1; //comienzo a contar con el timer
	
		out_info->ticks_per_us   = TICKS_PER_US;
    out_info->counter_bits   = COUNTER_BITS;
    out_info->counter_max    = COUNTER_MAX;

}

/* Lectura 64-bit sin carrera: lee TC, luego overflows, re-lee TC si hizo wrap */
uint64_t hal_tiempo_actual_tick64(void) {
    uint32_t hi1, lo1, hi2, lo2;

		NRF_TIMER1->TASKS_CAPTURE[1] = 1;
    lo1 = NRF_TIMER1->CC[1];
    hi1 = s_overflows_t1;
		NRF_TIMER1->TASKS_CAPTURE[1] = 1;
    lo2 = NRF_TIMER1->CC[1];
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

void TIMER0_IRQHandler(void) __irq {
    if (NRF_TIMER0->EVENTS_COMPARE[0]) {
			NRF_TIMER0->EVENTS_COMPARE[0] = 0;	// Clear interrupt flag
			if (s_cb) s_cb();                  // llamar callback
			
		}
}

void hal_tiempo_periodico_config_tick(uint32_t periodo_en_tick) {
    /* Config sin arrancar */
    NRF_TIMER0->TASKS_STOP = 1;
    NRF_TIMER0->TASKS_CLEAR = 1;

    if (periodo_en_tick == 0) return;

    NRF_TIMER0->MODE = TIMER_MODE_MODE_Timer;
    NRF_TIMER0->BITMODE = TIMER_BITMODE_BITMODE_32Bit;
    NRF_TIMER0->PRESCALER = 0; // 64 MHz

    NRF_TIMER0->CC[0] = periodo_en_tick - 1u;
    NRF_TIMER0->INTENSET = TIMER_INTENSET_COMPARE0_Msk;

    NRF_TIMER0->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Msk; // Reset automático al comparar
}

void hal_tiempo_periodico_set_callback(void (*cb)()) {
    s_cb = cb;
}

void hal_tiempo_periodico_enable(bool enable) {
    if (enable) {
        NVIC_EnableIRQ(TIMER0_IRQn);
        NRF_TIMER0->TASKS_START = 1;

    } else {
        NRF_TIMER0->TASKS_STOP = 1;
        NVIC_DisableIRQ(TIMER0_IRQn);
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
