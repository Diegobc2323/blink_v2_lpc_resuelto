# Plan: Crear Target DEBUG en Keil

## Análisis del Sistema Actual

He analizado tus archivos test y tengo claras las capacidades disponibles:

### Tests Disponibles
Tu archivo `test.c` incluye los siguientes tests:

1. **`test_fifo()`** - Prueba de cola FIFO
2. **`test_gestor_eventos()`** - Prueba del gestor de eventos
3. **`test_alarmas()`** - Prueba del servicio de alarmas
4. **`test_seccion_critica()`** - Prueba de secciones críticas
5. **`test_watchdog()`** - Prueba del watchdog timer
6. **`test_ejecutar_todos()`** - Ejecuta todos los tests secuencialmente
7. **`test_modo_interactivo_botones()`** - Modo interactivo para probar botones
8. **`test_botones_simultaneos_iniciar()`** - Detecta pulsaciones simultáneas

### Main.c Actual
Tu `main.c` actualmente:
- Inicializa todo el hardware y drivers
- Llama a `beat_hero_iniciar()` para el juego
- Lanza el gestor de eventos con `rt_GE_lanzador()`

## Preguntas para Definir el Modo DEBUG

Antes de implementar, necesito saber qué quieres hacer en modo DEBUG:

### Pregunta 1: ¿Qué escenario prefieres?

**Escenario A: Tests Automáticos al Inicio** 
- Al arrancar en modo DEBUG, ejecuta `test_ejecutar_todos()`
- Muestra resultados con LEDs
- Después de los tests, ¿quieres que: 
  - a) Inicie el juego normal
  - b) Entre en modo interactivo de botones
  - c) Se quede esperando sin hacer nada

**Escenario B: Modo Interactivo de Botones**
- Reemplaza `beat_hero` con `test_modo_interactivo_botones()`
- Los LEDs muestran qué botones están presionados
- No hay juego, solo pruebas de botones

**Escenario C: Menú de Selección**
- Al arrancar, espera que presiones un botón:
  - Botón 1: Ejecutar todos los tests
  - Botón 2: Modo interactivo de botones
  - Botón 3: Tests de botones simultáneos
  - Botón 4: Iniciar el juego normal

### Pregunta 2: ¿Quieres usar el monitor?
- ¿Deseas activar marcas de monitor adicionales para debugging?
- ¿Quieres mensajes más detallados en el monitor durante los tests?

### Pregunta 3: ¿Optimización?
- Target DEBUG: ¿Sin optimización (-O0) para facilitar debugging?
- Target normal: ¿Mantener optimización actual?

## Cómo Crear el Target DEBUG en Keil (Paso a Paso)

### Para nRF52840 (blink_nrf.uvprojx)

1. **Abrir el proyecto**
   - Abre `nrf/keil/blink_nrf.uvprojx` en Keil µVision

2. **Crear nuevo target**
   - Click derecho en el nombre del proyecto (en el panel izquierdo)
   - Seleccionar `Manage Project Items...`
   - En la pestaña `Project Targets`
   - Click en el botón `New (Insert)` al lado de la lista de targets
   - Nombrar el nuevo target como: **DEBUG**
   - Click `OK`

3. **Configurar símbolos de preprocesador**
   - Click derecho en el target `DEBUG` → `Options for Target 'DEBUG'...`
   - Ir a la pestaña `C/C++`
   - En el campo `Define:` agregar: `DEBUG_MODE`
   - Si ya hay otros símbolos (ej: `BOARD_PCA10056`), mantenerlos y agregar `DEBUG_MODE` separado por comas:
     ```
     BOARD_PCA10056, DEBUG_MODE
     ```

4. **Configurar optimización (opcional)**
   - En la misma ventana, pestaña `C/C++`
   - Sección `Optimization`
   - Seleccionar `-O0` (sin optimización) para facilitar debugging
   - Click `OK`

5. **Configurar opciones de debug**
   - Pestaña `Debug`
   - Asegurarse de que el debugger esté configurado correctamente (J-Link para nRF)

6. **Seleccionar el target activo**
   - En la barra de herramientas, el dropdown con el nombre del target
   - Seleccionar `DEBUG` cuando quieras compilar el modo debug
   - Seleccionar el target original cuando quieras compilar el juego normal

### Para LPC (blink_lpc.uvproj) - Mismo proceso
Sigue los mismos pasos pero para el proyecto `lpc/keil/blink_lpc.uvproj`

