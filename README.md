# **Introducción**

Este proyecto implementa una versión en línea del clásico juego de Pong utilizando la API de sockets de Berkeley, una interfaz de programación estandarizada para crear conexiones de red en sistemas operativos derivados de Unix. A través del uso del tipo de socket SOCK_STREAM, se garantiza una comunicación confiable y orientada a la conexión entre el servidor y los clientes.

El juego permite que un usuario inicie una instancia de servidor, mientras que otros dos usuarios pueden actuar como clientes, conectándose y compitiendo en un juego de Pong en línea. Ambos clientes interactúan con el mismo juego en tiempo real, gracias a las constantes actualizaciones de estado enviadas por el servidor.

Para facilitar la comunicación entre el cliente y el servidor, se diseñó un protocolo personalizado basado en texto, que define cómo se transmiten y reciben los diferentes comandos y estados del juego entre las partes. Este enfoque basado en texto simplifica el proceso de interpretación y manejo de mensajes, haciendo que la implementación sea más directa y legible.

## **Requerimientos del Sistema**

Antes de compilar y ejecutar el juego, asegúrate de que tu sistema cumpla con los siguientes requerimientos:

- **Sistema Operativo:** Unix o derivados de Unix (como Linux o Mac os).
- **Bibliotecas:** SDL2 y SDL2_ttf instaladas.
- **Compilador:** GCC.

## **Compilación**

Para compilar los archivos, necesitarás tener instaladas las bibliotecas SDL2 y SDL2_ttf. Asegúrate de tenerlas antes de proceder.

**Compilar el Servidor:**

```
gcc server3.c -o server -lpthread
```

**Compilar el Cliente:**

```
gcc client3.c -o client -lSDL2 -lSDL2_ttf -lpthread
```

## **Cómo correr el código**

**Servidor:**

```
./server <port> <log_file>
```

**Cliente:**

```
./client <ip> <port>
```

Una vez que ambos clientes estén conectados y hayan elegido a su jugador, el juego comenzará. Usa las teclas de flecha ARRIBA y ABAJO para mover tu paleta.

## **Funciones**

### Cliente

- `initSDL()`: Inicializa SDL para renderizar el juego.
- `renderScore(int, int)`: Renderiza la puntuación actual de ambos jugadores en la pantalla.
- `renderGame(float, float, float, float, int, int)`: Renderiza el estado del juego, incluidos las paletas, la bola y la puntuación.
- `renderGameOver(int, int)`: Muestra la pantalla de fin de juego con las puntuaciones finales.
- `main(int, char**)`: Punto de entrada. Se conecta al servidor, envía el registro y la selección del jugador, y entra en el bucle del juego.

### Servidor

- `broadcast_game_state()`: Envía el estado actual del juego a todos los clientes conectados.
- `log_event(const char*)`: Registra eventos del juego en un archivo.
- `check_game_end()`: Verifica si el juego ha terminado según las puntuaciones de los jugadores.
- `update()`: Actualiza el estado del juego, incluida la posición de la bola y la verificación de colisiones.
- `game_loop(void*)`: Bucle principal del juego que actualiza el estado del juego y lo transmite a los clientes.
- `handle_client(void*)`: Maneja la comunicación con un solo cliente.
- `main(int, char**)`: Punto de entrada. Inicia el servidor, escucha las conexiones entrantes de los clientes y crea hilos para manejar a los clientes.

## **Protocolo del Juego de Ping Pong**

Protocolo utilizado para la comunicación.

### Mensajes de Registro y Autenticación

- `PLAYER1`: Solicitud para registrarse como el jugador 1.
- `PLAYER2`: Solicitud para registrarse como el jugador 2.
- `Player` in use: Indica que el rol de jugador solicitado ya está ocupado.
- `Player` accepted: Confirma que el rol de jugador ha sido aceptado.
- `REGISTRATION`:NICKNAME:<nickname>:EMAIL:<email>: Registro del jugador con un nickname y correo electrónico.

### Estado del Juego

