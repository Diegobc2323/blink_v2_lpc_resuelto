#include "drv_aleatorios.h"
#include <stdlib.h> // Necesario para rand() y srand()

/**
 * @brief Implementación para LPC2105 (Simulación por Software)
 * El LPC2105 no tiene hardware de aleatorios, así que usamos un PRNG (Pseudo-RNG)
 * estándar de C.
 */

void hal_aleatorios_iniciar(uint32_t semilla) {
    // EN LPC: La semilla SÍ es importante. 
    // Como es por software, si siempre le pasas el mismo número (ej: 0),
    // la secuencia de "aleatorios" será idéntica cada vez que reinicies.
    // Lo ideal es pasarle un valor que varíe, como una lectura al aire del ADC 
    // o el valor de un temporizador en el momento que el usuario pulsa un botón.
    srand(semilla); 
}

uint32_t hal_aleatorios_obtener_32bits(void) {
    // En muchos compiladores ARM (Keil), rand() devuelve solo 15 bits (0..32767).
    // Para generar un número de 32 bits real y robusto, concatenamos varios resultados.
    
    uint32_t b1 = rand() & 0xFF;
    uint32_t b2 = rand() & 0xFF;
    uint32_t b3 = rand() & 0xFF;
    uint32_t b4 = rand() & 0xFF;

    // Construimos el de 32 bits byte a byte, similar a como lo hacías en nRF
    uint32_t resultado = (b4 << 24) | (b3 << 16) | (b2 << 8) | b1;

    return resultado;
}

uint32_t hal_aleatorios_rango(uint32_t rango) {
    if (rango == 0) return 0;

    // Reutilizamos la función de 32 bits que acabamos de definir
    uint32_t aleatorio = drv_aleatorios_obtener_32bits();
    
    return (aleatorio % rango);
}