## Propuesta de Implementación (Escenario A + Opción Manual)

Te sugiero implementar una solución flexible que permita:

### En main.c con `#ifdef DEBUG_MODE`

```c
int main(void){
    // Inicializaciones normales...
    drv_tiempo_iniciar();
    drv_monitor_iniciar();
    // ... resto de inicializaciones ...
    
    #ifdef DEBUG_MODE
        // ========== MODO DEBUG ==========
        
        drv_monitor_marcar(200); // Marca: Modo DEBUG activo
        
        // Opción 1: Ejecutar todos los tests al inicio
        uint32_t tests_pasados = test_ejecutar_todos();
        drv_monitor_marcar(200 + tests_pasados); // Marca: N tests pasados
        
        // Esperar 3 segundos para ver resultados en LEDs
        uint32_t inicio = drv_tiempo_actual_ms();
        while ((drv_tiempo_actual_ms() - inicio) < 3000) {
            // Espera activa
        }
        
        // Apagar LEDs
        for (int i = 1; i <= LEDS_NUMBER; i++) {
            drv_led_establecer(i, LED_OFF);
        }
        
        // Opción 2: Ahora entrar en modo interactivo de botones
        // (comentar beat_hero_iniciar y usar test en su lugar)
        rt_GE_suscribir(ev_PULSAR_BOTON, 1, test_modo_interactivo_botones);
        rt_GE_suscribir(ev_SOLTAR_BOTON, 1, test_modo_interactivo_botones);
        
        drv_monitor_marcar(250); // Marca: Modo interactivo iniciado
        
    #else
        // ========== MODO NORMAL ==========
        drv_aleatorios_iniciar(0);
        beat_hero_iniciar();
    #endif
    
    // Lanzar sistema (común para ambos modos)
    rt_GE_lanzador();
    
    while (1);
}
```

## Proposed Changes

### [MODIFY] [main.c](file:///c:/Users/propietario/Desktop/Diego/Ingenieria_Informatica/3er_year/Proyecto%20Hardware/Final%20Final/blink_v2_lpc_resuelto/src/main.c)

Agregar:
- `#include "test.h"` al inicio
- Directiva `#ifdef DEBUG_MODE` para separar lógica de debug vs normal
- Llamadas a funciones de test según el escenario elegido

### [NO CHANGE] test.c y test.h
Los archivos test ya están completos y funcionan correctamente.

## User Review Required

> [!IMPORTANT]
> **Decisiones de Diseño Necesarias**
> 
> Por favor responde las siguientes preguntas para que pueda implementar el modo DEBUG exactamente como lo necesitas:
> 
> 1. **¿Qué escenario prefieres?**
>    - A) Tests automáticos al inicio + luego modo interactivo
>    - B) Solo modo interactivo de botones (sin tests automáticos)
>    - C) Menú de selección con botones
>    - D) Solo ejecutar tests y quedarse en espera
> 
> 2. **Después de ejecutar tests automáticos, ¿qué debería pasar?**
>    - a) Iniciar el juego normal
>    - b) Entrar en modo interactivo de botones
>    - c) Quedarse en espera (solo ver resultados)
> 
> 3. **¿Necesitas alguna funcionalidad adicional en modo DEBUG?**
>    - Ejemplos: test de un módulo específico, pruebas de estrés, logs por monitor, etc.

## Verification Plan

### Manual Verification

1. **Compilar ambos targets**:
   ```
   - Compilar target DEBUG (debe definir DEBUG_MODE)
   - Compilar target normal (no debe definir DEBUG_MODE)
   - Verificar que ambos compilan sin errores
   ```

2. **Probar target DEBUG en placa**:
   ```
   - Cargar firmware del target DEBUG
   - Observar LEDs durante ejecución de tests
   - Verificar marcas del monitor
   - Probar funcionalidad según escenario elegido
   ```

3. **Verificar target normal no afectado**:
   ```
   - Cargar firmware del target normal
   - Verificar que el juego funciona igual que antes
   - Confirmar que no hay código de test ejecutándose
   ```

## Notas Adicionales

### Ventajas de este Approach
- ✅ No modifica el código normal del juego
- ✅ Fácil de cambiar entre modo DEBUG y normal
- ✅ Los tests están completamente aislados
- ✅ Se puede depurar con breakpoints fácilmente en modo DEBUG

### Estructura Recomendada de Símbolos
```
Target Normal:  BOARD_PCA10056
Target DEBUG:   BOARD_PCA10056, DEBUG_MODE
```
