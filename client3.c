#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define BALL_RADIUS 10
#define PADDLE_WIDTH 15
#define PADDLE_HEIGHT 100

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

char player1_nickname[50] = "Player 1";
char player2_nickname[50] = "Player 2";

int initSDL()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("Pong", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL)
    {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL)
    {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() == -1)
    {
        printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
        return 1;
    }

    return 0;
}

void renderScore(int p1_score, int p2_score)
{

    TTF_Font *font = TTF_OpenFont("/Library/Fonts/Arial Unicode.ttf", 24);
    if (font == NULL)
    {
        printf("Failed to load font! SDL_ttf Error: %s\n", TTF_GetError());
        return;
    }

    char scoreText[200];
    sprintf(scoreText, "%s: %d - %s: %d", player1_nickname, p1_score, player2_nickname, p2_score);

    SDL_Color textColor = {255, 255, 255, 255}; // White color
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, scoreText, textColor);
    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

    int textWidth = textSurface->w;
    int textHeight = textSurface->h;

    SDL_Rect renderQuad = {(WINDOW_WIDTH - textWidth) / 2, 10, textWidth, textHeight}; // Position it at the top center
    SDL_RenderCopy(renderer, textTexture, NULL, &renderQuad);

    // Cleanup
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
    TTF_CloseFont(font);
}

void renderGame(float p1_y, float p2_y, float ball_x, float ball_y, int p1_score, int p2_score)
{

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Set background color to black
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Set paddle and ball color to white

    SDL_Rect paddle1Rect = {0, (int)p1_y, PADDLE_WIDTH, PADDLE_HEIGHT};
    SDL_Rect paddle2Rect = {WINDOW_WIDTH - PADDLE_WIDTH, (int)p2_y, PADDLE_WIDTH, PADDLE_HEIGHT};
    SDL_RenderFillRect(renderer, &paddle1Rect);
    SDL_RenderFillRect(renderer, &paddle2Rect);

    SDL_Rect ballRect = {(int)ball_x - BALL_RADIUS, (int)ball_y - BALL_RADIUS, BALL_RADIUS * 2, BALL_RADIUS * 2};
    SDL_RenderFillRect(renderer, &ballRect);

    renderScore(p1_score, p2_score);

    SDL_RenderPresent(renderer);
}

