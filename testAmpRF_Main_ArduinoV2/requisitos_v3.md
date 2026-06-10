# Especificación de Requerimientos de Firmware: Controlador de Amplificadores (I2C Slave)

## 1. Descripción General del Sistema
Este firmware implementa el software de control para un esclavo I2C basado en el microcontrolador **STM32F103C6T6**. El dispositivo actúa exclusivamente como esclavo en la red, administrando el encendido, apagado, y transiciones de modo de dos subsistemas de amplificación de potencia RF (`Amplificador920` y `Amplificador24`), además de ejecutar tareas críticas de protección por sobretemperatura y sobreconsumo.

### 1.1. Reglas de Operación Críticas
* **Exclusividad de Operación:** Solo un amplificador puede estar activo (en modo funcional `TX`, `RX` o `EX`) a la vez. No se permite la operación simultánea de ambos bloques de RF.
* **Frecuencia del Bus I2C:** 100 kHz (Standard Mode).
* **Dirección I2C:** Definida por el símbolo `I2C_ADDRESS` con el valor `0x20`.
* **Framework de Desarrollo:** Arduino bajo entorno PlatformIO.

---

## 2. Definición de Hardware y Asignación de Pines

Todo el direccionamiento y control de hardware se realiza mediante los pines detallados a continuación. Estos deben configurarse obligatoriamente usando macros `#define`.

```cpp
#define I2C_ADDRESS              0x20

// Pines de Control Global
#define PIN_ON5V                 PB0   // Habilita la línea general de 5V
#define PIN_ON50V_GLOBAL         PB1   // Habilita la línea general de 50V
#define PIN_GLOBAL_I_SENSE       PA1   // Pin analógico para sensado de consumo general

// Subsistema Amplificador 920 MHz
#define PIN_IN_AMP2_920          PA13
#define PIN_IN_AMP3_920          PA15
#define PIN_SELLNA_LNA_920       PB3
#define PIN_SELLNA_EXT_920       PB5
#define PIN_SELPWR_AMP_920       PB7
#define PIN_SELPWR_LNA_920       PB6
#define PIN_ON_LNA1_920          PB4
#define PIN_ON_LNA2_920          PB14
#define PIN_SEL_LNAOUT2_920      PA11
#define PIN_SEL_LNAOUT3_920      PB13

// Subsistema Amplificador 2.4 GHz
#define PIN_SEL_PWR_LNA_24       PA10
#define PIN_LNA_ON_24            PA8
#define PIN_LNA_IN1_24           PB12
#define PIN_LNA_IN2_24           PB15
#define PIN_ON_AMP_24            PA9
```

