#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

// Prototipo para evitar declaração implícita da função TransformedBBox
static BoundingBox TransformedBBox(BoundingBox box, Vector3 pos, float scale);

// ───────── ENUMS ─────────
typedef enum {
    CAM_TERCEIRA_PESSOA = 0,
    CAM_PRIMEIRA_PESSOA,
    CAM_ISOMETRICA
} ModoCameraJogo;

typedef enum {
    MUNI_NORMAL = 0,    // 10 dano, rápida
    MUNI_PESADA,        // 25 dano, média
    MUNI_EXPLOSIVA      // 50 dano, lenta
} TipoMunicao;

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
    while (!WindowShouldClose()) {
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
        DrawText("Pressione ENTER ou ESPACO para voltar", 200, HEIGHT - 60, 20, RED);
        EndDrawing();
        
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
            break;
    }
}

// ───────── STRUCTS 3D ─────────
typedef struct {
    Vector3 pos;
    char nome[50];
    int score;
    int hp;
    int municao[3];      // municao por tipo [normal, pesada, explosiva]
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

typedef struct {
    Vector3 pos;
    float velocidade;
} Nuvem;

#define MAX_NUVENS 8
Nuvem nuvens[MAX_NUVENS];

typedef struct {
    Vector3 pos;
    float vida;
    float tamanho;
} Espuma;

#define MAX_ESPUMAS 50
Espuma espumas[MAX_ESPUMAS];

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

void criarNuvens(void) {
    for (int i = 0; i < MAX_NUVENS; i++) {
        nuvens[i].pos = (Vector3){ GetRandomValue(-80, 80), 15.0f + GetRandomValue(0, 5), GetRandomValue(-80, 80) };
        nuvens[i].velocidade = 0.02f + GetRandomValue(0, 5) * 0.005f;
    }
}

void atualizarNuvens(float dt) {
    for (int i = 0; i < MAX_NUVENS; i++) {
        nuvens[i].pos.x += nuvens[i].velocidade;
        if (nuvens[i].pos.x > 90) nuvens[i].pos.x = -90;
    }
}

void desenharNuvens(Camera3D camera, Texture2D tex) {
    for (int i = 0; i < MAX_NUVENS; i++) {
        DrawBillboard(camera, tex, nuvens[i].pos, 6.0f, WHITE);
    }
}

void desenharNuvensSimples() {
    for (int i = 0; i < MAX_NUVENS; i++) {
        DrawSphere(nuvens[i].pos, 3.0f, Fade(WHITE, 0.8f));
    }
}

void criarEspuma(Vector3 pos) {
    for (int i = 0; i < MAX_ESPUMAS; i++) {
        if (espumas[i].vida <= 0) {
            espumas[i].pos = pos;
            espumas[i].vida = 2.0f;  // duração em segundos
            espumas[i].tamanho = 0.5f + GetRandomValue(0, 10) * 0.1f;
            break;
        }
    }
}

void atualizarEspumas(float dt) {
    for (int i = 0; i < MAX_ESPUMAS; i++) {
        if (espumas[i].vida > 0) {
            espumas[i].vida -= dt;
            espumas[i].tamanho *= 0.98f;  // encolhe lentamente
        }
    }
}

void desenharEspumas() {
    for (int i = 0; i < MAX_ESPUMAS; i++) {
        if (espumas[i].vida > 0) {
            float alpha = espumas[i].vida / 2.0f;
            DrawSphere(espumas[i].pos, espumas[i].tamanho, Fade(WHITE, alpha));
        }
    }
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
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawText("GAME OVER", 300, 180, 40, RED);
        DrawText("Pressione ENTER ou ESPACO para voltar ao menu", 140, 250, 20, WHITE);
        EndDrawing();
        
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
            break;
    }
}

