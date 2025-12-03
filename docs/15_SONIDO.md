# 🔊 Funcionalidad: Sistema de Sonido

## Introducción

El driver de sonido (`drv_sonido`) proporciona feedback auditivo mediante:
- **LPC**: Salida PWM simulada (no implementado completamente)
- **NRF**: Buzzer vía PWM en P0.3

## API

```c
void drv_sonido_iniciar(void);
void drv_sonido_reproducir(uint16_t frecuencia_hz, uint16_t duracion_ms);
void drv_sonido_silencio(void);
```

## Uso Potencial

```c
// Feedback de pulsación correcta
drv_sonido_reproducir(440, 100);  // La (440 Hz), 100ms

// Feedback de error
drv_sonido_reproducir(220, 200);  // La grave (220 Hz), 200ms
```

**Nota**: En la implementación actual, el sonido está **inicializado pero no utilizado** activamente en `beat_hero.c`.

## Configuración Hardware

### LPC2105
- **Implementación**: Pendiente (requiere PWM o DAC)

### NRF52840
- **Pin**: P0.3
- **Periférico**: PWM0
- **Generación de Tono**: PWM con frecuencia variable

## Implementación NRF (Ejemplo)

```c
void hal_sonido_reproducir_nrf(uint16_t freq_hz) {
    uint32_t periodo = 16000000 / freq_hz;  // Asumiendo CLK 16 MHz
    NRF_PWM0->COUNTERTOP = periodo;
    NRF_PWM0->TASKS_SEQSTART[0] = 1;
}
```

---

[← Anterior: Monitor](14_MONITOR.md) | [Volver al índice](00_INDICE.md)