void renderGameOver(int p1_score, int p2_score)
{
    TTF_Font *font = TTF_OpenFont("/Library/Fonts/Arial Unicode.ttf", 32);
    if (font == NULL)
    {
        printf("Failed to load font! SDL_ttf Error: %s\n", TTF_GetError());
        return;
    }

    char gameOverText[] = "Game Over";
    SDL_Color textColor = {255, 0, 0, 255}; // Red color
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, gameOverText, textColor);
    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

    int textWidth = textSurface->w;
    int textHeight = textSurface->h;

    SDL_Rect renderQuad;
    renderQuad.x = (WINDOW_WIDTH - textWidth) / 2;
    renderQuad.y = WINDOW_HEIGHT / 2 - textHeight;
    renderQuad.w = textWidth;
    renderQuad.h = textHeight;
    SDL_RenderCopy(renderer, textTexture, NULL, &renderQuad);

    char scoreText[200];
    sprintf(scoreText, "Score - %s: %d, %s: %d", player1_nickname, p1_score, player2_nickname, p2_score);
    textSurface = TTF_RenderText_Solid(font, scoreText, textColor);
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

    textWidth = textSurface->w;
    textHeight = textSurface->h;

    renderQuad.x = (WINDOW_WIDTH - textWidth) / 2;
    renderQuad.y = WINDOW_HEIGHT / 2;
    renderQuad.w = textWidth;
    renderQuad.h = textHeight;
    SDL_RenderCopy(renderer, textTexture, NULL, &renderQuad);

    SDL_RenderPresent(renderer);

    // Cleanup
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
    TTF_CloseFont(font);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <SERVER_IP> <PORT>\n", argv[0]);
        return 1;
    }

    char *serverIP = argv[1];
    int port = atoi(argv[2]);

    if (initSDL() != 0)
    {
        return 1; // Exit if SDL initialization fails
    }

    int clientSocket;
    char buffer[1024];
    struct sockaddr_in serverAddr;
    char playerID[10] = {0};

    clientSocket = socket(PF_INET, SOCK_STREAM, 0);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(serverIP);
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    printf("Connected to the server.\n");

    char nickname[50];
    char email[100];

    printf("Enter your nickname: ");
    scanf("%49s", nickname); // Limit input to prevent buffer overflow
    getchar();               // Consume newline

    printf("Enter your email: ");
    scanf("%99s", email);
    getchar(); // Consume newline

    sprintf(buffer, "REGISTRATION:NICKNAME:%s:EMAIL:%s", nickname, email);
    send(clientSocket, buffer, strlen(buffer) + 1, 0);

    int choice = 0;
    printf("\nChoose your player:\n");
    printf("1. Player1\n");
    printf("2. Player2\n");
    printf("Choice: ");
    scanf("%d", &choice);
    getchar(); // consume newline

    if (choice == 1)
    {
        strcpy(playerID, "PLAYER1");
        strcpy(player1_nickname, nickname);
    }
    else if (choice == 2)
    {
        strcpy(playerID, "PLAYER2");
        strcpy(player2_nickname, nickname);
    }
    else
    {
        printf("Invalid choice.\n");
        close(clientSocket);
        SDL_DestroyWindow(window);
        SDL_DestroyRenderer(renderer);
        SDL_Quit();
        return 1;
    }

    send(clientSocket, playerID, strlen(playerID) + 1, 0);

    // Receive acknowledgment or rejection from the server
    memset(buffer, 0, sizeof(buffer));
    if (recv(clientSocket, buffer, 1024, 0) <= 0)
    {
        printf("Error receiving server response or server closed connection.\n");
        close(clientSocket);
        SDL_DestroyWindow(window);
        SDL_DestroyRenderer(renderer);
        SDL_Quit();
        return 1;
    }
    printf("Server Response: %s\n", buffer);

    if (strcmp(buffer, "Player in use") == 0)
    {
        printf("%s already in use. Please select a different player or restart the client.\n", playerID);
        close(clientSocket);
        SDL_DestroyWindow(window);
        SDL_DestroyRenderer(renderer);
        SDL_Quit();
        return 1;
    }

    // Inform server that client is ready
    send(clientSocket, "READY", 6, 0);
    printf("Waiting for other player...\n");

    // Wait until both players are ready
    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        if (recv(clientSocket, buffer, 1024, 0) <= 0)
        {
            printf("Error receiving server response or server closed connection.\n");
            close(clientSocket);
            SDL_DestroyWindow(window);
            SDL_DestroyRenderer(renderer);
            SDL_Quit();
            return 1;
        }

        if (strcmp(buffer, "WAITING") == 0)
        {
            continue; // Continue waiting
        }
        else
        {
            break; // Exit loop and start game
        }
    }

    float p1_y, p2_y, ball_x, ball_y; // Declare here, once for the entire while loop
    int p1_score = 0, p2_score = 0;

    while (1)
    {
        // Continuously check for updates from the server
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(clientSocket, buffer, 1024, 0);
        if (bytes_received <= 0)
        {
            printf("Error receiving game state or server closed connection.\n");
            break;
        }

        // Check for game over
        if (strcmp(buffer, "GAME:OVER") == 0)
        {
            printf("Game over!\n");
            renderGameOver(p1_score, p2_score);
            SDL_Delay(5000); // Wait for 5 seconds
            continue;        // Continue the game loop
        }

        int p1_score, p2_score;
        sscanf(buffer, "STATE:P1:%f,P2:%f,BALL:%f:%f,SCORE:P1:%d:P2:%d,NICKS:%49[^:]:%49s", &p1_y, &p2_y, &ball_x, &ball_y, &p1_score, &p2_score, player1_nickname, player2_nickname);
        renderGame(p1_y, p2_y, ball_x, ball_y, p1_score, p2_score);
        printf("Scores - Player1: %d, Player2: %d\n", p1_score, p2_score); // Display scores in the terminal

        // Handle SDL events (for actions and window close)
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_QUIT)
            {
                close(clientSocket);
                SDL_DestroyWindow(window);
                SDL_DestroyRenderer(renderer);
                SDL_Quit();
                return 0;
            }
            else if (e.type == SDL_KEYDOWN)
            {
                switch (e.key.keysym.sym)
                {
                case SDLK_UP:
                    strcpy(buffer, "ACTION:UP");
                    send(clientSocket, buffer, strlen(buffer), 0);
                    break;
                case SDLK_DOWN:
                    strcpy(buffer, "ACTION:DOWN");
                    send(clientSocket, buffer, strlen(buffer), 0);
                    break;
                }
            }
        }
    }

    close(clientSocket);
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