## 3. Formato de Trama de Comandos y Respuestas I2C
### 3.1. Trama de Comando (Maestro -> Esclavo)
El microcontrolador recibe paquetes I2C con un formato fijo y obligatorio de 4 bytes:
| Byte 0: Dispositivo|	Byte 1: Orden / Comando|	Byte 2: Argumento|	Byte 3: CRC8|
|------------------|-----------------------|------------------|-------------|
*	Códigos de Identificación de Dispositivo:
*	Controlador Global = 0x11 (Para este dispositivo, el campo Argumento se procesa por defecto como 0xFF en comandos globales).
*	Amplificador 920 = 0x22
*	Amplificador 24 = 0x33
*	Manejo del Campo Argumento: * Para comandos de conmutación de RF (TX y RX), los bits individuales del byte definen de forma directa el estado lógico de los pines de selección.
*	Para los demás comandos, este campo se envía por defecto en 0x00.
*	Verificación CRC8: Polinomio convencional 0x31. Se calcula estrictamente sobre los primeros 3 bytes recibidos (Dispositivo, Orden, Argumento). Si el CRC8 no coincide, la trama se descarta y no se ejecuta.
## 3.2. Trama de Respuesta (Esclavo -> Maestro)
Cada comando procesado por el esclavo genera de manera obligatoria una estructura de respuesta de 8 bytes cuando el maestro realiza una solicitud de lectura en el bus:
|Byte 0: Disp.	|Byte 1: Cmd Recibido	|Byte 2: Resultado	|Bytes 3-6: Respuesta (4B)	|Byte 7: CRC8|
|------------------|----|-----------------------|------------------|-------------|
*	Códigos del Campo Resultado:
*	Éxito en ejecución (OK) = 0x01
*	Fallo en ejecución / Alarma de Protección activa (FAIL) = 0xFF
*	Comando no reconocido (UNKNOWN) = 0x0A
*	Estructura Interna del Campo Respuesta (4 bytes):
*	Los bytes que no se utilicen explícitamente en la respuesta de un comando deben ser rellenados obligatoriamente con 0x00.
*	Para Comando monitoreo (0x01):
*	Byte 0: Temperatura interna del STM32 convertida matemáticamente y expresada en Grados Celsius enteros.
*	Byte 1: Consumo general medido en el ADC, mapeado linealmente en un rango de un byte entero (0x00 para 0 V hasta 0xFF para 3.3 V reflejados en el pin analógico).
*	Byte 2: 0x00
*	Byte 3: 0x00
*	Para Comando estado (0x02):
*	Byte 0: Registro de Estado General de Alertas (OK = 0x01, Falla por Temperatura = 0xF0, Falla por Consumo = 0x08).
*	Byte 1: Modo operativo actual del Amplificador 920 (Standby, TX, RX, EX, o Apagado).
*	Byte 2: Modo operativo actual del Amplificador 24 (Standby, TX, RX, o Apagado).
*	Byte 3: 0x00
*	Verificación CRC8: Mismo polinomio 0x31 aplicado sobre los 7 bytes previos constitutivos de la respuesta.
## 4. Lógica de Control Global y Máquina de Estados
### 4.1. Restricción de Estados para Comandos de Potencia
*	Tránsito Obligatorio por Standby: Todos los comandos que involucren un cambio de modo o encendido hacia una etapa funcional activa de RF deben ejecutarse únicamente si el amplificador objetivo se encuentra en estado Standby.
*	Si un amplificador se encuentra en un modo activo (TX, RX, EX) y se solicita cambiar a otro modo activo, el firmware debe forzar secuencialmente la transición de regreso hacia Standby primero, y desde allí procesar la nueva orden.
*	Temporización de Conmutación (CHANGEMODEDELAY): El retardo de tiempo no bloqueante definido por la macro CHANGEMODEDELAY se debe aplicar estrictamente entre el estado Standby y el modo nuevo, asegurando la estabilización eléctrica antes de activar el nuevo camino de RF.

### 4.2. Sistema de Monitoreo Analógico y Seguridad (Protección Activa)
*	Frecuencia de Muestreo: Cada 200 ms, de manera cíclica, el firmware leerá los canales analógicos a través del periférico ADC.
*	Temperatura: Sensor interno del microcontrolador leído mediante el canal correspondiente a ATEMP (calculando su conversión real a °C).
*	Consumo: Corriente general del sistema medida en el pin PIN_GLOBAL_I_SENSE.
*	Límites de Seguridad: Definidos mediante las macros estáticas TEMPERATURE_LIMIT y CONSUMPTION_LIMIT.
*	Acciones ante Eventos de Falla: Si cualquiera de las variables analógicas medidas supera el límite preestablecido:
	1.	Se ejecutan inmediatamente las secuencias de apagado seguro de ambos amplificadores de forma prioritaria.
	2.	El sistema cambia internamente a un estado de Falla Activa (y actualiza el Byte 0 del comando estado).
	3.	Bloqueo del Sistema: Mientras persista la falla, se ignorará y bloqueará cualquier comando que implique la conmutación de pines de potencia o cambios de modo funcional. Si llega un comando de este tipo, la respuesta I2C responderá inmediatamente con el código de error FAIL (0xFF).
	4.	Recuperación Automática: El esclavo mantendrá habilitados únicamente los comandos de lectura de telemetría (estado y monitoreo). En el momento en que las lecturas del ADC retornen de forma estable a los rangos seguros, el firmware levantará la alarma automáticamente, restaurando la operación y permitiendo de nuevo comandos de control.
