# Diagrama de Máquina de Estados - Driver Botones

Este documento describe la lógica de la máquina de estados implementada en `drv_botones.c` para la gestión de rebotes e interrupciones.

```mermaid
stateDiagram-v2
    [*] --> e_esperando

    state "e_esperando<br/>(IRQ Habilitada)" as e_esperando
    state "e_rebotes<br/>(Espera TRP)" as e_rebotes
    state "e_muestreo<br/>(Polling TEP)" as e_muestreo
    state "e_salida<br/>(Espera TRD)" as e_salida

    e_esperando --> e_rebotes : ISR -> ev_PULSAR_BOTON<br/>[Deshabilitar IRQ, Iniciar Timer TRP]

    e_rebotes --> e_muestreo : Timeout TRP && Botón Pulsado (LOW)<br/>[Confirmar Pulsación, Iniciar Timer TEP]
    e_rebotes --> e_esperando : Timeout TRP && Botón Soltado (HIGH)<br/>[Falsa Alarma, Rehabilitar IRQ]

    e_muestreo --> e_muestreo : Timeout TEP && Botón Pulsado<br/>[Seguir Muestreando]
    e_muestreo --> e_salida : Timeout TEP && Botón Soltado (HIGH)<br/>[Notificar Soltado, Iniciar Timer TRD]

    e_salida --> e_esperando : Timeout TRD<br/>[Limpiar Flags, Rehabilitar IRQ]
```

## Descripción de Estados

1. **e_esperando**
   - Estado inicial.
   - La interrupción del botón está habilitada.
   - Espera a que el usuario pulse el botón (flanco de bajada).

2. **e_rebotes**
   - Se entra tras detectar una interrupción.
   - Se espera un tiempo `TRP_MS` (80ms) para filtrar rebotes mecánicos iniciales.
   - Al finalizar el tiempo, se verifica el estado real del pin.

3. **e_muestreo**
   - Estado estable de "Botón Pulsado".
   - Se verifica periódicamente cada `TEP_MS` (50ms) si el botón sigue pulsado.
   - Si se detecta que se ha soltado, se pasa al estado de salida.

4. **e_salida**
   - Se espera un tiempo de seguridad `TRD_MS` (50ms) para evitar rebotes al soltar.
   - Tras este tiempo, se limpian las interrupciones pendientes y se rehabilitan para el próximo ciclo.