- `WAITING`: Esperando al otro jugador para comenzar.
- `READY`: Ambos jugadores están listos para comenzar el juego.
- `STATE`:P1:<y-pos>,P2:<y-pos>,BALL:<x-pos>:<y-pos>,SCORE:P1:<score1>:P2:<score2>,NICKS:<nick1>:<nick2>:
- `Mensaje` de estado que describe la posición actual de los jugadores y la pelota, junto con el puntaje actual y los nicknames de los jugadores.
- `GAME`:OVER: Notificación de que el juego ha terminado.

### Acciones de Jugador

- `ACTION`:UP: Movimiento hacia arriba del jugador.
- `ACTION`:DOWN: Movimiento hacia abajo del jugador.
  
<img src="https://github.com/sebastianvelezg/telepong/assets/68916783/20367305-5a4d-409a-825a-d9fa8e8ac2f8" width="400" alt="board-silly-cutup"> <img src="https://github.com/sebastianvelezg/telepong/assets/68916783/07099895-8283-4396-840c-2766c16852dc" width="400" alt="board-jocular-prankster"> 


## **Desarrollo**

### Definiciones iniciales:

- Se definen varias constantes como `SCREEN_WIDTH, SCREEN_HEIGHT, BALL_RADIUS, PADDLE_WIDTH, PADDLE_HEIGHT, PADDLE_SPEED, BALL_SPEED_X, BALL_SPEED_Y, WINNING_SCORE y MAX_CLIENTS`.
- Se define un archivo `logfile` que se utilizará para registrar eventos del juego.
- Se definen múltiples `pthread_mutex_t` y `pthread_cond_t` para manejar el acceso concurrente y la sincronización entre hilos.

### Objetos del juego:

- `Ball`: Este objeto se representa mediante la estructura Ball. Esta estructura contiene la posición (x, y) de la pelota en la pantalla, así como la dirección (dx, dy) en la que se está moviendo.

- `Paddle`: El juego tiene dos paletas, cada una controlada por un jugador diferente. Estas paletas se representan mediante la estructura Paddle. Cada paleta tiene una posición (x, y) en la pantalla y una puntuación score.

### Funciones de juego:

- `check_game_end()`: Verifica si un jugador ha alcanzado la puntuación ganadora y reinicia el juego si es necesario.
- `update()`: Actualiza las posiciones de los objetos en el juego, maneja las colisiones y verifica si un jugador ha marcado un punto.
- `game_loop()`: Es el bucle principal del juego que continúa actualizando el juego y transmitiendo el estado del juego a los clientes.
- `broadcast_game_state()`: Transmite el estado actual del juego a ambos clientes.

### Funciones del servidor:

- `handle_client()`: Esta función maneja la interacción con un cliente individual. Cada cliente se ejecuta en un hilo separado.
- `main()`: Establece el servidor, espera conexiones de los clientes y crea un hilo por cliente. También inicia el hilo del bucle principal del juego.

### Protocolo de comunicación:

- Los clientes envían comandos como `ACTION:UP`, `ACTION:DOWN`, etc., que indican las acciones que los jugadores desean realizar.
- El servidor transmite el estado del juego usando un formato específico que incluye la posición de los paddles, la posición y dirección del balón, la puntuación y los apodos de los jugadores.

### Registro de eventos:

- Los eventos importantes, como las colisiones del balón, las puntuaciones y los movimientos de los jugadores, se registran en un archivo de registro.

## **Conclusiones**

El desarrollo de este servidor requirió un conocimiento profundo de la programación de sockets, así como del funcionamiento interno de un juego en tiempo real. El uso de subprocesos permite que el servidor maneje múltiples conexiones de manera eficiente, lo cual es crucial para mantener una experiencia de juego fluida para los jugadores.

El juego, aunque sea un poco simple, presenta muchos de los desafíos fundamentales de la programación de juegos en línea, como sincronizar estados entre clientes, administrar conexiones entrantes y salientes y actualizar y transmitir de manera eficiente el estado del juego.

El mayor desafío al desarrollar este servidor fue garantizar que los movimientos y acciones de un jugador se reflejaran en tiempo real en el juego del otro jugador. Cualquier retraso o desincronización habría afectado el juego. Además, una gestión adecuada de registros y errores es esencial para diagnosticar problemas y garantizar un juego ininterrumpido.

En general, desarrollar este servidor fue una experiencia valiosa que demostró complejidad, El servidor resultante es robusto, eficiente y proporciona una base sólida sobre la cual construir y expandirse en el futuro.
