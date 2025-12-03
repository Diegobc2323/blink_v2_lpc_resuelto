# 📌 Funcionalidad: GPIO

## Introducción

La capa HAL de GPIO proporciona acceso básico a pines de entrada/salida:
- Configuración de dirección (entrada/salida)
- Lectura de estado
- Escritura de valor

## API

```c
void hal_gpio_iniciar(void);
void hal_gpio_establecer_dir(HAL_GPIO_PIN_T pin, HAL_GPIO_DIR_T dir);
void hal_gpio_escribir(HAL_GPIO_PIN_T pin, uint8_t valor);
uint8_t hal_gpio_leer(HAL_GPIO_PIN_T pin);
```

## Uso en el Proyecto

### Lectura de Botones (drv_botones.c)
```c
if (hal_gpio_leer(s_pins_botones[button_id]) == 0) {
    // Botón pulsado (LOW)
}
```

### Control de LEDs (drv_leds.c)
```c
hal_gpio_escribir(pin_led, estado_activo ? 1 : 0);
```

## Configuración Hardware

### LPC2105
- **Registros**: `IOPIN`, `IOSET`, `IOCLR`, `IODIR`
- **Puerto**: P0 (32 bits)

### NRF52840
- **Registros**: `NRF_P0->OUT`, `NRF_P0->IN`, `NRF_P0->DIR`
- **Pin Config**: `NRF_P0->PIN_CNF[pin]`

---

[← Anterior: Watchdog](05_WATCHDOG.md) | [Volver al índice](00_INDICE.md) | [Siguiente: Interrupciones →](07_INTERRUPCIONES.md)
