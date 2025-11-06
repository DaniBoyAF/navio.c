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
} Player;

typedef struct {
    Vector2 pos;
    int hp;
    int speed;
} inimigo;

typedef struct {
    Vector2 pos;
    int speed;
    int hp;
} Boss;

void mover_inimigo(inimigo* ini, Player* player) {
    float dx = player->pos.x - ini->pos.x;
    float dy = player->pos.y - ini->pos.y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist > 0) {
        ini->pos.x += (dx / dist) * ini->speed;
        ini->pos.y += (dy / dist) * ini->speed;
    }
}

void mover_Boss(Boss* boss, Player* player) {
    float dx = player->pos.x - boss->pos.x;
    float dy = player->pos.y - boss->pos.y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist > 0) {
        boss->pos.x += (dx / dist) * boss->speed;
        boss->pos.y += (dy / dist) * boss->speed;
    }
}

bool ver_batida(Player *p, Vector2 posbot, int largurabot, int alturabot) {
    return (p->pos.x < posbot.x + largurabot &&
            p->pos.x + PLAYER_SIZE > posbot.x &&
            p->pos.y < posbot.y + alturabot &&
            p->pos.y + PLAYER_SIZE > posbot.y);
}

int main() {
    InitWindow(WIDTH, HEIGHT, "Navio C");
    SetTargetFPS(60);

    Player player = { (Vector2){WIDTH/2, HEIGHT/2}, "Player", 0, MAX_HP };

    int totalInimigos = 5;
    inimigo inimigos[5];
    for (int i = 0; i < totalInimigos; i++) {
        inimigos[i].pos = (Vector2){ rand() % WIDTH, rand() % HEIGHT };
        inimigos[i].speed = 2;
        inimigos[i].hp = 3;
    }
    
    Boss boss = { (Vector2){100, 100}, 1, MAX_HP_BOSS };

    while (!WindowShouldClose()) {
        // Movimento do jogador
        if (IsKeyDown(KEY_W)) player.pos.y -= 3;
        if (IsKeyDown(KEY_S)) player.pos.y += 3;
        if (IsKeyDown(KEY_A)) player.pos.x -= 3;
        if (IsKeyDown(KEY_D)) player.pos.x += 3;

        // Movimento dos inimigos e boss
        for (int i = 0; i < totalInimigos; i++) {
            mover_inimigo(&inimigos[i], &player);
            if (ver_batida(&player, inimigos[i].pos, PLAYER_SIZE, PLAYER_SIZE)) {
                player.hp--;
                inimigos[i].pos = (Vector2){ rand() % WIDTH, rand() % HEIGHT };
            }
        }
        
           mover_Boss(&boss, &player);
            if (ver_batida(&player, boss.pos, PLAYER_SIZE*2, PLAYER_SIZE*2)) {
                player.hp--;
                boss.pos = (Vector2){ rand() % WIDTH, rand() % HEIGHT };
            }
        
        

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawRectangle(player.pos.x, player.pos.y, PLAYER_SIZE, PLAYER_SIZE, BLUE);

        for (int i = 0; i < totalInimigos; i++)
            DrawRectangle(inimigos[i].pos.x, inimigos[i].pos.y, PLAYER_SIZE, PLAYER_SIZE, RED);

        DrawRectangle(boss.pos.x, boss.pos.y, PLAYER_SIZE * 2, PLAYER_SIZE * 2, PURPLE);

        DrawText(TextFormat("HP: %d", player.hp), 10, 10, 20, BLACK);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
