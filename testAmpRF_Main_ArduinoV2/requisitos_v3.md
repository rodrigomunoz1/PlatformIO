# Descripción
## Introducción del Proyecto
- El microcotrolador es el STM32F103C6T6
- utiliza el framework de arduino en platformio
- la comunicación se realiza por i2c a 100KHz, siendo el microcrontrolador el esclavo
- el microcrontrolador controla dos amplificadores, el Amplificador920 y el Amplificador24, cada uno con sus propios pines de control y secuencias de encendido/apagado y cambio de modo.
- el controlador debe monitorear la temperatura y el consumo de los amplificadores, y en caso de detectar una temperatura o consumo fuera de los rangos predefinidos, debe apagar el amplificador correspondiente y enviar un mensaje de alerta por i2c cuando le llegue consultas por el estado de los amplificadores.
- la dirección i2c del microcontrolador esta definida en el simbolo I2C_ADDRESS con el valor 0x20 y siempre actua como esclavo.

los comandos se ejecutan desde el estado Standby, si el amplificador esta en otro modo, se debe pasar a modo Standby y luego ejecutar el comando.

cuando se pasa de un modo a otro, se debe espertar un tiempo CHANGEMODEDELAY predefinido

todos los comandos que se envien al microcontroador deben ser respondidos por este mediante la respuesta estandar del esclavo.

## Controlador
En esta familia de comandos, el dispositivo ejecuta tareas de monitoreo de temperatura y consumo, y responde a consultas por el estado de los amplificadores. No se pueden ejecutar comandos de cambio de modo o encendido/apagado en este modo, solo se pueden ejecutar comandos de consulta de estado. El campo `amplificador seleccionado` del comando i2c para este modo es 0xFF.

los comandos son:
- `monitoreo`(cod. 0x01): el dispositivo devuelve la temperatura y el consumo en grados Celsius y miliamperios respectivaemnte.
- `estado` (cod. 0x02): el dispositivo devuelve el estado general, es decir, si ha occurrido falla de temperatura o consumo, o si esta todo ok. el dispositivo devuelve el estado de cada amplificador, es decir, si esta en modo Standby, TX, RX, EX, Apagado.
- `ON5V` (cod. 0x0A):pone el pin ON5V a 0 o 1 según sea su argumento de comando.
- `ON50V` (cod. 0x0F): pone el pin ON50V a 0 o 1 según sea su argumento de comando.
  
cuando la temperatura esta por sobre un limite definido por el simbolo TEMPERATURE_LIMIT o el consumo esta por sobre un limite definido por el simbolo CONSUMPTION_LIMIT, el dispositivo debe apagar todos los amplificadores.

si el controlador recibe un comando de cambio de modo o encendido/apagado mientras hay una falla activa, debe responder con un mensaje de error. Solo debe aceptar comandos de consulta de estado mientras hay una falla activa. Si las condiciones de falla desaparecen (baja la temperatura y el consumo), el dispositivo debe volver a aceptar comandos de cambio de modo o encendido/apagado.

la temperatura se mide a travez del ADC1 en el canal especialzado para ello. El consumo se mide también a través del ADC1 por el pin `GlobalSWIsense_Pin`.

## Control del Amplificador920

### Comandos Amplificador920
los comandos son:
- `Encendido` (cod. 0x10): enciende el amplificador y lo deja en modo standby
- `Apagado` (cod. 0x20): apaga el amplificador
- `TX` (cod. 0x40): pone el amplificador en modo TX, el valor de IN_AMP2 e IN_AMP3 se define por argumento de comando
- `RX` (cod. 0x80): pone el amplificador en modo RX, el valor de SEL_LNAOUT2 y SEL_LNAOUT3 se define por argumento de comando
- `EX` (cod. 0xF0): pone el amplificador en modo EX

#### Secuencia de Encendido - Standby

1. SELPWR_LNA=1,SELPWR_AMP=0
2. SELLNA_LNA=1, SELLNA_EX=0
3. IN_AMP2=1,IN_AMP3=1 (50ohm ex)
4. SEL_LNAOUT2=1,SEL_LNAOUT3=0 (50ohm ex)
5. ON_LNA1=1, ON_LNA2=1 (ambos lna en modo shutdown)
6. ON5V=1 (encender 5V)

El estado despues de esta secuencia es Standby.

#### Standby a Modo TX
1. SELPWR_LNA=0, SELPWR_AMP=1
2. IN_AMP2 y IN_AMP3 según argumento de comando (IN_AMP2=bit 0, IN_AMP3=bit 1 del argumento de comando);

#### Modo TX a Standby
1. SELPWR_LNA=1, SELPWR_AMP=0
2. IN_AMP2=1,IN_AMP3=1 (50ohm ex)

#### Standby a Modo RX
1. ON_LNA2 = 0
2. esperar un tiempo ONLNADELAY predefinido
3. ON_LNA1 = 0  
4. esperar un tiempo ONLNADELAY predefinido 
5. SEL_LNAOUT2 y SEL_LNAOUT3 según argumento de comando (SEL_LNAOUT2=bit 0, SEL_LNAOUT3=bit 1 del argumento de comando);

