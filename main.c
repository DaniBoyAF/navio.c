#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// ───────── CONFIG ─────────
#define WIDTH 800
#define HEIGHT 450
#define PLAYER_SIZE 1.0f
#define MAX_HP 20
#define MAX_HP_BOSS 500
#define MAX_BULLETS 30
#define BULLET_SPEED 25.0f        // velocidade das balas (unidade por segundo)
#define ARQUIVO_SCORES "scores.txt"

// ───────── ENUMS ─────────
typedef enum { CAM_TERCEIRA_PESSOA = 0, CAM_PRIMEIRA_PESSOA, CAM_ISOMETRICA } ModoCameraJogo;
typedef enum { MUNI_NORMAL = 0, MUNI_PESADA, MUNI_EXPLOSIVA } TipoMunicao;

// ───────── TYPEDEFS ─────────
typedef struct ListaScore {
    int pontuacao, dinheiro, tempo;
    struct ListaScore *proximo;
} ListaScore;

typedef struct {
    Vector3 pos;
    char nome[50];
    int score;
    int hp;
    int municao[3];
    int tipo_muni;
    Model modelo;
    float yaw;
} Player;

typedef struct {
    Vector3 pos;
    int hp;
    float speed;
    bool alive;
    Model modelo;
    float fireTimer;
    float fireRate;
} Inimigo;

typedef struct {
    Vector3 pos;
    int hp;
    float speed;
    Model modelo;
    float yaw;
} Boss;

typedef struct {
    Vector3 pos;
    Vector3 dir;
    bool active;
    int dano;
} Bullet;

typedef struct {
    Vector3 pos;
    float velocidade;
} Nuvem;

typedef struct {
    Vector3 pos;
    float vida;
    float tamanho;
} Espuma;

// ───────── CONSTANTS / GLOBAL ARRAYS ─────────
#define MAX_NUVENS 8
#define MAX_ESPUMAS 50
static Nuvem nuvens[MAX_NUVENS];
static Espuma espumas[MAX_ESPUMAS];

// ───────── PROTÓTIPOS ─────────
static BoundingBox TransformedBBox(BoundingBox box, Vector3 pos, float scale);
static void criarNuvens(void);
static void atualizarNuvens(float dt);
static void desenharNuvensSimples(void);
static void criarEspuma(Vector3 pos);
static void atualizarEspumas(float dt);
static void desenharEspumas(void);

// Score helpers
static ListaScore* criarNo(int pontuacao, int dinheiro, int tempo);
static void add_ordenado_score(ListaScore** inicio, int pontuacao, int dinheiro, int tempo);
static void salvarScores(ListaScore* inicio);
static void carregarScores(ListaScore** inicio);
static void liberarScores(ListaScore** inicio);

// Gameplay helpers
static void mover_inimigo(Inimigo* ini, Player* player);
static void mover_Boss(Boss* boss, Player* player);
static bool ver_batida(Vector3 a, float tamA, Vector3 b, float tamB);

// ───────── IMPLEMENTAÇÕES ─────────

// Bounding box simples (escala uniforme, sem rotação)
static BoundingBox TransformedBBox(BoundingBox box, Vector3 pos, float scale)
{
    BoundingBox out;
    out.min.x = box.min.x * scale + pos.x;
    out.min.y = box.min.y * scale + pos.y;
    out.min.z = box.min.z * scale + pos.z;
    out.max.x = box.max.x * scale + pos.x;
    out.max.y = box.max.y * scale + pos.y;
    out.max.z = box.max.z * scale + pos.z;
    return out;
}

