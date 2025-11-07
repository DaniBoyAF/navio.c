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

// ───────── LISTA DE SCORE ─────────
typedef struct ListaScore {
    int pontuacao;
    int dinheiro;
    int tempo;
    struct ListaScore *proximo;
} ListaScore;

ListaScore* criarNo(int pontuacao, int dinheiro, int tempo) {
    ListaScore* novo = malloc(sizeof(ListaScore));
    novo->pontuacao = pontuacao;
    novo->dinheiro = dinheiro;
    novo->tempo = tempo;
    novo->proximo = NULL;
    return novo;
}

void add_ordenado_score(ListaScore** inicio, int pontuacao, int dinheiro, int tempo) {
    ListaScore* novo = criarNo(pontuacao, dinheiro, tempo);

    if (*inicio == NULL || novo->pontuacao > (*inicio)->pontuacao) {
        novo->proximo = *inicio;
        *inicio = novo;
        return;
    }

    ListaScore* temp = *inicio;
    while (temp->proximo != NULL && temp->proximo->pontuacao > pontuacao) {
        temp = temp->proximo;
    }
    novo->proximo = temp->proximo;
    temp->proximo = novo;
}

void mostrarScores(ListaScore* inicio) {
    ListaScore* temp = inicio;
    printf("\n===== SCOREBOARD =====\n");
    while (temp != NULL) {
        printf("Pontuação: %d | Dinheiro: %d | Tempo: %d\n", temp->pontuacao, temp->dinheiro, temp->tempo);
        temp = temp->proximo;
    }
    printf("======================\n");
}

// ───────── STRUCTS DO JOGO ─────────
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

// ───────── FUNÇÕES DO JOGO ─────────
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

bool ver_batida(Vector2 aPos, int aLarg, int aAlt, Vector2 bPos, int bLarg, int bAlt) {
    return (aPos.x < bPos.x + bLarg &&
            aPos.x + aLarg > bPos.x &&
            aPos.y < bPos.y + bAlt &&
            aPos.y + aAlt > bPos.y);
}

// ───────── MAIN ─────────
int main(void) {
    InitWindow(WIDTH, HEIGHT, "Navio C");
    SetTargetFPS(60);
    srand(time(NULL));

    ListaScore* scoreBoard = NULL;

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

    int tempoJogo = 0;
    while (!WindowShouldClose() && player.hp > 0) {

        tempoJogo++;

        if (IsKeyDown(KEY_W)) player.pos.y -= 3;
        if (IsKeyDown(KEY_S)) player.pos.y += 3;
        if (IsKeyDown(KEY_A)) player.pos.x -= 3;
        if (IsKeyDown(KEY_D)) player.pos.x += 3;

        if (IsKeyPressed(KEY_ONE)) player.tipo_muni = 0;
        if (IsKeyPressed(KEY_TWO)) player.tipo_muni = 1;
        if (IsKeyPressed(KEY_THREE)) player.tipo_muni = 2;

        int dano_arma[] = {25, 10, 50};
        int dano = dano_arma[player.tipo_muni];

        for (int i = 0; i < totalInimigos; i++) {
            mover_inimigo(&inimigos[i], &player);
            if (inimigos[i].alive &&
                ver_batida(player.pos, PLAYER_SIZE, PLAYER_SIZE, inimigos[i].pos, PLAYER_SIZE, PLAYER_SIZE)) {
                player.hp--;
                inimigos[i].pos = (Vector2){ rand() % WIDTH, rand() % HEIGHT };
            }
        }

        mover_Boss(&boss, &player);
        if (ver_batida(player.pos, PLAYER_SIZE, PLAYER_SIZE, boss.pos, PLAYER_SIZE*2, PLAYER_SIZE*2)) {
            player.hp -= 3;
            boss.pos = (Vector2){ rand() % WIDTH, rand() % HEIGHT };
        }

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

        for (int i = 0; i < MAX_BULLETS; i++) {
            if (balas[i].active) {
                balas[i].pos.x += balas[i].dir.x * BULLET_SPEED;
                balas[i].pos.y += balas[i].dir.y * BULLET_SPEED;

                if (balas[i].pos.x < 0 || balas[i].pos.x > WIDTH ||
                    balas[i].pos.y < 0 || balas[i].pos.y > HEIGHT) {
                    balas[i].active = false;
                }

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

        BeginDrawing();
        ClearBackground(BLUE);

        DrawRectangle(player.pos.x, player.pos.y, PLAYER_SIZE, PLAYER_SIZE, BLACK);

        for (int i = 0; i < totalInimigos; i++)
            if (inimigos[i].alive)
                DrawRectangle(inimigos[i].pos.x, inimigos[i].pos.y, PLAYER_SIZE, PLAYER_SIZE, RED);

        DrawRectangle(boss.pos.x, boss.pos.y, PLAYER_SIZE * 2, PLAYER_SIZE * 2, PURPLE);

        for (int i = 0; i < MAX_BULLETS; i++)
            if (balas[i].active)
                DrawCircle(balas[i].pos.x, balas[i].pos.y, 5, YELLOW);

        DrawText(TextFormat("HP: %d", player.hp), 10, 10, 20, BLACK);
        DrawText(TextFormat("Score: %d", player.score), 10, 40, 20, WHITE);
        DrawText(TextFormat("Tempo: %d", tempoJogo/60), 10, 70, 20, GREEN);
        DrawText(TextFormat("Munição: %d / %d", player.municao, player.municao_total), 10, 100, 20, YELLOW);

        EndDrawing();
    }

    add_ordenado_score(&scoreBoard, player.score, player.municao_total, tempoJogo/60);
    mostrarScores(scoreBoard);

    CloseWindow();
    return 0;
}
