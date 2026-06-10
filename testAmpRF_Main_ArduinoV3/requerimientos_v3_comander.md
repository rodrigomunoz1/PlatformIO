se requiere un programa para enviar comandos i2c a un dispositivo esclavo, los comandos son introducidos por el usuario a través del puerto serial y traducidos a comandos i2c para el dispositivo esclavo. El programa debe ser capaz de recibir comandos por el puerto serial y ejecutar acciones en base a esos comandos, además de imprimir los resultados de los comandos por el puerto serial en un formato logible y fácil de entender por un humano.

* dispositivo: dev kit v1 esp32
* Arduino PlatformIO
* dirección i2c del esclavo: 0x20
* velocidad de comunicación i2c: 100kHz
* los resultados de los comandos deben ser impresos por el puerto serial en un formato logible y fácil de entetender por un humano.
* los comandos se deben enviar por i2c al esclavo. 

los comandos que se deben escribir en el puerto serie son:

* `mon`:  imprime los datos del comando de monitoreo
* `stat`: imprime los datos del comando de estado

* `amp920 on`: enciende la linea de 5V de los dos amplificadores
* `amp920 off`: apaga la linea de 5V de los amplificadores
* `amp920 RX <arg>`: pone el amplificador 920 en modo RX con el valor de arg
* `amp920 TX <arg>`: pone el amplificador 920 en modo TX con el valor de arg
* `amp920 EXT`: pone el amplificador 920 en modo externo (`EX`)

* `amp24 on`:enciende la linea de 5V de los dos amplificadores (igual que amp920 on)
* `amp24 off`: apaga la linea de 5V de los amplificadores (igual que amp920 off)
* `amp24 RX <arg>`: pone el amplificador 24 en modo RX con el valor de arg
* `amp24 TX <arg>`: pone el amplificador 24 en modo TX con el valor de arg

todos los comandos deben imprimir la respuesta del esclavo.

los requemientos del software del esclavo y su programa se adjuntan en el documento `requerimientos_v3.md` y `main.c`. en estos documentos se especifican los comandos i2c que el esclavo espera recibir y las acciones que debe ejecutar en base a esos comandos.