// ───────── Score functions ─────────
static ListaScore* criarNo(int pontuacao, int dinheiro, int tempo) {
    ListaScore* novo = (ListaScore*) malloc(sizeof(ListaScore));
    novo->pontuacao = pontuacao; novo->dinheiro = dinheiro; novo->tempo = tempo; novo->proximo = NULL;
    return novo;
}
static void add_ordenado_score(ListaScore** inicio, int pontuacao, int dinheiro, int tempo) {
    ListaScore* novo = criarNo(pontuacao, dinheiro, tempo);
    if (*inicio == NULL || novo->pontuacao > (*inicio)->pontuacao) { novo->proximo = *inicio; *inicio = novo; return; }
    ListaScore* temp = *inicio;
    while (temp->proximo != NULL && temp->proximo->pontuacao > pontuacao) temp = temp->proximo;
    novo->proximo = temp->proximo; temp->proximo = novo;
}
static void salvarScores(ListaScore* inicio){
    FILE* arquivo = fopen(ARQUIVO_SCORES,"w");
    if (!arquivo) { printf("erro ao abrir o arquivo de scores\n"); return; }
    ListaScore* temp = inicio;
    while (temp) { fprintf(arquivo, "%d %d %d\n", temp->pontuacao, temp->dinheiro, temp->tempo); temp = temp->proximo; }
    fclose(arquivo); printf("Salvo com sucesso!\n");
}
static void carregarScores(ListaScore** inicio) {
    FILE* arquivo = fopen(ARQUIVO_SCORES, "r");
    if (!arquivo) return;
    int pontuacao, dinheiro, tempo;
    while (fscanf(arquivo, "%d %d %d", &pontuacao, &dinheiro, &tempo) != EOF) add_ordenado_score(inicio, pontuacao, dinheiro, tempo);
    fclose(arquivo);
}
static void liberarScores(ListaScore** inicio){
    ListaScore* temp;
    while (*inicio) { temp = *inicio; *inicio = (*inicio)->proximo; free(temp); }
}

// ───────── Ambiente (nuvens / espumas) ─────────
static void criarNuvens(void) {
    for (int i = 0; i < MAX_NUVENS; i++) {
        nuvens[i].pos = (Vector3){ GetRandomValue(-80, 80), 15.0f + GetRandomValue(0, 5), GetRandomValue(-80, 80) };
        nuvens[i].velocidade = 0.02f + GetRandomValue(0, 5) * 0.005f;
    }
}
static void atualizarNuvens(float dt) {
    for (int i = 0; i < MAX_NUVENS; i++) {
        nuvens[i].pos.x += nuvens[i].velocidade;
        if (nuvens[i].pos.x > 90) nuvens[i].pos.x = -90;
    }
}
static void desenharNuvensSimples(void) {
    for (int i = 0; i < MAX_NUVENS; i++) DrawSphere(nuvens[i].pos, 3.0f, Fade(WHITE, 0.8f));
}

static void criarEspuma(Vector3 pos) {
    for (int i = 0; i < MAX_ESPUMAS; i++) {
        if (espumas[i].vida <= 0) { espumas[i].pos = pos; espumas[i].vida = 2.0f; espumas[i].tamanho = 0.5f + GetRandomValue(0,10)*0.1f; break; }
    }
}
static void atualizarEspumas(float dt) {
    for (int i = 0; i < MAX_ESPUMAS; i++) if (espumas[i].vida > 0) { espumas[i].vida -= dt; espumas[i].tamanho *= 0.98f; }
}
static void desenharEspumas(void) {
    for (int i = 0; i < MAX_ESPUMAS; i++) if (espumas[i].vida > 0) {
        float alpha = espumas[i].vida / 2.0f;
        DrawSphere(espumas[i].pos, espumas[i].tamanho, Fade(WHITE, alpha));
    }
}

// ───────── Movimentação / colisões ─────────
void mover_inimigo(Inimigo* ini, Player* player) {
    if (!ini->alive) return;
    float dx = player->pos.x - ini->pos.x;
    float dz = player->pos.z - ini->pos.z;
    float dist = sqrtf(dx*dx + dz*dz);
    if (dist > 0.0001f) { ini->pos.x += (dx / dist) * ini->speed; ini->pos.z += (dz / dist) * ini->speed; }
}
void mover_Boss(Boss* boss, Player* player) {
    float dx = player->pos.x - boss->pos.x;
    float dz = player->pos.z - boss->pos.z;
    float dist = sqrtf(dx*dx + dz*dz);
    if (dist > 0.0001f) { boss->pos.x += (dx / dist) * boss->speed; boss->pos.z += (dz / dist) * boss->speed; }
}
bool ver_batida(Vector3 a, float tamA, Vector3 b, float tamB) {
    float dx = a.x - b.x; float dz = a.z - b.z;
    float distancia = sqrtf(dx*dx + dz*dz);
    return distancia < (tamA + tamB);
}

