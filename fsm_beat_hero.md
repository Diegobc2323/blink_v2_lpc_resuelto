# Diagrama de Máquina de Estados - Beat Hero

Este documento describe la lógica de la máquina de estados implementada en `beat_hero.c`.

```mermaid
stateDiagram-v2
    [*] --> e_INIT

    state e_INIT {
        [*] --> DemoAnimacion
        DemoAnimacion --> DemoAnimacion : ev_JUEGO_NUEVO_LED (ID_DEMO)<br/>[Rotar LEDs]
    }

    e_INIT --> e_JUEGO : ev_PULSAR_BOTON (1 o 2)<br/>[Iniciar Partida]

    state e_JUEGO {
        [*] --> Jugando
        
        Jugando --> Jugando : ev_JUEGO_NUEVO_LED (ID_TICK)<br/>[Avanzar Compás / Actualizar LEDs]
        Jugando --> Jugando : ev_PULSAR_BOTON (1 o 2)<br/>[Evaluar Jugada]
        
        Jugando --> FinPartida : Score < -5 (Derrota)
        Jugando --> FinPartida : Compases >= MAX (Victoria)
        Jugando --> FinPartida : ev_PULSAR_BOTON (3 o 4)<br/>[Abortar]
    }

    e_JUEGO --> e_RESULTADO : Fin de Partida

    state e_RESULTADO {
        [*] --> MostrarResultado
        MostrarResultado --> EsperandoReinicio : ev_PULSAR_BOTON (3 o 4)<br/>[Iniciar Timer 3s]
        EsperandoReinicio --> MostrarResultado : ev_SOLTAR_BOTON<br/>[Cancelar Timer]
    }

    e_RESULTADO --> e_INIT : ev_JUEGO_TIMEOUT (ID_RESET)<br/>[Reiniciar Variables]
```

## Descripción de Estados

1. **e_INIT (Menú Principal)**
   - Muestra una animación de demostración en los LEDs.
   - Espera a que el usuario pulse el botón 1 o 2 para comenzar.

2. **e_JUEGO (Partida en Curso)**
   - **Tick de Juego:** Cada cierto tiempo (según BPM) avanza el compás y actualiza los LEDs.
   - **Interacción:** Evalúa las pulsaciones de los botones 1 y 2.
   - **Condiciones de Fin:**
     - **Victoria:** Completar todos los compases.
     - **Derrota:** Puntuación demasiado baja.
     - **Abortar:** Pulsar botones 3 o 4.

3. **e_RESULTADO (Pantalla Final)**
   - Muestra el resultado (todos los LEDs encendidos para victoria, cruz para derrota).
   - Permite reiniciar manteniendo pulsados los botones 3 o 4 durante 3 segundos.