## 5. Diccionario de Comandos y Secuencias por Dispositivo
### 5.1. Subsistema Controlador Global (Dispositivo 0x11)
*	monitoreo (0x01): Compila y retorna los bytes de temperatura (°C) y consumo mapeado (0x00-0xFF).
*	estado (0x02): Entrega el registro de fallas activas y modos actuales de las etapas de RF.
*	ON5V (0x0A): Controla directamente el estado lógico del pin físico PIN_ON5V según el valor binario enviado en el byte de argumento.
*	ON50V (0x0F): Controla directamente el estado lógico del pin físico PIN_ON50V_GLOBAL según el valor binario enviado en el byte de argumento.
### 5.2. Subsistema Amplificador 920 MHz (Dispositivo 0x22)
*	Encendido (0x10): Ejecuta la secuencia desde el estado inicial apagado hacia el modo Standby.
*	Apagado (0x20): Transiciona ordenadamente desde el estado activo actual hacia el modo Apagado.
*	TX (0x40): Configura la red de conmutación de RF para transmisión según los bits del argumento.
*	RX (0x80): Configura la red de RF y los retardos de LNA para recepción según los bits del argumento.
*	EX (0xF0): Pone el amplificador en modo Transceptor Externo.
Secuencias de Conmutación Detalladas (920 MHz):
*	Secuencia de Encendido -> Standby:
	1.	PIN_SELPWR_LNA_920 = 1, PIN_SELPWR_AMP_920 = 0
	2.	PIN_SELLNA_LNA_920 = 1, PIN_SELLNA_EXT_920 = 0
	3.	PIN_IN_AMP2_920 = 1, PIN_IN_AMP3_920 = 1 (Carga interna de 50 ohm ex)
	4.	PIN_SEL_LNAOUT2_920 = 1, PIN_SEL_LNAOUT3_920 = 0 (Carga interna de 50 ohm ex)
	5.	PIN_ON_LNA1_920 = 1, PIN_ON_LNA2_920 = 1 (Ambos integrados LNA en modo Shutdown por lógica alta)
	6.	PIN_ON5V = 1 (Habilitación de la energía del bus)
*	Standby -> Modo TX:
	1.	Aplicar retraso CHANGEMODEDELAY antes de iniciar los cambios físicos sobre los pines.
	2.	PIN_SELPWR_LNA_920 = 0, PIN_SELPWR_AMP_920 = 1
	3.	Mapear al pin PIN_IN_AMP2_920 el valor del Bit 0 del argumento, y al pin PIN_IN_AMP3_920 el Bit 1 del argumento.
*	Modo TX -> Standby:
	1.	PIN_SELPWR_LNA_920 = 1, PIN_SELPWR_AMP_920 = 0
	2.	PIN_IN_AMP2_920 = 1, PIN_IN_AMP3_920 = 1
*	Standby -> Modo RX:
	1.	Aplicar retraso CHANGEMODEDELAY antes de iniciar los cambios físicos sobre los pines.
	2.	PIN_ON_LNA2_920 = 0 (Enciende el LNA secundario)
	3.	Esperar tiempo de estabilización definido en macro ONLNADELAY.
	4.	PIN_ON_LNA1_920 = 0 (Enciende el LNA primario cercano a antena)
	5.	Esperar tiempo de estabilización definido en macro ONLNADELAY.
	6.	Mapear al pin PIN_SEL_LNAOUT2_920 el valor del Bit 0 del argumento, y al pin PIN_SEL_LNAOUT3_920 el Bit 1 del argumento.