// ───────── Telas (menu / scoreboard / game over) ─────────
int menu(void) {
    while (!WindowShouldClose()) {
        BeginDrawing(); ClearBackground(RAYWHITE);
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

void mostrarScoresTela(ListaScore* inicio) {
    while (!WindowShouldClose()) {
        BeginDrawing(); ClearBackground(RAYWHITE);
        DrawText("===== SCOREBOARD =====", 220, 40, 30, BLACK);
        int y = 100; ListaScore* temp = inicio; int count = 0;
        while (temp != NULL && count < 10) {
            DrawText(TextFormat("%d) Pontos: %d | Dinheiro: %d | Tempo: %d", count+1, temp->pontuacao, temp->dinheiro, temp->tempo),
                     150, y, 20, DARKGRAY);
            y += 30; temp = temp->proximo; count++;
        }
        DrawText("Pressione ENTER ou ESPACO para voltar", 200, HEIGHT - 60, 20, RED);
        EndDrawing();
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) break;
    }
}

void telaGameOver(void) {
    while (!WindowShouldClose()) {
        BeginDrawing(); ClearBackground(BLACK);
        DrawText("GAME OVER", 300, 180, 40, RED);
        DrawText("Pressione ENTER ou ESPACO para voltar ao menu", 140, 250, 20, WHITE);
        EndDrawing();
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) break;
    }
}

