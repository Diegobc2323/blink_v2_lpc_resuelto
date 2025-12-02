/* *****************************************************************************
 * ARCHIVO DE TEST - Beat Hero
 * Definiciones y prototipos para los tests del sistema
 * ****************************************************************************/

#ifndef TEST_H
#define TEST_H

#include <stdint.h>
#include <stdbool.h>
#include "rt_evento_t.h"

/* =============================================================================
 * PROTOTIPOS DE FUNCIONES DE TEST
 * ===========================================================================*/

/**
 * @brief Test de la cola FIFO
 * 
 * Verifica:
 * - Encolar y extraer eventos
 * - Orden FIFO correcto
 * - Gestión de cola llena y vacía
 * - Protección con sección crítica
 * 
 * @return true si todas las pruebas pasan, false en caso contrario
 */
bool test_fifo(void);

/**
 * @brief Test del Gestor de Eventos
 * 
 * Verifica:
 * - Suscripción a eventos
 * - Despacho de eventos a callbacks suscritos
 * - Prioridades de callbacks
 * - Cancelar suscripciones
 * 
 * @return true si todas las pruebas pasan
 */
bool test_gestor_eventos(void);

/**
 * @brief Test del servicio de alarmas
 * 
 * Verifica:
 * - Programar alarmas esporádicas
 * - Programar alarmas periódicas
 * - Cancelar alarmas
 * - Codificación de flags
 * - Múltiples alarmas simultáneas
 * 
 * @return true si todas las pruebas pasan
 */
bool test_alarmas(void);

/**
 * @brief Iniciar test de botones con detección de pulsaciones simultáneas
 * 
 * Este test debe ejecutarse con el sistema completo corriendo.
 * Instrucciones:
 * 1. Llamar a esta función en el main
 * 2. Presionar botones 1 y 2 al mismo tiempo
 * 3. Si se detecta pulsación simultánea, LED 4 se enciende
 * 
 * Suscribe callbacks al gestor de eventos para detectar botones simultáneos.
 */
void test_botones_simultaneos_iniciar(void);

/**
 * @brief Verificar si se detectó pulsación simultánea de botones
 * 
 * @return true si se detectaron 2 o más botones presionados al mismo tiempo
 */
bool test_botones_simultaneos_verificar(void);

/**
 * @brief Test de secciones críticas
 * 
 * Verifica:
 * - Entrar y salir de secciones críticas
 * - Anidamiento correcto
 * - Protección de variables compartidas
 * 
 * @return true si el test pasa
 */
bool test_seccion_critica(void);

/**
 * @brief Test del Watchdog Timer
 * 
 * Verifica:
 * - Inicialización del WDT
 * - Alimentación periódica
 * 
 * ADVERTENCIA: Para probar el reset por timeout, comentar drv_WDT_alimentar()
 * en rt_GE_lanzador() y observar que el sistema se reinicia automáticamente.
 * 
 * @return true si el test de alimentación pasa
 */
bool test_watchdog(void);

/**
 * @brief Ejecutar todos los tests secuencialmente
 * 
 * Instrucciones:
 * 1. Llamar a esta función desde el main antes de rt_GE_lanzador()
 * 2. Observar los LEDs para ver qué tests pasan:
 *    - Durante cada test se encienden LEDs específicos
 *    - Al final, el número de LEDs encendidos = tests pasados
 * 3. Revisar las marcas del monitor para debugging detallado
 * 
 * Marcas del monitor:
 * - 100: Inicio de suite completa
 * - 10-11: Test FIFO
 * - 12-13: Test Gestor de Eventos
 * - 14-15: Test Alarmas
 * - 18-19: Test Sección Crítica
 * - 20-21: Test Watchdog
 * - 101: Fin de suite
 * - 100+N: Suite finalizada con N tests pasados
 * 
 * @return Número de tests que pasaron (máximo 6)
 */
uint32_t test_ejecutar_todos(void);

/**
 * @brief Modo de test interactivo de botones
 * 
 * Usar como reemplazo de beat_hero_actualizar() para probar botones.
 * 
 * Funcionamiento:
 * - LED 1: Botón 1 presionado
 * - LED 2: Botón 2 presionado
 * - LED 3: Botón 3 presionado
 * - LED 4: 2 o más botones presionados simultáneamente
 * 
 * Ejemplo de uso en main.c:
 * @code
 * // Comentar beat_hero_iniciar()
 * // En su lugar:
 * rt_GE_suscribir(ev_PULSAR_BOTON, 1, test_modo_interactivo_botones);
 * rt_GE_suscribir(ev_SOLTAR_BOTON, 1, test_modo_interactivo_botones);
 * @endcode
 * 
 * @param evento Evento recibido (ev_PULSAR_BOTON o ev_SOLTAR_BOTON)
 * @param id_boton ID del botón (0, 1, 2, 3)
 */
void test_modo_interactivo_botones(EVENTO_T evento, uint32_t id_boton);

/* =============================================================================
 * CALLBACKS INTERNOS (No llamar directamente)
 * ===========================================================================*/

void test_callback_evento(EVENTO_T evento, uint32_t auxData);
void test_callback_alarma(EVENTO_T evento, uint32_t auxData);
void test_callback_botones_simultaneos(EVENTO_T evento, uint32_t id_boton);

#endif /* TEST_H */
