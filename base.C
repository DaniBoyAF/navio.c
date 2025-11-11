#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define WIDTH 800
#define HEIGHT 450
#define PLAYER_SIZE 1.0f
#define MAX_HP 20
#define MAX_HP_BOSS 500
#define MAX_BULLETS 30
#define BULLET_SPEED 0.5f
#define ARQUIVO_SCORES "scores.txt"

// ───────── LISTA DE SCORE ─────────
typedef struct ListaScore {
    int pontuacao;
    int dinheiro;
    int tempo;
    struct ListaScore *proximo;
} ListaScore;

ListaScore* criarNo(int pontuacao, int dinheiro, int tempo) {
    ListaScore* novo = (ListaScore*) malloc(sizeof(ListaScore));
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
void salvarScores(ListaScore* inicio){
    FILE* arquivo = fopen(ARQUIVO_SCORES,"w");
    if (arquivo == NULL){
    printf("erro ao abrir o arquivo de scores\n");
    return;
    }
    ListaScore* temp = inicio;
    while (temp != NULL){
        fprintf(arquivo, "%d %d %d\n", temp->pontuacao, temp->dinheiro, temp->tempo);
        temp = temp->proximo;
    }
    fclose(arquivo);    
    printf("Salvo com sucesso!\n");
}

// Mostra no terminal (opcional)
void mostrarScores(ListaScore* inicio) {
    ListaScore* temp = inicio;
    printf("\n===== SCOREBOARD =====\n");
    while (temp != NULL) {
        printf("Pontuação: %d | Dinheiro: %d | Tempo: %d\n", temp->pontuacao, temp->dinheiro, temp->tempo);
        temp = temp->proximo;
    }
    printf("======================\n");
}
void carregarScores(ListaScore** inicio) {
    FILE* arquivo = fopen(ARQUIVO_SCORES, "r");
    if (arquivo == NULL) {
        printf("Nenhum arquivo de scores encontrado. Iniciando novo scoreboard.\n");
        return;
    }
    int pontuacao, dinheiro, tempo;
    while (fscanf(arquivo, "%d %d %d", &pontuacao, &dinheiro, &tempo) != EOF) {
        add_ordenado_score(inicio, pontuacao, dinheiro, tempo);
    }
    fclose(arquivo);
}
void liberarScores(ListaScore** inicio){
    ListaScore* temp;
    while (*inicio != NULL){
        temp = *inicio;
        *inicio = (*inicio)->proximo;
        free(temp);
    }
}
// Mostra na tela Raylib
void mostrarScoresTela(ListaScore* inicio) {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawText("===== SCOREBOARD =====", 220, 40, 30, BLACK);
    int y = 100;
    ListaScore* temp = inicio;
    int count = 0;
    while (temp != NULL && count < 10) {
        DrawText(TextFormat("%d) Pontos: %d | Dinheiro: %d | Tempo: %d",
                            count + 1, temp->pontuacao, temp->dinheiro, temp->tempo),
                 150, y, 20, DARKGRAY);
        y += 30;
        temp = temp->proximo;
        count++;
    }
    DrawText("Pressione qualquer tecla para voltar", 200, HEIGHT - 60, 20, RED);
    EndDrawing();
    while (!WindowShouldClose() && !IsKeyPressed(KEY_ENTER) && !IsKeyPressed(KEY_SPACE) && !IsKeyPressed(KEY_ESCAPE))
        ; // espera tecla
}

// ───────── STRUCTS 3D ─────────
typedef struct {
    Vector3 pos;
    char nome[50];
    int score;
    int hp;
    int municao;
    int municao_total;
    int tipo_muni;
    Model modelo;
} Player;

typedef struct {
    Vector3 pos;
    int hp;
    float speed;
    bool alive;
    Model modelo;
} Inimigo;

typedef struct {
    Vector3 pos;
    int hp;
    float speed;
    Model modelo;
} Boss;

typedef struct {
    Vector3 pos;
    Vector3 dir;
    bool active;
    int dano;
    
} Bullet;

// ───────── FUNÇÕES AUXILIARES ─────────
void mover_inimigo(Inimigo* ini, Player* player) {
    if (!ini->alive) return;
    float dx = player->pos.x - ini->pos.x;
    float dz = player->pos.z - ini->pos.z;
    float dist = sqrtf(dx * dx + dz * dz);
    if (dist > 0) {
        ini->pos.x += (dx / dist) * ini->speed;
        ini->pos.z += (dz / dist) * ini->speed;
    }
}

void mover_Boss(Boss* boss, Player* player) {
    float dx = player->pos.x - boss->pos.x;
    float dz = player->pos.z - boss->pos.z;
    float dist = sqrtf(dx * dx + dz * dz);
    if (dist > 0) {
        boss->pos.x += (dx / dist) * boss->speed;
        boss->pos.z += (dz / dist) * boss->speed;
    }
}

bool ver_batida(Vector3 a, float tamA, Vector3 b, float tamB) {
    float dx = a.x - b.x;
    float dz = a.z - b.z;
    float distancia = sqrtf(dx * dx + dz * dz);
    return distancia < (tamA + tamB);
}

// ───────── TELAS ─────────
int menu(void) {
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("NAVIO 3D", 320, 100, 40, BLUE);
        DrawText("1 - Começar o jogo", 280, 200, 25, DARKGRAY);
        DrawText("2 - Ver pontuações", 280, 240, 25, DARKGRAY);
        DrawText("3 - Sair", 280, 280, 25, DARKGRAY);
        EndDrawing();

        if (IsKeyPressed(KEY_ONE)) return 1;
        if (IsKeyPressed(KEY_TWO)) return 2;
        if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_ESCAPE)) return 3;
    }
    return 3;
}