#### Modo RX a Standby
1. ON_LNA1=1 y esperar un tiempo ONLNADELAY predefinido
2. ON_LNA2=1
3. SEL_LNAOUT2=1,SEL_LNAOUT3=0 (50ohm ex)

#### Standby a Modo EX
1. SELLNA_LNA=0, SELLNA_EX=1

#### Modo Ex a Standby
1. SELLNA_LNA=1, SELLNA_EX=0

#### Standby a Apagado
1. ON5V=0 (apagar 5V)
2. Esperar un tiempo POWEROFFDELAY predefinido
3. todos los pines del aplificador920 a 0

## Control del Amplificador24

### Comandos Amplificador24
los comandos son:
- `Encendido` (cod. 0x10): enciende el amplificador y lo deja en modo standby
- `Apagado` (cod. 0x20): apaga el amplificador
- `TX` (cod. 0x40): pone el amplificador en modo TX, el valor de LNA_IN1 y LNA_IN2 se define por argumento de comando (bit0 = LNA_IN1 y bit1 = LNA_IN2)
- `RX` (cod. 0x80): pone el amplificador en modo RX, el valor de LNA_IN1 y LNA_IN2 se define por argumento de comando (bit0 = LNA_IN1 y bit1 = LNA_IN2)

#### Secuencia de Encendido - Standby
campo `modo` del comando i2c: 0x01

1. SELPWR_LNA=1 
2. LNA_ON=1 (LNA en modo shutdown)
3. LNA_IN1=1,LNA_IN2=0
4. ON_AMP=0 (amplificador apagado)
5. ON5V=1 (encender 5V)

El estado despues de esta secuencia es Standby.

#### Standby a Modo TX
1. SELPWR_LNA=0
2. LNA_IN1 y LNA_IN2 según argumento de comando
3. ON_AMP=1 (encender amplificador)

#### Modo TX a Standby
1. ON_AMP=0 (apagar amplificador)
2. SELPWR_LNA=1
3. LNA_IN1=1,LNA_IN2=0

### Standby a Modo RX
1. LNA_ON = 0
2. esperar un tiempo ONLNADELAY predefinido
3. LNA_IN1 y LNA_IN2 según argumento de comando (LNA_IN1=bit 0, LNA_IN2=bit 1 del argumento de comando);

#### Modo RX a Standby
1. LNA_ON=1 y esperar un tiempo ONLNADELAY predefinido
2. LNA_IN1=1,LNA_IN2=0 (50ohm ex)

#### Standby a Apagado
1. ON5V=0 (apagar 5V)
2. Esperar un tiempo POWEROFFDELAY predefinido
3. todos los pines del aplificador24 a 0

# Formato de Comandos y respuestas I2C

## comandos enviados desde el maestro
- el maestro envía comandos en un formato único de la forma:

| dispositivo (1 byte) |comando (1 byte)| argumento (1 byte)|CRC8 (1 byte)
----------------------|-----------------|-------------------|------|

el código para el  campo dispositivo es:
- controlador = 0x11
- amplificador920 = 0x22
- amplificador24 = 0x33

el campo argumento:
- para los comandos `TX`y `RX`, los bits del argumento indican que pines deben activarse y desactivarse. en los otros comando este campo es cero

campo CRC8 bits:
- es un CRC8 bits convencional con polinomio 0x31 sobre los bytes desde dispositivo hasta argumento.

## respuestas del esclavo

- el esclavo envía su respuesta en un formato fijo como sigue:

|dispositivo (1 byte)| comando recibido (1 byte)| resultado (ok/fail/unknown) (1 byte) | respuesta (4 bytes)|CRC8|
|----|----|----|----|---|
- dispositivo: código del dispositivo al cual se le envío el comando al cual se responde
- comando recibido: el codigo del comando al cual se responde
- resultado: ok=0x01. si el comando recibido se ejecuto correctamente, fail= 0xFF si fallo la ejecución y unknown=0x0A si el comando es desconocido
- los 4   bytes de la respuesta solo se usan si es necesario, si no son usados todos los bytes se ponen a 0x00
- para el comando de `monitoreo`, el primer byte es la temperatura, el segundo byte es el consumo con 0x00 correspondiente a 0v y 0xFF correspondiente a 3.3V de la lectura análoga. todos los demás bytes son cero.
- para el comando `estado`, el primer byte de la respuesta es el estado ok=0x01,falla Temperatura=0xF0 o falla Consumo = 0x08, el segundo byte es el modo actual del amplificador 920 y el tercer byte es el modo actual del amplificador de 24. todos los demás bits de este campo son cero.

el dispositivo no ejecutará comandos que impliquen activación de pines cuando esta en estado de falla por temperatura o consumo. los comandos correspondientes deberán devolver como resultado el código fail.

