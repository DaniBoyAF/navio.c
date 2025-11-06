#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#define WIDTH 800
#define HEIGHT 450
#define PLAYER_SIZE 32
#define MAX_HP 3
#define MAX_HP_BOSS 500

typedef struct {
    Vector2 pos;
    char nome[50];
    int score;
    int hp;
    int X;
    int y;
    double tempo;
}Player;
typedef struct {
    Vector2 pos;
    int X;
    int Y;
    int hp;
    int speed;
}inimigo;
typedef struct{
    Vector2 pos;
    int X;
    int Y;
    int speed;
    int hp;
}Boss;
void mover_inimigo(Inimigo*inimigo, Player* player){
    float dx=player.pos.x-inimigo->pos.x;
    float dy = player.pos.y - inimigo->pos.y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist > 0) {
        inimigo->pos.x += (dx / dist) * inimigo->speed; // Normaliza direção
        inimigo->pos.y += (dy / dist) * inimigo->speed;
    }
}
void mover_Boss(Boss*boss, Player*player){
    float dx=player.pos.x-Boss->pos.x;
    float dy = player.pos.y - Boss->pos.y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist > 0) {
        Boss->pos.x += (dx / dist) * Boss->speed; // Normaliza direção
        Boss->pos.y += (dy / dist) * Boss->speed;
    }
}

bool ver_batida(Player *p, Vector2 posbot,int largurabot,int alturabot){
    return(p->pos.x<posbot.x +largurabot && p->pos.x + PLAYER_SIZE > posbot.x &&
p->pos.y < posbot.y + alturabot &&p->pos.y + PLAYER_SIZE > posbot.y);
}
int main{
    InitWindow(WIDTH,HEIGTH,"Navio C");
    SettragetFPS(60);
    srand(time(NULL));

    Player player = {{WIDTH / 2, HEIGHT / 2}, "Jogador", 0, MAX_HP, 0, 0, 0};
    int totalInimigos = 3;
    Inimigo inimigos[totalInimigos];
    for (int i = 0; i < totalInimigos; i++) {
        inimigos[i].pos = (Vector2){rand() % WIDTH, rand() % HEIGHT};
        inimigos[i].speed = 2;
        inimigos[i].hp = 3;
    }

    // Inicializa Boss
    Boss boss = {{rand() % WIDTH, rand() % HEIGHT}, 0, 0, 1, MAX_HP_BOSS};

    while (!WindowShouldClose()) {
        // Movimento do player
        if (IsKeyDown(KEY_W)) player.pos.y -= 4;  // Cima
        if (IsKeyDown(KEY_S)) player.pos.y += 4;  // Baixo
        if (IsKeyDown(KEY_A)) player.pos.x -= 4;  // Esquerda
        if (IsKeyDown(KEY_D)) player.pos.x += 4;  // Direita

        // Impede sair da tela
        if (player.pos.x < 0) player.pos.x = 0;
        if (player.pos.x > WIDTH - PLAYER_SIZE) player.pos.x = WIDTH - PLAYER_SIZE;
        if (player.pos.y < 0) player.pos.y = 0;
        if (player.pos.y > HEIGHT - PLAYER_SIZE) player.pos.y = HEIGHT - PLAYER_SIZE;

        // Move inimigos e verifica colisão
        for (int i = 0; i < totalInimigos; i++) {
            mover_inimigo(&inimigos[i], &player);
            if (ver_batida(&player, inimigos[i].pos, PLAYER_SIZE, PLAYER_SIZE)) {
                player.hp--;
                inimigos[i].pos = (Vector2){rand() % WIDTH, rand() % HEIGHT}; // teleporta o inimigo
            }
        }

        // Move boss e verifica colisão
        mover_Boss(&boss, &player);
        if (ver_batida(&player, boss.pos, PLAYER_SIZE * 2, PLAYER_SIZE * 2)) {
            player.hp -= 2;
        }

        // Desenho
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText("Use W, A, S, D para mover", 10, 10, 20, GRAY);
        DrawText(TextFormat("HP: %d", player.hp), 10, 40, 20, RED);

        // Player
        DrawRectangleV(player.pos, (Vector2){PLAYER_SIZE, PLAYER_SIZE}, BLUE);

        // Inimigos
        for (int i = 0; i < totalInimigos; i++) {
            DrawRectangleV(inimigos[i].pos, (Vector2){PLAYER_SIZE, PLAYER_SIZE}, RED);
        }

        // Boss
        DrawRectangleV(boss.pos, (Vector2){PLAYER_SIZE * 2, PLAYER_SIZE * 2}, PURPLE);

        if (player.hp <= 0)
            DrawText("GAME OVER!", WIDTH / 2 - 100, HEIGHT / 2, 40, BLACK);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