// ───────── JOGO (função principal do jogo) ─────────
int jogar(ListaScore **scoreBoard) {
    // Camera inicial
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 5.0f, 5.0f, 5.0f };
    camera.target = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    ModoCameraJogo modoCam = CAM_TERCEIRA_PESSOA;

    // Carrega modelos/texturas/shaders
    Model mapa = LoadModel("models/mar1.glb");
    Model modeloPlayer = LoadModel("models/barco.glb");
    Model modeloInimigo = LoadModel("models/barco.glb");
    Model modeloBoss = LoadModel("models/boss.glb");

    // Player init
    Player player = {0};
    player.pos = (Vector3){0, 1, 0}; strcpy(player.nome, "Player");
    player.score = 0; player.hp = MAX_HP;
    player.municao[MUNI_NORMAL] = 50; player.municao[MUNI_PESADA] = 20; player.municao[MUNI_EXPLOSIVA] = 5;
    player.tipo_muni = MUNI_NORMAL; player.modelo = modeloPlayer; player.yaw = 0.0f;

    // Inimigos
    int totalInimigos = 5;
    Inimigo inimigos[5];
    for (int i = 0; i < totalInimigos; i++) {
        inimigos[i].pos = (Vector3){ GetRandomValue(-50,50), 1.0f, GetRandomValue(-50,50) };
        inimigos[i].speed = 0.05f;
        inimigos[i].hp = 3;
        inimigos[i].alive = true;
        inimigos[i].modelo = modeloInimigo;
        inimigos[i].fireRate = 1.5f + GetRandomValue(0,150)/100.0f;
        inimigos[i].fireTimer = GetRandomValue(0,1000)/1000.0f * inimigos[i].fireRate;
    }

    // Boss
    Boss boss = { .pos = (Vector3){10,1,10}, .hp = MAX_HP_BOSS, .speed = 0.03f, .modelo = modeloBoss, .yaw = 0.0f };

    // Balas
    Bullet balas[MAX_BULLETS] = {0};
    Bullet enemyBullets[MAX_BULLETS] = {0};

    int tempoJogo = 0;
    Shader shaderAgua = LoadShader(NULL, "shaders/agua.fs");
    float shaderTime = 0.0f;
    int locTempo = GetShaderLocation(shaderAgua, "tempo");

    criarNuvens();
    for (int i = 0; i < MAX_ESPUMAS; i++) espumas[i].vida = 0;

    SetTargetFPS(60);

    // Loop principal do jogo
    while (!WindowShouldClose() && player.hp > 0 && boss.hp > 0) {
        tempoJogo++;
        float dt = GetFrameTime();

        // INPUT: movimento + rotação
        float moveSpeed = 5.0f * dt;
        float turnSpeed = 120.0f * dt;
        float yawRad = player.yaw * (PI / 180.0f);
        if (IsKeyDown(KEY_W)) { player.pos.x += sinf(yawRad) * moveSpeed; player.pos.z += cosf(yawRad) * moveSpeed; }
        if (IsKeyDown(KEY_S)) { player.pos.x -= sinf(yawRad) * moveSpeed; player.pos.z -= cosf(yawRad) * moveSpeed; }
        if (IsKeyDown(KEY_A)) player.yaw -= turnSpeed;
        if (IsKeyDown(KEY_D)) player.yaw += turnSpeed;

        if (IsKeyPressed(KEY_ONE)) player.tipo_muni = MUNI_NORMAL;
        if (IsKeyPressed(KEY_TWO)) player.tipo_muni = MUNI_PESADA;
        if (IsKeyPressed(KEY_THREE)) player.tipo_muni = MUNI_EXPLOSIVA;
        if (IsKeyPressed(KEY_C)) modoCam = (modoCam + 1) % 3;

        // Camera follow
        switch (modoCam) {
            case CAM_TERCEIRA_PESSOA:
                camera.position = (Vector3){ player.pos.x, player.pos.y + 8.0f, player.pos.z + 15.0f };
                camera.target = player.pos;
                break;
            case CAM_PRIMEIRA_PESSOA:
                camera.position = (Vector3){ player.pos.x, player.pos.y + 2.0f, player.pos.z };
                camera.target = (Vector3){ player.pos.x, player.pos.y + 2.0f, player.pos.z - 5.0f };
                break;
            case CAM_ISOMETRICA:
                camera.position = (Vector3){ player.pos.x + 20.0f, player.pos.y + 20.0f, player.pos.z + 20.0f };
                camera.target = player.pos;
                break;
        }

        int dano_arma[] = {10, 25, 50};
        int dano = dano_arma[player.tipo_muni];

        // Atualiza inimigos
        for (int i = 0; i < totalInimigos; i++) mover_inimigo(&inimigos[i], &player);
        mover_Boss(&boss, &player);

        // Colisões usando bounding boxes (player e boss/inimigos)
        BoundingBox bbPlayer = TransformedBBox(GetModelBoundingBox(player.modelo), player.pos, 1.0f);
        for (int i = 0; i < totalInimigos; i++) {
            BoundingBox bbInimigo = TransformedBBox(GetModelBoundingBox(inimigos[i].modelo), inimigos[i].pos, 1.0f);
            if (inimigos[i].alive && CheckCollisionBoxes(bbPlayer, bbInimigo)) { player.hp--; inimigos[i].pos = (Vector3){ GetRandomValue(-50,50), 1.0f, GetRandomValue(-50,50) }; }
        }
        BoundingBox bbBoss = TransformedBBox(GetModelBoundingBox(boss.modelo), boss.pos, 2.0f);
        if (CheckCollisionBoxes(bbPlayer, bbBoss)) { player.hp -= 3; boss.pos = (Vector3){ GetRandomValue(-50,50), 1.0f, GetRandomValue(-50,50) }; }

        // Atirar (player)
        if (IsKeyPressed(KEY_SPACE) && player.municao[player.tipo_muni] > 0) {
            player.municao[player.tipo_muni]--;
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!balas[i].active) {
                    balas[i].active = true;
                    // spawn na frente do player (considerando yaw)
                    balas[i].pos = (Vector3){ player.pos.x + sinf(yawRad)*1.5f, player.pos.y + 0.6f, player.pos.z + cosf(yawRad)*1.5f };
                    balas[i].dir = (Vector3){ sinf(yawRad), 0.0f, cosf(yawRad) };
                    balas[i].dano = dano;
                    break;
                }
            }
        }

        // Atualiza balas do jogador (usa dt)
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (!balas[i].active) continue;
            balas[i].pos.x += balas[i].dir.x * BULLET_SPEED * dt;
            balas[i].pos.y += balas[i].dir.y * BULLET_SPEED * dt;
            balas[i].pos.z += balas[i].dir.z * BULLET_SPEED * dt;
            if (fabs(balas[i].pos.x) > 200 || fabs(balas[i].pos.z) > 200) { balas[i].active = false; continue; }

            // colisão com inimigos
            for (int j = 0; j < totalInimigos; j++) {
                if (!inimigos[j].alive) continue;
                BoundingBox bbInimigo = TransformedBBox(GetModelBoundingBox(inimigos[j].modelo), inimigos[j].pos, 1.0f);
                if (CheckCollisionBoxSphere(bbInimigo, balas[i].pos, 0.5f)) {
                    inimigos[j].hp -= balas[i].dano; balas[i].active = false;
                    if (inimigos[j].hp <= 0) { inimigos[j].alive = false; player.score += 10; }
                    break;
                }
            }
            // colisão com boss
            if (balas[i].active && CheckCollisionBoxSphere(bbBoss, balas[i].pos, 0.5f)) {
                boss.hp -= balas[i].dano; balas[i].active = false;
                if (boss.hp <= 0) { boss.hp = 0; player.score += 100; break; }
            }
        }

        // Inimigos atiram
        for (int ei = 0; ei < totalInimigos; ei++) {
            if (!inimigos[ei].alive) continue;
            inimigos[ei].fireTimer -= dt;
            if (inimigos[ei].fireTimer <= 0.0f) {
                for (int b = 0; b < MAX_BULLETS; b++) {
                    if (!enemyBullets[b].active) {
                        enemyBullets[b].active = true;
                        enemyBullets[b].pos = (Vector3){ inimigos[ei].pos.x, inimigos[ei].pos.y + 0.6f, inimigos[ei].pos.z };
                        Vector3 dir = { player.pos.x - inimigos[ei].pos.x, player.pos.y - inimigos[ei].pos.y, player.pos.z - inimigos[ei].pos.z };
                        float mag = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
                        if (mag > 0.0001f) { dir.x /= mag; dir.y /= mag; dir.z /= mag; }
                        enemyBullets[b].dir = dir;
                        enemyBullets[b].dano = 5;
                        break;
                    }
                }
                inimigos[ei].fireTimer = inimigos[ei].fireRate;
            }
        }

        // Atualiza balas inimigas
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (!enemyBullets[i].active) continue;
            enemyBullets[i].pos.x += enemyBullets[i].dir.x * BULLET_SPEED * dt;
            enemyBullets[i].pos.y += enemyBullets[i].dir.y * BULLET_SPEED * dt;
            enemyBullets[i].pos.z += enemyBullets[i].dir.z * BULLET_SPEED * dt;
            if (fabs(enemyBullets[i].pos.x) > 200 || fabs(enemyBullets[i].pos.z) > 200) { enemyBullets[i].active = false; continue; }
            if (ver_batida(enemyBullets[i].pos, 0.3f, player.pos, PLAYER_SIZE)) { player.hp -= enemyBullets[i].dano; enemyBullets[i].active = false; }
        }

        // Espumas baseadas em movimento
        static Vector3 prevPlayerPos = {0};
        Vector3 delta = { player.pos.x - prevPlayerPos.x, 0, player.pos.z - prevPlayerPos.z };
        float moved = sqrtf(delta.x*delta.x + delta.z*delta.z);
        if (moved > 0.001f) {
            Vector3 dir = { delta.x / moved, 0, delta.z / moved };
            int spawnCount = (int)fmaxf(1.0f, fminf(4.0f, moved*100.0f));
            for (int s = 0; s < spawnCount; s++) {
                float spread = GetRandomValue(-10,10)/100.0f;
                Vector3 posEspuma = { player.pos.x - dir.x*1.0f + spread, -0.4f, player.pos.z - dir.z*1.0f + spread };
                criarEspuma(posEspuma);
            }
        }
        prevPlayerPos = player.pos;

        // Atualiza ambiente e shader time
        atualizarNuvens(dt);
        atualizarEspumas(dt);
        shaderTime += dt;
        SetShaderValue(shaderAgua, locTempo, &shaderTime, SHADER_UNIFORM_FLOAT);

        // DRAW
        BeginDrawing();
        ClearBackground(SKYBLUE);
        BeginMode3D(camera);

        BeginShaderMode(shaderAgua);
        for (int i = 0; i < 3; i++) {
            float offset = sinf(shaderTime * 0.5f + i) * 0.2f;
            DrawPlane((Vector3){0, -0.5f + offset, 0}, (Vector2){200,200}, (i==0)?BLUE:(Color){0,100,200,100});
        }
        EndShaderMode();

        // Modelos
        DrawModelEx(player.modelo, player.pos, (Vector3){0,1,0}, player.yaw, (Vector3){1,1,1}, WHITE);
        DrawModel(boss.modelo, boss.pos, 2.0f, WHITE);
        for (int i = 0; i < totalInimigos; i++) if (inimigos[i].alive) DrawModel(inimigos[i].modelo, inimigos[i].pos, 1.0f, WHITE);

        // Balas
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (balas[i].active) {
                Color corBala = (player.tipo_muni==MUNI_NORMAL)?YELLOW:(player.tipo_muni==MUNI_PESADA)?ORANGE:RED;
                DrawSphere(balas[i].pos, 0.2f, corBala);
            }
            if (enemyBullets[i].active) DrawSphere(enemyBullets[i].pos, 0.18f, MAROON);
        }

        desenharNuvensSimples();
        desenharEspumas();
        EndMode3D();

        // HUD
        DrawText(TextFormat("HP: %d", player.hp), 10, 10, 20, BLACK);
        DrawText(TextFormat("Score: %d", player.score), 10, 40, 20, DARKGRAY);
        Color cores[] = {YELLOW, ORANGE, RED};
        const char* nomes[] = {"Normal","Pesada","Explosiva"};
        for (int i = 0; i < 3; i++) {
            Color cor = (i==player.tipo_muni)?cores[i]:GRAY;
            DrawText(TextFormat("%d: %s x%d", i+1, nomes[i], player.municao[i]), 10, 70 + i*25, 18, cor);
        }
        if (boss.hp > 0) {
            DrawRectangle(WIDTH/2 - 150, 10, 300, 20, DARKGRAY);
            float pct = (float)boss.hp / MAX_HP_BOSS;
            DrawRectangle(WIDTH/2 - 150, 10, (int)(300 * pct), 20, RED);
            DrawText(TextFormat("BOSS: %d / %d", boss.hp, MAX_HP_BOSS), WIDTH/2 - 70, 12, 16, WHITE);
        }
        const char* camModes[] = {"3ª Pessoa","1ª Pessoa","Isométrica"};
        DrawText(TextFormat("Câmera (C): %s", camModes[modoCam]), 10, HEIGHT - 30, 16, DARKBLUE);

        EndDrawing();
    }

    // Cleanup
    UnloadShader(shaderAgua);
    UnloadModel(mapa);
    UnloadModel(modeloPlayer);
    UnloadModel(modeloInimigo);
    UnloadModel(modeloBoss);

    int totalMuni = player.municao[0] + player.municao[1] + player.municao[2];
    add_ordenado_score(scoreBoard, player.score, totalMuni, tempoJogo/60);
    salvarScores(*scoreBoard);

    if (boss.hp <= 0) {
        BeginDrawing(); ClearBackground(BLACK); DrawText("VITÓRIA!", 280, 150, 50, GREEN); EndDrawing();
        while (!WindowShouldClose()) { BeginDrawing(); ClearBackground(BLACK); DrawText("VITÓRIA!", 280,150,50,GREEN); EndDrawing(); if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) break; }
    } else telaGameOver();

    return 0;
}

// ───────── main ─────────
int main(void) {
    InitWindow(WIDTH, HEIGHT, "Navio 3D");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    srand(time(NULL));

    ListaScore* scoreBoard = NULL;
    carregarScores(&scoreBoard);

    while (!WindowShouldClose()) {
        int opcao = menu();
        if (opcao == 1) jogar(&scoreBoard);
        else if (opcao == 2) mostrarScoresTela(scoreBoard);
        else if (opcao == 3) break;
    }

    salvarScores(scoreBoard);
    liberarScores(&scoreBoard);
    CloseWindow();
    return 0;
}

