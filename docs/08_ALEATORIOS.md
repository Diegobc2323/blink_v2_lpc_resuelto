# 🎲 Funcionalidad: Generación de Números Aleatorios

## Introducción

El sistema de números aleatorios proporciona RNG para generar patrones del juego. Implementaciones diferentes por plataforma:
- **LPC**: Basado en timestamp (pseudo-aleatorio)
- **NRF**: Hardware RNG (verdaderamente aleatorio)

## API

```c
void drv_aleatorios_iniciar(uint32_t semilla);
uint32_t drv_aleatorios_rango(uint32_t max);
```

## Uso en el Proyecto

### Generación de Patrones (beat_hero.c)
```c
uint32_t rnd = drv_aleatorios_rango(100);  // 0-99

if (s_nivel_dificultad == 1) {
    compas[2] = (rnd > 50) ? 1 : 2;  // 50% cada uno
} else if (s_nivel_dificultad == 2) {
    compas[2] = (rnd < 20) ? 0 : ((rnd < 60) ? 1 : 2);
}
```

## Implementaciones HAL

### LPC2105 (hal_aleatorios_lpc.c)
**Algoritmo**: Linear Congruential Generator (LCG)
```c
// Xn+1 = (a * Xn + c) mod m
s_seed = (1103515245 * s_seed + 12345) & 0x7FFFFFFF;
```

**Semilla**: Timestamp de `drv_tiempo_actual_us()` si no se especifica

### NRF52840 (hal_aleatorios_nrf.c)
**Periférico**: `NRF_RNG` (Random Number Generator)

```c
NRF_RNG->TASKS_START = 1;
while (!NRF_RNG->EVENTS_VALRDY);  // Esperar valor
uint8_t random = NRF_RNG->VALUE;
```

**Ventaja**: Verdaderamente aleatorio (ruido térmico)

## Calidad de Aleatoriedad

| Plataforma | Tipo | Calidad | Velocidad |
|------------|------|---------|-----------|
| LPC | PRNG (LCG) | Media | Rápida |
| NRF | TRNG (Hardware) | Alta | Moderada |

Para el juego Beat Hero ambos son suficientes.

---

[← Anterior: Interrupciones](07_INTERRUPCIONES.md) | [Volver al índice](00_INDICE.md) | [Siguiente: LEDs →](09_LEDS.md)
