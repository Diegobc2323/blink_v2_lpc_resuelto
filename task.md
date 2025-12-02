# Task: Crear Target DEBUG en Keil

## Objetivo
Crear un nuevo target llamado DEBUG en Keil que habilite el modo de depuración utilizando los archivos test existentes.

## Tareas

### 1. Análisis y Diseño
- [ ] Revisar archivos test.c y test.h
- [ ] Identificar tests disponibles
- [ ] Definir comportamiento del modo DEBUG
- [ ] Determinar símbolos de preprocesador necesarios

### 2. Configuración de Keil
- [ ] Crear nuevo target "DEBUG" en el proyecto
- [ ] Configurar símbolos de preprocesador (DEBUG_MODE)
- [ ] Configurar opciones de optimización
- [ ] Configurar opciones de debugging

### 3. Modificación del Código
- [ ] Modificar main.c con directivas #ifdef DEBUG_MODE
- [ ] Integrar llamadas a funciones test
- [ ] Configurar comportamiento según mode seleccionado

### 4. Documentación
- [ ] Documentar cómo crear target en Keil
- [ ] Documentar cómo usar el modo DEBUG
- [ ] Documentar tests disponibles

### 5. Verificación
- [ ] Compilar target DEBUG
- [ ] Compilar target normal (asegurar que no afecte)
- [ ] Probar ejecución de tests en placa