# Requerimiento de Programación
- el código debe estar para arduino en la platformio.
- escuchar por i2c los comandos enviados por el master y ejecutarlos.
- monitorear la temperatura y el consumo de los amplificadores cada 200ms, y en caso de detectar una temperatura o consumo fuera de los rangos predefinidos, debe apaga los amplificadores. no se podrá ejecutar ningun comando de activación de pines hasta que la condición de falla desaparesca, los demás comandos de monitoreo y consulta de estado se podrán ejecutar con condición de falla.
- el maestro despúes de enviar cada comando, enviará un requerimiento de lectura del esclavo, por lo que este debe responder a todos los comandos recibidos según el formato descrito en la sección de "formatos de comandos y respuestas i2c".
- usa la directiva define para definir cada pin, comando, modo, estado, threshold u otro valor relevante que sea relevante de poder modificar fácilmente.
- asegurate de inicializar bien los perifericos, como el i2c y los pines análogos. la temperatura se mide con el sensor de temperatura interno del stm32 (función `analogRead(ATEMP)`). 
- el código debe ser limpio y claro, fácil de entender.
- comenta el código de manera clara y detallada, explicando la lógica detrás de cada sección del código y cómo se relaciona con los requerimientos del proyecto.

# Definicion de Pines

```#define PIN_ON5V PB0
#define PIN_ON50V_GLOBAL PB1
#define PIN_GLOBAL_I_SENSE PA1 //pin analogo sensado de consumo

// Amplificador 920
#define PIN_IN_AMP2_920 PA15
#define PIN_IN_AMP3_920 PA13
#define PIN_SELLNA_LNA_920 PB3
#define PIN_SELLNA_EXT_920 PB5
#define PIN_SELPWR_AMP_920 PB7
#define PIN_SELPWR_LNA_920 PB6
#define PIN_ON_LNA1_920 PB4
#define PIN_ON_LNA2_920 PB14
#define PIN_SEL_LNAOUT2_920 PA11
#define PIN_SEL_LNAOUT3_920 PB13

// Amplificador 24
#define PIN_SEL_PWR_LNA_24 PA10
#define PIN_LNA_ON_24 PA8
#define PIN_LNA_IN1_24 PB12
#define PIN_LNA_IN2_24 PB15
#define PIN_ON_AMP_24 PA9
```


# Referencial (Ignorar para programación)
Descripción de pines amplificador920:
|PIN| Descripción|Función|
|---|------------|-------|
|IN_AMP2| V2 del chip sky13414-485 SW RF (v1 a tierra)| selecciona la entrada RF1 (J2), RF2 (J4) o RF3 (J1) al amplificador según sea la combinación v2 v3|
|IN_AMP3|v3 del chip sky13414-485 SW RF (v1 a tierra)|selecciona la entrada RF1 (J2), RF2 (J4) o RF3 (J1) al amplificador según sea la combinación v2 v3|
|SELLNA_LNA| v2 del chip as179-92lf| conecta la salida del SW de la antena al LNA |
|SELLNA_EX| v1 del chip  as179-92lf|conecta la salida del SW de la antena directamente al tranceiver externo |
|SELPWR_AMP| control1 del chip VSW2-33-10W+| conecta la antena a la salida del amplificador|
|SELPWR_LNA| control2 del chip VSW2-33-10+| conecta la antena a la entrada del selector del LNA AS179-92lf|
|ON_LNA1| shutdown del qpl9547tr7| enciende/apaga el LNA mas cercano a la antena receptora (shutdown con alto)|
|ON_LNA2| shutdown del qpl9547tr7|enciende/apaga el LNA mas cercano a los receptores (shutdown con alto)|
|SEL_LNAOUT2| v2 del chip sky13414-485lf (v1 tierra)| conecta la salida del LNA con alguno de los receptores en las salidas RF1 (J9), RF2 (J5), RF4 (J6) según sea la combinación V2 V3|
|SEL_LNAOUT3|v3 del chip sky13414-485lf (v1 tierra)| conecta la salida del LNA con alguno de los receptores en las salidas RF1 (J9), RF2 (J5), RF4 (J6) según sea la combinación V2 V3|

Descripción de pines Amplificador24:
|PIN| Descripción|Función|
|---|------------|-------|
|LNA_ON| pin shutdown del qpl9547tr7| enciende/apaga el LNA mas cercano al receptor (0=on, 1=off)|
|ON_AMP| pin VEN1 del chip gr5115| enciende/apaga el amplificador 0=apagado, 1=enciende|
|SEL_PWR_LNA| pin T/R del chip MAMF-011119| selecciona la antena para TX/RX (RX=1,TX=0)|
|LNA_IN2| v2 del chip as179-92lf| conecta la salida del selector del LNA a uno de los transmisores o receptores|
|LNA_IN1| v1 del chip as179-92lf| conecta la salida del selector del LNA a uno de los transmisores o receptores|

para el chip sky13414-485:
|v2|v3|Salida| Power Amplifier| RX LNA |
|---|---|------|--------------|--------|
|0|0|RF1| J2 | J9 |
|0|1|RF2| J4 | J5 |
|1|0|RF3| J1 | 50ohm |
|1|1|RF4| 50ohm|  J6 |

para el chip as179-92lf (revisar):
|v1|v2|Salida| 
|---|---|------|
|0|0|indefinido|
|0|1|J2 del chip al LNA|
|1|0|J3 del chip a EXT|
|1|1|indefinido|

