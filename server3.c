#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define BALL_RADIUS 50
#define PADDLE_WIDTH 10
#define PADDLE_HEIGHT 100
#define PADDLE_SPEED 10
#define BALL_SPEED_X 5
#define BALL_SPEED_Y 5
#define WINNING_SCORE 5

#define MAX_CLIENTS 2

FILE *logfile;

void broadcast_game_state();

void log_event(const char *event);

void log_event(const char *event)
{
    fprintf(logfile, "%s\n", event);
    fflush(logfile);
}

int client_sockets[MAX_CLIENTS] = {0};
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t game_start_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t game_start_mutex = PTHREAD_MUTEX_INITIALIZER;

int player1_in_use = 0;
int player2_in_use = 0;
int clients_ready = 0;
char player1_nickname[50] = "";
char player2_nickname[50] = "";

typedef struct
{
    float x, y, dx, dy;
} Ball;

typedef struct
{
    float x, y;
    int score;
} Paddle;

Ball ball = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, -BALL_SPEED_X, BALL_SPEED_Y * 0.3};
Paddle paddle1 = {0, SCREEN_HEIGHT / 2 - PADDLE_HEIGHT / 2, 0};
Paddle paddle2 = {SCREEN_WIDTH - PADDLE_WIDTH, SCREEN_HEIGHT / 2 - PADDLE_HEIGHT / 2, 0};

