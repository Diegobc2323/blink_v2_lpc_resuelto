# ⚡ Funcionalidad: Gestión de Interrupciones

## Introducción

La gestión de interrupciones se distribuye en dos capas principales:
1. **HAL**: Configuración específica de hardware (VIC en LPC, NVIC en NRF)
2. **Drivers**: Uso de interrupciones (botones, timers)

## Arquitectura de Interrupciones

| Componente | LPC2105 | NRF52840 |
|------------|---------|----------|
| **Controlador** | VIC (Vectored Interrupt Controller) | NVIC (Nested Vectored Interrupt Controller) |
| **Canales** | 32 vectores | Múltiples IRQs |
| **Prioridad** | Via slots VIC | Via NVIC_SetPriority |

## Interrupciones Utilizadas

### LPC2105
- **Timer0** (Canal 4): Tiempo periódico
- **EINT0** (Canal 14): Botón 3 (P0.16)
- **EINT1** (Canal 15): Botón 1 (P0.14)
- **EINT2** (Canal 16): Botón 2 (P0.15)

### NRF52840
- **RTC0_IRQn**: Tiempo periódico
- **GPIOTE_IRQn**: Eventos de botones (port event)

## Configuración de Prioridades

**No implementado explícitamente** en este proyecto. Todas las interrupciones tienen prioridad por defecto.

## Buenas Prácticas Implementadas

1. **ISRs Cortas**: Solo encolan evento y retornan
2. **Procesamiento en Usuario**: Callbacks del gestor de eventos
3. **Deshabilitar IRQ en ISR**: Evitar rebotes repetitivos

---

[← Anterior: GPIO](06_GPIO.md) | [Volver al índice](00_INDICE.md) | [Siguiente: Aleatorios →](08_ALEATORIOS.md)