void telaGameOver(void) {
    BeginDrawing();
    ClearBackground(BLACK);
    DrawText("GAME OVER", 300, 180, 40, RED);
    DrawText("Pressione qualquer tecla para voltar ao menu", 180, 250, 20, WHITE);
    EndDrawing();
    while (!WindowShouldClose() && !IsKeyPressed(KEY_ENTER) && !IsKeyPressed(KEY_SPACE) && !IsKeyPressed(KEY_ESCAPE))
        ;
}

// ───────── JOGO ─────────
int jogar(ListaScore **scoreBoard) {
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 5.0f, 5.0f, 5.0f };
    camera.target = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

  

    Model mapa = LoadModel("models/mar1.glb");
    Model modeloPlayer = LoadModel("models/player.glb");
    Model modeloInimigo = LoadModel("models/barco.glb");
    Model modeloBoss = LoadModel("models/boss.glb");
    Player player = { (Vector3){0, 1, 0}, "Player", 0, MAX_HP, 10, 50, 0 ,modeloPlayer};
    int totalInimigos = 5;
    Inimigo inimigos[5];
    for (int i = 0; i < totalInimigos; i++) {
        inimigos[i].pos = (Vector3){ rand()%20 - 10, 1.0f, rand()%20 - 10 };
        inimigos[i].speed = 0.05f;
        inimigos[i].hp = 3;
        inimigos[i].alive = true;
        inimigos[i].modelo = modeloInimigo;
    }

    Boss boss = { (Vector3){10, 1, 10}, MAX_HP_BOSS, 0.03f ,modeloBoss};
    Bullet balas[MAX_BULLETS] = {0};

    int tempoJogo = 0;

    while (!WindowShouldClose() && player.hp > 0) {
        tempoJogo++;
        float movespeed = 0.1f;
        if (IsKeyDown(KEY_W)) player.pos.z -= movespeed;
        if (IsKeyDown(KEY_S)) player.pos.z += movespeed;
        if (IsKeyDown(KEY_A)) player.pos.x -= movespeed;
        if (IsKeyDown(KEY_D)) player.pos.x += movespeed;
        
        if (IsKeyPressed(KEY_ONE)) player.tipo_muni = 0;
        if (IsKeyPressed(KEY_TWO)) player.tipo_muni = 1;
        if (IsKeyPressed(KEY_THREE)) player.tipo_muni = 2;
        // ───────── CÂMERA SEGUE O JOGADOR ─────────
        camera.position.x = player.pos.x ;
        camera.position.y = player.pos.y + 5.0f;
        camera.position.z = player.pos.z + 10.0f;
        camera.target = player.pos;
        int dano_arma[] = {25, 10, 50};
        int dano = dano_arma[player.tipo_muni];

        for (int i = 0; i < totalInimigos; i++) {
            mover_inimigo(&inimigos[i], &player);
            if (inimigos[i].alive && ver_batida(player.pos, PLAYER_SIZE, inimigos[i].pos, PLAYER_SIZE)) {
                player.hp--;
                inimigos[i].pos = (Vector3){ rand()%20 - 10, 1.0f, rand()%20 - 10 };
            }
        }

        mover_Boss(&boss, &player);
        if (ver_batida(player.pos, PLAYER_SIZE, boss.pos, 2.0f)) {
            player.hp -= 3;
            boss.pos = (Vector3){ rand()%20 - 10, 1.0f, rand()%20 - 10 };
        }

        if (IsKeyPressed(KEY_SPACE) && player.municao > 0) {
            player.municao--;
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!balas[i].active) {
                    balas[i].active = true;
                    balas[i].pos = player.pos;
                    balas[i].dir = (Vector3){ 0, 0, -1 };
                    balas[i].dano = dano;
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_BULLETS; i++) {
            if (balas[i].active) {
                balas[i].pos.x += balas[i].dir.x * BULLET_SPEED;
                balas[i].pos.z += balas[i].dir.z * BULLET_SPEED;

                if (fabs(balas[i].pos.x) > 25 || fabs(balas[i].pos.z) > 25)
                    balas[i].active = false;

                for (int j = 0; j < totalInimigos; j++) {
                    if (inimigos[j].alive && ver_batida(balas[i].pos, 0.5f, inimigos[j].pos, PLAYER_SIZE)) {
                        inimigos[j].hp -= balas[i].dano;
                        balas[i].active = false;
                        if (inimigos[j].hp <= 0) {
                            inimigos[j].alive = false;
                            player.score += 10;
                        }
                    }
                }

                if (ver_batida(balas[i].pos, 0.5f, boss.pos, 2.0f)) {
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
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
        DrawModel(mapa, (Vector3){0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
            DrawGrid(20, 1.0f);
            DrawModel(player.modelo, player.pos, 1.0f, WHITE);
             DrawModel(boss.modelo, boss.pos, 2.0f, WHITE);
            for (int i = 0; i < totalInimigos; i++)
                if (inimigos[i].alive)
                     DrawModel(inimigos[i].modelo, inimigos[i].pos, 1.0f, WHITE);
            for (int i = 0; i < MAX_BULLETS; i++)
                if (balas[i].active)
                    DrawSphere(balas[i].pos, 0.2f, YELLOW);
        EndMode3D();

        DrawText(TextFormat("HP: %d", player.hp), 10, 10, 20, BLACK);
        DrawText(TextFormat("Score: %d", player.score), 10, 40, 20, DARKGRAY);
        DrawText(TextFormat("Munição: %d / %d", player.municao, player.municao_total), 10, 70, 20, ORANGE);
        EndDrawing();
    }
  
     // ===== DESCARREGAR MODELOS =====
    UnloadModel(mapa);
    UnloadModel(modeloPlayer);
    UnloadModel(modeloInimigo);
    UnloadModel(modeloBoss);
    add_ordenado_score(scoreBoard, player.score, player.municao_total, tempoJogo/60);
    mostrarScores(*scoreBoard);
    salvarScores(*scoreBoard);
    telaGameOver();
    return 0;
}

// ───────── MAIN ─────────
int main(void) {
    InitWindow(WIDTH, HEIGHT, "Navio 3D");
    SetTargetFPS(60);
    srand(time(NULL));

    ListaScore* scoreBoard = NULL;
    carregarScores(&scoreBoard);
    while (!WindowShouldClose()) {
        int opcao = menu();
        if (opcao == 1)
            jogar(&scoreBoard);
        else if (opcao == 2)
            mostrarScoresTela(scoreBoard);
        else if (opcao == 3)
            break;
    }
    salvarScores(scoreBoard);
    liberarScores(&scoreBoard);
    CloseWindow();
    return 0;
}