void check_game_end()
{
    if (paddle1.score >= WINNING_SCORE || paddle2.score >= WINNING_SCORE)
    {
        char buffer[1024];
        sprintf(buffer, "GAME:OVER");
        log_event("Game over");

        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (client_sockets[i] != 0)
            {
                send(client_sockets[i], buffer, strlen(buffer), 0);
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        // Reset game state
        paddle1.score = 0;
        paddle2.score = 0;
        ball.x = SCREEN_WIDTH / 2;
        ball.y = SCREEN_HEIGHT / 2;
        ball.dx = -BALL_SPEED_X;
        ball.dy = BALL_SPEED_Y * 0.3;
    }
}

void update()
{

    ball.x += ball.dx;
    ball.y += ball.dy;

    // Collision with top and bottom
    if (ball.y <= 0 || ball.y >= SCREEN_HEIGHT)
    {
        log_event("Ball collided with top or bottom.");

        ball.dy = -ball.dy;
    }

    // Collision with paddles
    if ((ball.dx < 0 && ball.x <= paddle1.x + PADDLE_WIDTH && ball.y >= paddle1.y && ball.y <= paddle1.y + PADDLE_HEIGHT) ||
        (ball.dx > 0 && ball.x >= paddle2.x && ball.y >= paddle2.y && ball.y <= paddle2.y + PADDLE_HEIGHT))
    {
        log_event("Ball collided with a paddle.");

        ball.dx = -ball.dx;
    }

    // Ball goes out of bounds
    if (ball.x < 0)
    {
        log_event("Ball went out of bounds on the left.");

        paddle2.score++;
        ball.x = SCREEN_WIDTH / 2;
        ball.y = SCREEN_HEIGHT / 2;
        ball.dx = 1;
        check_game_end();
    }
    else if (ball.x > SCREEN_WIDTH)
    {
        log_event("Ball went out of bounds on the right.");

        paddle1.score++;
        ball.x = SCREEN_WIDTH / 2;
        ball.y = SCREEN_HEIGHT / 2;
        ball.dx = -1;
        check_game_end();
    }
}

void *game_loop(void *arg)
{
    pthread_mutex_lock(&game_start_mutex);
    while (clients_ready < MAX_CLIENTS)
    {
        pthread_cond_wait(&game_start_cond, &game_start_mutex);
    }
    pthread_mutex_unlock(&game_start_mutex);
    sleep(2);
    log_event("Game loop started.");

    while (1)
    {
        update();
        broadcast_game_state();
        usleep(10000);
    }
    log_event("Game loop ended.");

    return NULL;
}

void broadcast_game_state()
{
    char buffer[1024];
    sprintf(buffer, "STATE:P1:%f,P2:%f,BALL:%f:%f,SCORE:P1:%d:P2:%d,NICKS:%s:%s",
            paddle1.y, paddle2.y, ball.x, ball.y, paddle1.score, paddle2.score, player1_nickname, player2_nickname);

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (client_sockets[i] != 0)
        {
            send(client_sockets[i], buffer, strlen(buffer), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *socket_desc)
{
    int newSocket = *(int *)socket_desc;
    char buffer[1024];
    char playerID[10] = {0};

    // Receive playerID from the client
    memset(playerID, 0, sizeof(playerID));
    int bytes_received = recv(newSocket, playerID, sizeof(playerID) - 1, 0);
    if (bytes_received <= 0)
    {
        printf("Failed to identify player or player disconnected.\n");
        close(newSocket);
        pthread_exit(NULL);
    }

    // Check which player the client wants to be and if that player is available
    if (strcmp(playerID, "PLAYER1") == 0)
    {
        if (player1_in_use)
        {
            send(newSocket, "Player in use", 14, 0);
            close(newSocket);
            pthread_exit(NULL);
        }
        else
        {
            player1_in_use = 1;
            send(newSocket, "Player accepted", 16, 0);
        }
    }
    else if (strcmp(playerID, "PLAYER2") == 0)
    {
        if (player2_in_use)
        {
            send(newSocket, "Player in use", 14, 0);
            close(newSocket);
            pthread_exit(NULL);
        }
        else
        {
            player2_in_use = 1;
            send(newSocket, "Player accepted", 16, 0);
        }
    }

    // Wait for client's ready signal
    memset(buffer, 0, sizeof(buffer));
    if (recv(newSocket, buffer, 1024, 0) <= 0)
    {
        printf("Client disconnected or error occurred.\n");
        close(newSocket);
        pthread_exit(NULL);
    }

    if (strcmp(buffer, "READY") == 0)
    {
        clients_ready++;
        if (clients_ready == MAX_CLIENTS)
        {
            pthread_mutex_lock(&game_start_mutex);
            pthread_cond_signal(&game_start_cond);
            pthread_mutex_unlock(&game_start_mutex);
        }

        // If only one client is ready, send waiting message
        while (clients_ready < MAX_CLIENTS)
        {
            send(newSocket, "WAITING", 8, 0);
            usleep(500000); // Half a second delay to avoid flooding
        }
    }

    // Add the client to the client_sockets array
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (client_sockets[i] == 0)
        {
            client_sockets[i] = newSocket;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    printf("Connected: %s\n", playerID);
    char log_message[2048];
    sprintf(log_message, "Client connected as %s", playerID);
    log_event(log_message);

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));

        // Handle client actions
        if (recv(newSocket, buffer, 1024, 0) <= 0)
        {
            printf("Client disconnected or error occurred.\n");
            sprintf(log_message, "Client %s disconnected or error occurred.", playerID);
            log_event(log_message);
            break;
        }

        sprintf(log_message, "Received command from %s: %s", playerID, buffer);
        log_event(log_message);

        if (strcmp(buffer, "ACTION:UP") == 0)
        {
            if (strcmp(playerID, "PLAYER1") == 0 && paddle1.y > 0)
            {
                paddle1.y -= PADDLE_SPEED;
            }
            else if (strcmp(playerID, "PLAYER2") == 0 && paddle2.y > 0)
            {
                paddle2.y -= PADDLE_SPEED;
            }
        }
        else if (strcmp(buffer, "ACTION:DOWN") == 0)
        {
            if (strcmp(playerID, "PLAYER1") == 0 && paddle1.y + PADDLE_HEIGHT < SCREEN_HEIGHT)
            {
                paddle1.y += PADDLE_SPEED;
            }
            else if (strcmp(playerID, "PLAYER2") == 0 && paddle2.y + PADDLE_HEIGHT < SCREEN_HEIGHT)
            {
                paddle2.y += PADDLE_SPEED;
            }
        }
    }

    // Remove the client from the client_sockets array before exiting
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (client_sockets[i] == newSocket)
        {
            client_sockets[i] = 0;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    if (strcmp(playerID, "PLAYER1") == 0)
    {
        player1_in_use = 0;
    }
    else if (strcmp(playerID, "PLAYER2") == 0)
    {
        player2_in_use = 0;
    }

    close(newSocket);
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <PORT> <Log File>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    char *logFilePath = argv[2];

    logfile = fopen(logFilePath, "w");
    if (!logfile)
    {
        perror("Could not open log file");
        return 1;
    }

    int serverSocket, newSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size;
    char buffer[1024];

    serverSocket = socket(PF_INET, SOCK_STREAM, 0);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

    if (listen(serverSocket, 5) == 0)
        printf("Waiting for players...\n");
    else
        printf("Error\n");

    pthread_t game_thread;
    if (pthread_create(&game_thread, NULL, game_loop, NULL) < 0)
    {
        perror("Could not create game loop thread.");
        return 1;
    }
    pthread_detach(game_thread);

    while (1)
    {
        addr_size = sizeof serverStorage;
        newSocket = accept(serverSocket, (struct sockaddr *)&serverStorage, &addr_size);
        printf("Connection accepted from a client.\n");

        memset(buffer, 0, sizeof(buffer));
        if (recv(newSocket, buffer, 1024, 0) <= 0)
        {
            printf("Failed to receive client info or client disconnected.\n");
            close(newSocket);
            pthread_exit(NULL);
        }

        char command[15]; // To hold the initial command (e.g., "REGISTRATION")
        char receivedNickname[50];
        char receivedEmail[100];

        sscanf(buffer, "%14[^:]:NICKNAME:%49[^:]:EMAIL:%99s", command, receivedNickname, receivedEmail);

        if (strcmp(command, "REGISTRATION") == 0)
        {
            printf("Received Nickname: %s\n", receivedNickname);
            printf("Received Email: %s\n", receivedEmail);
            if (!player1_in_use)
            {
                strcpy(player1_nickname, receivedNickname);
            }
            else if (!player2_in_use)
            {
                strcpy(player2_nickname, receivedNickname);
            }
        }

        pthread_t client_thread;
        int *new_sock = malloc(sizeof(int));
        if (new_sock == NULL)
        {
            perror("Failed to allocate memory.");
            return 1;
        }
        *new_sock = newSocket;

        if (pthread_create(&client_thread, NULL, handle_client, new_sock) < 0)
        {
            perror("Could not create thread.");
            return 1;
        }

        pthread_detach(client_thread);
    }

    close(serverSocket);
    fclose(logfile);

    return 0;
}