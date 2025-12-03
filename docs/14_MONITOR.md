# 🐛 Funcionalidad: Sistema de Monitor (Debug)

## Introducción

El sistema de monitor (`drv_monitor`) proporciona **señalización visual de eventos** mediante pines GPIO. Útil para:
- Profiling con osciloscopio
- Debug de timing
- Detección de errores críticos (overflow)

## API

```c
void drv_monitor_iniciar(void);
void drv_monitor_marcar(MONITOR_id_t id);
void drv_monitor_desmarcar(MONITOR_id_t id);
void drv_monitor_pulso(MONITOR_id_t id);
```

## Uso en el Proyecto

### Señalización de Overflow (rt_FIFO_encolar)
```c
if (s_rt_fifo.eventos_a_tratar > TAMCOLA) {
    drv_monitor_marcar(s_rt_fifo.monitor);  // Pin HIGH = Error
    while(1);
}
```

### Profiling de Funciones
```c
void funcion_critica() {
   drv_monitor_marcar(MONITOR1);
    // Código a medir...
    drv_monitor_desmarcar(MONITOR1);
}
```

**Resultado**: Pulso en pin observable con osciloscopio → medir duración

## Mapeo de Hardware

### LPC2105
| Monitor | Pin GPIO |
|---------|----------|
| MONITOR1 | MONITOR1_GPIO |
| MONITOR2 | MONITOR2_GPIO |
| MONITOR3 | MONITOR3_GPIO |
| MONITOR4 | MONITOR4_GPIO |

### NRF52840 DK
| Monitor | Pin |
|---------|-----|
| MONITOR1 | P0.28 |
| MONITOR2 | P0.29 |
| MONITOR3 | P0.30 |
| MONITOR4 | P0.31 |

## Observaciones

1. **Solo para Debug**: No usar en producción
2. **IDs Configurables**: Se pasan al inicializar cada componente
3. **Sobrecarga Mínima**: Escritura directa a GPIO (nanosegundos)

---

[← Anterior: Sección Crítica](13_SECCION_CRITICA.md) | [Volver al índice](00_INDICE.md) | [Siguiente: Sonido →](15_SONIDO.md)
