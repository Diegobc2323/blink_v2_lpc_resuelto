# 💡 Funcionalidad: Manejo de LEDs

## Introducción

El driver de LEDs (`drv_leds`) proporciona control de alto nivel para los 4 LEDs del sistema. Abstrae:
- Configuración de pines
- Estado activo (HIGH/LOW según plataforma)
- Operaciones individuales y grupales

## API

```c
void drv_leds_iniciar(void);
void drv_led_establecer(uint8_t numero_led, led_Estado_t estado);
void drv_leds_establecer_mascara(uint8_t mascara);
```

## Uso en el Proyecto

### Control Individual (beat_hero.c)
```c
drv_led_establecer(1, LED_ON);   // Encender LED 1
drv_led_establecer(2, LED_OFF);  // Apagar LED 2
```

### Visualización de Compás
```c
void actualizar_leds_display(void) {
    drv_led_establecer(1, (compas[2] & 1) ? LED_ON : LED_OFF);
    drv_led_establecer(2, (compas[2] & 2) ? LED_ON : LED_OFF);
    drv_led_establecer(3, (compas[1] & 1) ? LED_ON : LED_OFF);
    drv_led_establecer(4, (compas[1] & 2) ? LED_ON : LED_OFF);
}
```

### Control por Máscara
```c
// Patrón binario: bit0=LED1, bit1=LED2, bit2=LED3, bit3=LED4
drv_leds_establecer_mascara(0b1010);  // LEDs 2 y 4 encendidos
```

## Mapeo de Hardware

### LPC2105
| LED | Pin GPIO | Estado Activo |
|-----|----------|---------------|
| LED 1 | LED1_GPIO | HIGH |
| LED 2 | LED2_GPIO | HIGH |
| LED 3 | LED3_GPIO | HIGH |
| LED 4 | LED4_GPIO | HIGH |

### NRF52840 DK
| LED | Pin | Estado Activo |
|-----|-----|---------------|
| LED 1 | P0.13 | LOW |
| LED 2 | P0.14 | LOW |
| LED 3 | P0.15 | LOW |
| LED 4 | P0.16 | LOW |

**Diferencia**: NRF usa lógica inversa (LEDs en cátodo común)

## Implementación

El driver compensa automáticamente la diferencia de polaridad:

```c
void drv_led_establecer(uint8_t num, led_Estado_t estado) {
    if (estado == LED_ON) {
        hal_gpio_escribir(pin, LEDS_ACTIVE_STATE);
    } else {
        hal_gpio_escribir(pin, !LEDS_ACTIVE_STATE);
    }
}
```

Donde `LEDS_ACTIVE_STATE` se define en `board.h`:
- LPC: `#define LEDS_ACTIVE_STATE 1`
- NRF: `#define LEDS_ACTIVE_STATE 0`

---

[← Anterior: Aleatorios](08_ALEATORIOS.md) | [Volver al índice](00_INDICE.md) | [Siguiente: Consumo →](10_CONSUMO.md)