// ───────── JOGO ─────────
int jogar(ListaScore **scoreBoard) {
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 5.0f, 5.0f, 5.0f };
    camera.target = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    ModoCameraJogo modoCam = CAM_TERCEIRA_PESSOA;

    Model mapa = LoadModel("models/mar1.glb");
    Model modeloPlayer = LoadModel("models/player.glb");
    Model modeloInimigo = LoadModel("models/barco.glb");
    Model modeloBoss = LoadModel("models/boss.glb");
    
    Player player = {0};
    player.pos = (Vector3){0, 1, 0};
    strcpy(player.nome, "Player");
    player.score = 0;
    player.hp = MAX_HP;
    player.municao[MUNI_NORMAL] = 50;
    player.municao[MUNI_PESADA] = 20;
    player.municao[MUNI_EXPLOSIVA] = 5;
    player.tipo_muni = MUNI_NORMAL;
    player.modelo = modeloPlayer;

    int totalInimigos = 5;
    Inimigo inimigos[5];
    for (int i = 0; i < totalInimigos; i++) {
        inimigos[i].pos = (Vector3){ rand()%100 - 50, 1.0f, rand()%100 - 50 };
        inimigos[i].speed = 0.05f;
        inimigos[i].hp = 3;
        inimigos[i].alive = true;
        inimigos[i].modelo = modeloInimigo;
    }

    Boss boss = { (Vector3){10, 1, 10}, MAX_HP_BOSS, 0.03f, modeloBoss};
    Bullet balas[MAX_BULLETS] = {0};

    int tempoJogo = 0;

    Shader shaderAgua = LoadShader(NULL, "shaders/agua.fs");
    float tempo = 0.0f;
    int locTempo = GetShaderLocation(shaderAgua, "tempo");

    criarNuvens();

    // Inicializar espumas
    for (int i = 0; i < MAX_ESPUMAS; i++) espumas[i].vida = 0;

    while (!WindowShouldClose() && player.hp > 0 && boss.hp > 0) {
        tempoJogo++;
        float movespeed = 0.1f;
        if (IsKeyDown(KEY_W)) player.pos.z -= movespeed;
        if (IsKeyDown(KEY_S)) player.pos.z += movespeed;
        if (IsKeyDown(KEY_A)) player.pos.x -= movespeed;
        if (IsKeyDown(KEY_D)) player.pos.x += movespeed;
        
        // Trocar munição (1, 2, 3)
        if (IsKeyPressed(KEY_ONE))   player.tipo_muni = MUNI_NORMAL;
        if (IsKeyPressed(KEY_TWO))   player.tipo_muni = MUNI_PESADA;
        if (IsKeyPressed(KEY_THREE)) player.tipo_muni = MUNI_EXPLOSIVA;
        // Trocar modo de câmera (C)
        if (IsKeyPressed(KEY_C)) {
            modoCam = (modoCam + 1) % 3;
        }

        // ───────── CÂMERA DINÂMICA ─────────
        switch (modoCam) {
            case CAM_TERCEIRA_PESSOA:
                camera.position.x = player.pos.x;
                camera.position.y = player.pos.y + 8.0f;
                camera.position.z = player.pos.z + 15.0f;
                camera.target = player.pos;
                break;
            case CAM_PRIMEIRA_PESSOA:
                camera.position = (Vector3){player.pos.x, player.pos.y + 2.0f, player.pos.z};
                camera.target = (Vector3){player.pos.x, player.pos.y + 2.0f, player.pos.z - 5.0f};
                break;
            case CAM_ISOMETRICA:
                camera.position.x = player.pos.x + 20.0f;
                camera.position.y = player.pos.y + 20.0f;
                camera.position.z = player.pos.z + 20.0f;
                camera.target = player.pos;
                break;
        }

        int dano_arma[] = {10, 25, 50};
        int dano = dano_arma[player.tipo_muni];

    // antes de checar inimigos:
    BoundingBox bbPlayer = TransformedBBox(GetModelBoundingBox(player.modelo), player.pos, 1.0f);

    for (int i = 0; i < totalInimigos; i++) {
        BoundingBox bbInimigo = TransformedBBox(GetModelBoundingBox(inimigos[i].modelo), inimigos[i].pos, 1.0f);
        mover_inimigo(&inimigos[i], &player);
        if (inimigos[i].alive && CheckCollisionBoxes(bbPlayer, bbInimigo)) {
            player.hp--;
            inimigos[i].pos = (Vector3){ rand()%100 - 50, 1.0f, rand()%100 - 50 };
        }
    }
    // colisão com boss
    BoundingBox bbBoss = TransformedBBox(GetModelBoundingBox(boss.modelo), boss.pos, 2.0f); // se desenha com escala 2.0f
    if (CheckCollisionBoxes(bbPlayer, bbBoss)) {
        player.hp -= 3;
        boss.pos = (Vector3){ rand()%100 - 50, 1.0f, rand()%100 - 50 };
    }

        if (IsKeyPressed(KEY_SPACE) && player.municao[player.tipo_muni] > 0) {
            player.municao[player.tipo_muni]--;
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

                if (fabs(balas[i].pos.x) > 100 || fabs(balas[i].pos.z) > 100)
                    balas[i].active = false;

                for (int j = 0; j < totalInimigos; j++) {
                    if (inimigos[j].alive) {
                        BoundingBox bbInimigo = TransformedBBox(GetModelBoundingBox(inimigos[j].modelo), inimigos[j].pos, 1.0f);
                        if (CheckCollisionBoxSphere(bbInimigo, balas[i].pos, 0.5f)) {
                            inimigos[j].hp -= balas[i].dano;
                            balas[i].active = false;
                            if (inimigos[j].hp <= 0) { inimigos[j].alive = false; player.score += 10; }
                        }
                    }
                }

                // bala x boss
                if (CheckCollisionBoxSphere(bbBoss, balas[i].pos, 0.5f)) {
                    boss.hp -= balas[i].dano;
                    balas[i].active = false;
                    if (boss.hp <= 0) { boss.hp = 0; player.score += 100; break; }
                }
            }
        }

        // Criar espumas atrás do jogador se movendo
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_S) || IsKeyDown(KEY_A) || IsKeyDown(KEY_D)) {
            if (GetRandomValue(0, 10) < 3) {  // chance de criar espuma
                Vector3 posEspuma = (Vector3){player.pos.x + GetRandomValue(-5, 5) * 0.1f, 
                                              -0.4f, 
                                              player.pos.z + 1.0f};
                criarEspuma(posEspuma);
            }
        }

        atualizarNuvens(GetFrameTime());
        atualizarEspumas(GetFrameTime());

        tempo += GetFrameTime();
        SetShaderValue(shaderAgua, locTempo, &tempo, SHADER_UNIFORM_FLOAT);

        BeginDrawing();
        ClearBackground(SKYBLUE);

        BeginMode3D(camera);
        BeginShaderMode(shaderAgua);
            for (int i = 0; i < 3; i++) {
                float offset = sin(tempo * 0.5f + i) * 0.2f;
                DrawPlane((Vector3){0, -0.5f + offset, 0}, (Vector2){200, 200},
                          (i == 0) ? BLUE : (Color){0, 100, 200, 100});
            }
        EndShaderMode();

        DrawModel(player.modelo, player.pos, 1.0f, WHITE);
        DrawModel(boss.modelo, boss.pos, 2.0f, WHITE);

        for (int i = 0; i < totalInimigos; i++)
            if (inimigos[i].alive)
                DrawModel(inimigos[i].modelo, inimigos[i].pos, 1.0f, WHITE);

        for (int i = 0; i < MAX_BULLETS; i++)
            if (balas[i].active) {
                Color corBala = (player.tipo_muni == MUNI_NORMAL) ? YELLOW : 
                               (player.tipo_muni == MUNI_PESADA) ? ORANGE : RED;
                DrawSphere(balas[i].pos, 0.2f, corBala);
            }

        // Desenhe as nuvens
        desenharNuvensSimples();
        desenharEspumas();  // desenhe após os modelos
        EndMode3D();

        // ───────── HUD ─────────
        DrawText(TextFormat("HP: %d", player.hp), 10, 10, 20, BLACK);
        DrawText(TextFormat("Score: %d", player.score), 10, 40, 20, DARKGRAY);
        
        // Munição colorida por tipo
        Color cores[] = {YELLOW, ORANGE, RED};
        const char* nomes[] = {"Normal", "Pesada", "Explosiva"};
        for (int i = 0; i < 3; i++) {
            Color cor = (i == player.tipo_muni) ? cores[i] : GRAY;
            DrawText(TextFormat("%d: %s x%d", i+1, nomes[i], player.municao[i]), 
                     10, 70 + i*25, 18, cor);
        }
        
        // Barra de vida do Boss (só se HP > 0)
        if (boss.hp > 0) {
            DrawRectangle(WIDTH/2 - 150, 10, 300, 20, DARKGRAY);
            float percentHP = (float)boss.hp / MAX_HP_BOSS;
            DrawRectangle(WIDTH/2 - 150, 10, (int)(300 * percentHP), 20, RED);
            DrawText(TextFormat("BOSS: %d / %d", boss.hp, MAX_HP_BOSS), WIDTH/2 - 70, 12, 16, WHITE);
        }
        
        // Modo de câmera
        const char* camModes[] = {"3ª Pessoa", "1ª Pessoa", "Isométrica"};
        DrawText(TextFormat("Câmera (C): %s", camModes[modoCam]), 10, HEIGHT - 30, 16, DARKBLUE);
        
        EndDrawing();
    }

    UnloadShader(shaderAgua);
    UnloadModel(mapa);
    UnloadModel(modeloPlayer);
    UnloadModel(modeloInimigo);
    UnloadModel(modeloBoss);
    
    int totalMuni = player.municao[0] + player.municao[1] + player.municao[2];
    add_ordenado_score(scoreBoard, player.score, totalMuni, tempoJogo/60);
    salvarScores(*scoreBoard);
    
    // Tela de vitória ou derrota
    if (boss.hp <= 0) {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawText("VITÓRIA!", 280, 150, 50, GREEN);
        DrawText(TextFormat("Score Final: %d", player.score), 260, 220, 30, WHITE);
        DrawText("Pressione ENTER para continuar", 200, 280, 20, GRAY);
        EndDrawing();
        
        while (!WindowShouldClose()) {
            BeginDrawing();
            ClearBackground(BLACK);
            DrawText("VITÓRIA!", 280, 150, 50, GREEN);
            DrawText(TextFormat("Score Final: %d", player.score), 260, 220, 30, WHITE);
            DrawText("Pressione ENTER para continuar", 200, 280, 20, GRAY);
            EndDrawing();
            
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
                break;
        }
    } else {
        telaGameOver();
    }
    
    return 0;
}

// ───────── MAIN ─────────
int main(void) {
    InitWindow(WIDTH, HEIGHT, "Navio 3D");
    SetExitKey(KEY_NULL);  // desabilita ESC fechar a janela automaticamente
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

// Retorna a bounding box do modelo já escalada e traduzida para a posição do modelo
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
