#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define WIDTH 800
#define HEIGHT 450
#define PLAYER_SIZE 32
#define MAX_HP 20
#define MAX_HP_BOSS 500
#define MAX_BULLETS 30
#define BULLET_SPEED 8

typedef struct {
    Vector2 pos;
    char nome[50];
    int score;
    int hp;
    int municao;
    int municao_total;
    int tipo_muni;
} Player;

typedef struct {
    Vector2 pos;
    int hp;
    int speed;
    bool alive;
} Inimigo;

typedef struct {
    Vector2 pos;
    int speed;
    int hp;
} Boss;

typedef struct {
    Vector2 pos;
    Vector2 dir;
    bool active;
    int dano;
} Bullet;

// ─────────── Movimentação dos inimigos ───────────
void mover_inimigo(Inimigo* ini, Player* player) {
    if (!ini->alive) return;
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

// ─────────── Colisão ───────────
bool ver_batida(Vector2 aPos, int aLarg, int aAlt, Vector2 bPos, int bLarg, int bAlt) {
    return (aPos.x < bPos.x + bLarg &&
            aPos.x + aLarg > bPos.x &&
            aPos.y < bPos.y + bAlt &&
            aPos.y + aAlt > bPos.y);
}

// ─────────── MAIN ───────────
int main(void) {
    InitWindow(WIDTH, HEIGHT, "Navio C");
    SetTargetFPS(60);
    srand(time(NULL));

    Player player = { (Vector2){WIDTH/2, HEIGHT/2}, "Player", 0, MAX_HP, 10, 50, 0 };

    int totalInimigos = 5;
    Inimigo inimigos[5];
    for (int i = 0; i < totalInimigos; i++) {
        inimigos[i].pos = (Vector2){ rand() % WIDTH, rand() % HEIGHT };
        inimigos[i].speed = 2;
        inimigos[i].hp = 3;
        inimigos[i].alive = true;
    }
    
    Boss boss = { (Vector2){100, 100}, 1, MAX_HP_BOSS };

    Bullet balas[MAX_BULLETS] = {0};

    while (!WindowShouldClose()) {

        // ───────── Movimentação do player ─────────
        if (IsKeyDown(KEY_W)) player.pos.y -= 3;
        if (IsKeyDown(KEY_S)) player.pos.y += 3;
        if (IsKeyDown(KEY_A)) player.pos.x -= 3;
        if (IsKeyDown(KEY_D)) player.pos.x += 3;

        // ───────── Troca de arma ─────────
        if (IsKeyPressed(KEY_ONE)) player.tipo_muni = 0;
        if (IsKeyPressed(KEY_TWO)) player.tipo_muni = 1;
        if (IsKeyPressed(KEY_THREE)) player.tipo_muni = 2;

        int dano_arma[] = {25, 10, 50};
        int dano = dano_arma[player.tipo_muni];

        // ───────── Movimento e colisão dos inimigos ─────────
        for (int i = 0; i < totalInimigos; i++) {
            mover_inimigo(&inimigos[i], &player);
            if (inimigos[i].alive &&
                ver_batida(player.pos, PLAYER_SIZE, PLAYER_SIZE, inimigos[i].pos, PLAYER_SIZE, PLAYER_SIZE)) {
                player.hp--;
                inimigos[i].pos = (Vector2){ rand() % WIDTH, rand() % HEIGHT };
            }
        }
        
        // ───────── Movimento e colisão do boss ─────────
        mover_Boss(&boss, &player);
        if (ver_batida(player.pos, PLAYER_SIZE, PLAYER_SIZE, boss.pos, PLAYER_SIZE*2, PLAYER_SIZE*2)) {
            player.hp -= 3;
            boss.pos = (Vector2){ rand() % WIDTH, rand() % HEIGHT };
        }

        // ───────── Disparo ─────────
        if (IsKeyPressed(KEY_SPACE) && player.municao > 0) {
            player.municao--;
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!balas[i].active) {
                    balas[i].active = true;
                    balas[i].pos = (Vector2){player.pos.x + PLAYER_SIZE/2, player.pos.y + PLAYER_SIZE/2};
                    Vector2 mouse = GetMousePosition();
                    Vector2 dir = { mouse.x - player.pos.x, mouse.y - player.pos.y };
                    float len = sqrtf(dir.x*dir.x + dir.y*dir.y);
                    if (len > 0) { dir.x /= len; dir.y /= len; }
                    balas[i].dir = dir;
                    balas[i].dano = dano;
                    break;
                }
            }
        }

        // ───────── Atualiza balas ─────────
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (balas[i].active) {
                balas[i].pos.x += balas[i].dir.x * BULLET_SPEED;
                balas[i].pos.y += balas[i].dir.y * BULLET_SPEED;

                // Saiu da tela?
                if (balas[i].pos.x < 0 || balas[i].pos.x > WIDTH || 
                    balas[i].pos.y < 0 || balas[i].pos.y > HEIGHT) {
                    balas[i].active = false;
                }

                // Colidiu com inimigos
                for (int j = 0; j < totalInimigos; j++) {
                    if (inimigos[j].alive &&
                        ver_batida(balas[i].pos, 8, 8, inimigos[j].pos, PLAYER_SIZE, PLAYER_SIZE)) {
                        inimigos[j].hp -= balas[i].dano;
                        balas[i].active = false;
                        if (inimigos[j].hp <= 0) {
                            inimigos[j].alive = false;
                            player.score += 10;
                        }
                    }
                }

                // Colidiu com boss
                if (ver_batida(balas[i].pos, 8, 8, boss.pos, PLAYER_SIZE*2, PLAYER_SIZE*2)) {
                    boss.hp -= balas[i].dano;
                    balas[i].active = false;
                    if (boss.hp <= 0) {
                        boss.hp = 0;
                        player.score += 100;
                    }
                }
            }
        }

        // ───────── Desenho ─────────
        BeginDrawing();
        ClearBackground(BLUE);

        // Player
        DrawRectangle(player.pos.x, player.pos.y, PLAYER_SIZE, PLAYER_SIZE, BLACK);

        // Inimigos
        for (int i = 0; i < totalInimigos; i++)
            if (inimigos[i].alive)
                DrawRectangle(inimigos[i].pos.x, inimigos[i].pos.y, PLAYER_SIZE, PLAYER_SIZE, RED);

        // Boss
        DrawRectangle(boss.pos.x, boss.pos.y, PLAYER_SIZE * 2, PLAYER_SIZE * 2, PURPLE);

        // Balas
        for (int i = 0; i < MAX_BULLETS; i++)
            if (balas[i].active)
                DrawCircle(balas[i].pos.x, balas[i].pos.y, 5, YELLOW);

        // HUD
        DrawText(TextFormat("HP: %d", player.hp), 10, 10, 20, BLACK);
        DrawText(TextFormat("Score: %d", player.score), 10, 40, 20, WHITE);
        DrawText(TextFormat("Munição: %d / %d", player.municao, player.municao_total), 10, 70, 20, YELLOW);
        DrawText(TextFormat("Boss HP: %d", boss.hp), 10, 100, 20, PURPLE);

        const char *tipos[] = {"Bola de Aço", "Incendiária", "Quebra Joelho"};
        DrawText(TextFormat("Arma: %s", tipos[player.tipo_muni]), 10, 130, 20, GREEN);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}