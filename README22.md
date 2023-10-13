Introducción

Este proyecto implementa una versión en línea del clásico juego de Pong utilizando la API de sockets de Berkeley, una interfaz de programación estandarizada para crear conexiones de red en sistemas operativos derivados de Unix. A través del uso del tipo de socket SOCK_STREAM, se garantiza una comunicación confiable y orientada a la conexión entre el servidor y los clientes. El juego permite que un usuario inicie una instancia de servidor, mientras que otros dos usuarios pueden actuar como clientes, conectándose y compitiendo en un juego de Pong en línea. Ambos clientes interactúan con el mismo juego en tiempo real, gracias a las constantes actualizaciones de estado enviadas por el servidor. Para facilitar la comunicación entre el cliente y el servidor, se diseñó un protocolo personalizado basado en texto, que define cómo se transmiten y reciben los diferentes comandos y estados del juego entre las partes. Este enfoque basado en texto simplifica el proceso de interpretación y manejo de mensajes, haciendo que la implementación sea más directa y legible.

Requerimientos del Sistema
Antes de compilar y ejecutar el juego, asegúrate de que tu sistema cumpla con los siguientes requerimientos:

Sistema Operativo: Unix o derivados de Unix (como Linux o Mac os).
Bibliotecas: SDL2 y SDL2_ttf instaladas.
Compilador GCC.

Compilación

Para compilar los archivos, necesitarás tener instaladas las bibliotecas SDL2 y SDL2_ttf. Asegúrate de tenerlas antes de proceder.

Compilar el Cliente:

gcc client3.c -o client -lSDL2 -lSDL2_ttf -lpthread

Compilar el Servidor:

gcc server3.c -o server -lpthread

Cómo correr el código

Servidor:

./server <port> <log_file>

Cliente:

./client <ip> <port>

Una vez que ambos clientes estén conectados y hayan elegido a su jugador, el juego comenzará. Usa las teclas de flecha ARRIBA y ABAJO para mover tu paleta.

Funciones

Cliente

initSDL(): Inicializa SDL para renderizar el juego.

renderScore(int, int): Renderiza la puntuación actual de ambos jugadores en la pantalla.

renderGame(float, float, float, float, int, int): Renderiza el estado del juego, incluidos las paletas, la bola y la puntuación.

renderGameOver(int, int): Muestra la pantalla de fin de juego con las puntuaciones finales.

main(int, char\*\*): Punto de entrada. Se conecta al servidor, envía el registro y la selección del jugador, y entra en el bucle del juego.

Servidor

broadcast_game_state(): Envía el estado actual del juego a todos los clientes conectados.

log_event(const char\*): Registra eventos del juego en un archivo.

check_game_end(): Verifica si el juego ha terminado según las puntuaciones de los jugadores.

update(): Actualiza el estado del juego, incluida la posición de la bola y la verificación de colisiones.

game_loop(void\*): Bucle principal del juego que actualiza el estado del juego y lo transmite a los clientes.

handle_client(void\*): Maneja la comunicación con un solo cliente.

main(int, char\*\*): Punto de entrada. Inicia el servidor, escucha las conexiones entrantes de los clientes y crea hilos para manejar a los clientes.
