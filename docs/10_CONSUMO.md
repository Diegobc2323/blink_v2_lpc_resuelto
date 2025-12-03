# ⚡ Funcionalidad: Gestión de Consumo Energético

## Introducción

El driver de consumo (`drv_consumo`) gestiona los modos de bajo consumo del microcontrolador:
- **Idle Mode**: CPU detenida, periféricos activos
- **Sleep Mode**: CPU y algunos periféricos detenidos
- **Deep Sleep**: Solo RTC y wakeup sources activas

## API

```c
void drv_consumo_iniciar(uint32_t monitor_id);
void drv_consumo_esperar(void);     // WFE/WFI (Idle)
void drv_consumo_dormir(void);      // Deep Sleep
```

## Uso en el Proyecto

### Idle en Event Loop (rt_GE_lanzador)
```c
while (1) {
    if (rt_FIFO_extraer(...)) {
        // Procesar evento
    } else {
        drv_consumo_esperar();  // CPU dormida hasta próxima IRQ
    }
}
```

### Deep Sleep por Inactividad (rt_GE_actualizar)
```c
case ev_INACTIVIDAD:
    drv_consumo_dormir();  // Entrar en deep sleep
    break;
```

## Implementaciones HAL

### LPC2105 (hal_consumo_lpc.c)
```c
void hal_consumo_esperar() {
    PCON |= 0x01;  // Idle Mode
}

void hal_consumo_dormir() {
    PCON |= 0x02;  // Power Down Mode
    // Despertar via EINT o RTC
}
```

### NRF52840 (hal_consumo_nrf.c)
```c
void hal_consumo_esperar() {
    __WFE();  // Wait For Event (ARM instruction)
}

void hal_consumo_dormir() {
    NRF_POWER->SYSTEMOFF = 1;  // System OFF mode
    // Despertar solo con GPIO configurado como wakeup
}
```

## Ahorro Energético

| Modo | Consumo (NRF) | Wakeup Sources |
|------|---------------|----------------|
| **Run** | ~4 mA | N/A |
| **Idle (WFE)** | ~1-2 mA | Cualquier evento |
| **System OFF** | ~0.5 µA | GPIO detectar, NFC, Reset |

## Observaciones

1. **WFE vs WFI**: Proyecto usa WFE (más flexible)
2. **Deep Sleep**: Requiere configurar wakeup sources antes
3. **RTC sigue activo**: En modos sleep, para mantener timekeeping

---

[← Anterior: LEDs](09_LEDS.md) | [Volver al índice](00_INDICE.md) | [Siguiente: Eventos →](11_EVENTOS.md)