*	Modo RX -> Standby:
	1.	PIN_ON_LNA1_920 = 1 $\rightarrow$ Esperar tiempo de estabilización ONLNADELAY.
	2.	PIN_ON_LNA2_920 = 1
	3.	PIN_SEL_LNAOUT2_920 = 1, PIN_SEL_LNAOUT3_920 = 0
*	Standby -> Modo EX:
	1.	Aplicar retraso CHANGEMODEDELAY antes de iniciar los cambios físicos sobre los pines.
	2.	PIN_SELLNA_LNA_920 = 0, PIN_SELLNA_EXT_920 = 1
*	Modo EX -> Standby:
	1.	PIN_SELLNA_LNA_920 = 1, PIN_SELLNA_EXT_920 = 0
*	Standby -> Apagado:
	1.	PIN_ON5V = 0 (Si el otro amplificador no requiere la línea activa)
	2.	Esperar tiempo de retraso definido en la macro POWEROFFDELAY.
	3.	Forzar a nivel lógico 0 todos los pines de salida asociados al Amplificador 920.
## 5.3. Subsistema Amplificador 2.4 GHz (Dispositivo 0x33)
*	Encendido (0x10): Ejecuta la secuencia desde el estado inicial apagado hacia el modo Standby.
*	Apagado (0x20): Transiciona ordenadamente desde el estado activo actual hacia el modo Apagado.
*	TX (0x40): Configura la red de conmutación de RF para transmisión según los bits del argumento.
*	RX (0x80): Configura la red de RF y los retardos de LNA para recepción según los bits del argumento.
Secuencias de Conmutación Detalladas (2.4 GHz):
*	Secuencia de Encendido -> Standby:
	1.	PIN_SEL_PWR_LNA_24 = 1
	2.	PIN_LNA_ON_24 = 1 (LNA en modo Shutdown por lógica alta)
	3.	PIN_LNA_IN1_24 = 1, PIN_LNA_IN2_24 = 0
	4.	PIN_ON_AMP_24 = 0 (Etapa de amplificación desactivada)
	5.	PIN_ON5V = 1 (Habilitación de la energía del bus)
*	Standby -> Modo TX:
	1.	Aplicar retraso CHANGEMODEDELAY antes de iniciar los cambios físicos sobre los pines.
	2.	PIN_SEL_PWR_LNA_24 = 0
	3.	Mapear al pin PIN_LNA_IN1_24 el valor del Bit 0 del argumento, y al pin PIN_LNA_IN2_24 el Bit 1 del argumento.
	4.	PIN_ON_AMP_24 = 1
*	Modo TX -> Standby:
	1.	PIN_ON_AMP_24 = 0
	2.	PIN_SEL_PWR_LNA_24 = 1
	3.	PIN_LNA_IN1_24 = 1, PIN_LNA_IN2_24 = 0
*	Standby -> Modo RX:
	1.	Aplicar retraso CHANGEMODEDELAY antes de iniciar los cambios físicos sobre los pines.
	2.	PIN_LNA_ON_24 = 0 (Enciende el bloque LNA por lógica baja)
	3.	Esperar tiempo de estabilización definido en macro ONLNADELAY.
	4.	Mapear al pin PIN_LNA_IN1_24 el valor del Bit 0 del argumento, y al pin PIN_LNA_IN2_24 el Bit 1 del argumento.
*	Modo RX -> Standby:
	1.	PIN_LNA_ON_24 = 1 (Apaga LNA) $\rightarrow$ Esperar tiempo de estabilización ONLNADELAY.
	2.	PIN_LNA_IN1_24 = 1, PIN_LNA_IN2_24 = 0 (Conmutación segura a carga interna)
*	Standby -> Apagado:
	1.	PIN_ON5V = 0
	2.	Esperar tiempo de retraso definido en la macro POWEROFFDELAY.
	3.	Forzar a nivel lógico 0 todos los pines de salida asociados de forma exclusiva al Amplificador 24.