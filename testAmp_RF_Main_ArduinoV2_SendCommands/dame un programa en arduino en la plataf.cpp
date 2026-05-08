dame un programa en arduino en la plataforma plataformio para un devkit v1 esp32 que se comunique con 
el stm32 mediante i2c. este programa debe recibir comandos tipo terminal por el puerto serie a 115200 baudios,
los comandos son:

monitor
status
power <920 | 24 | 5v | 50v> <on | off>
mode <920 | 24> <tx | rx | ex | standby> [args_hex]

el resultado de la ejecución de cada comando debe ser presentado en el terminal en un formato legible 
por un humano, indicando por ejemplo la temperatura en celsius y el consumo en mA, los modos y
comando por su nombre y no por su código hexadecimal, etc.

