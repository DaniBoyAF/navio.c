#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// ───────── CONFIG ─────────
#define WIDTH 1280
#define HEIGHT 720
#define PLAYER_SIZE 1.0f
#define MAX_HP 20
#define MAX_HP_BOSS 5000
#define MAX_BULLETS 30
#define BULLET_SPEED 250.0f        // velocidade das balas (unidade por segundo)
#define ARQUIVO_SCORES "scores.txt"
#define MAP_PLANE_SIZE 2000.0f     // ajustar aqui o tamanho visual do mar


// ───────── ENUMS ─────────
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
    float yaw;// ângulo de rotação (em graus)
    float fireTimer;     // tempo restante até poder atirar de novo (segundos)
    float fireCooldown;  // tempo de cooldown entre tiros (segundos)
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
#define MAX_NUVENS 34
#define MAX_ESPUMAS 50
static Nuvem nuvens[MAX_NUVENS];
static Espuma espumas[MAX_ESPUMAS];
// textura do menu (png)
static Texture2D menuTexture;
// texture para a tabela de scores
static Texture2D scoreTexture;
#define SCORE_ROWS 10
#define SCORE_COLS 3
static void buildScoresMatrix(ListaScore* inicio, int mat[SCORE_ROWS][SCORE_COLS]);
static void salvarScoresComoMatriz(ListaScore* inicio);

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
static void buildScoresMatrix(ListaScore* inicio, int mat[SCORE_ROWS][SCORE_COLS]);
static void salvarScoresComoMatriz(ListaScore* inicio);

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

// monta matriz (até SCORE_ROWS) a partir da lista de scores
static void buildScoresMatrix(ListaScore* inicio, int mat[SCORE_ROWS][SCORE_COLS]) {
    for (int r = 0; r < SCORE_ROWS; r++) {
        for (int c = 0; c < SCORE_COLS; c++) mat[r][c] = 0;
    }
    ListaScore* tmp = inicio;
    int i = 0;
    while (tmp != NULL && i < SCORE_ROWS) {
        mat[i][0] = tmp->pontuacao;
        mat[i][1] = tmp->dinheiro;
        mat[i][2] = tmp->tempo;
        tmp = tmp->proximo; i++;
    }
}

// salva matriz em arquivo (formato: rows cols\n then rows lines)
static void salvarScoresComoMatriz(ListaScore* inicio) {
    int mat[SCORE_ROWS][SCORE_COLS];
    buildScoresMatrix(inicio, mat);
    // grava a matriz no arquivo de scores padrão (substitui scores.txt)
    FILE *f = fopen(ARQUIVO_SCORES, "w");
    if (!f) return;
    for (int r = 0; r < SCORE_ROWS; r++) {
        fprintf(f, "%d %d %d\n", mat[r][0], mat[r][1], mat[r][2]);
    }
    fclose(f);
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
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        float scale = (float)sw / (float)WIDTH; // escala relativa à largura base
        int titleFont = (int)fmaxf(20, 40 * scale);
        int itemFont  = (int)fmaxf(12, 25 * scale);

        BeginDrawing();
        // fundo cobrindo toda a janela
        if (menuTexture.id != 0) {
            Rectangle src = { 0.0f, 0.0f, (float)menuTexture.width, (float)menuTexture.height };
            Rectangle dst = { 0.0f, 0.0f, (float)sw, (float)sh };
            DrawTexturePro(menuTexture, src, dst, (Vector2){0,0}, 0.0f, WHITE);
        } else ClearBackground(RAYWHITE);

        // centraliza textos
        const char *title = "SEA CANNON";
        int wTitle = MeasureText(title, titleFont);
        DrawText(title, sw/2 - wTitle/2, (int)(sh*0.12f), titleFont, WHITE);

        const char *opt1 = "1 - Começar o jogo";
        const char *opt2 = "2 - Ver pontuações";
        const char *opt3 = "3 - Sair";
        const char *opt4 = "4 - Ajuda (comandos)";
        int gap = (int)(30 * scale);
        int baseY = sh/2 - gap;
        DrawText(opt1, sw/2 - MeasureText(opt1, itemFont)/2, baseY, itemFont, WHITE);
        DrawText(opt2, sw/2 - MeasureText(opt2, itemFont)/2, baseY + gap, itemFont, WHITE);
        DrawText(opt3, sw/2 - MeasureText(opt3, itemFont)/2, baseY + gap*2, itemFont, WHITE);
        DrawText(opt4, sw/2 - MeasureText(opt4, itemFont)/2, baseY + gap*3, itemFont, WHITE);

        EndDrawing();
        if (IsKeyPressed(KEY_ONE)) return 1;
        if (IsKeyPressed(KEY_TWO)) return 2;
        if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_ESCAPE)) return 3;
        if (IsKeyPressed(KEY_FOUR))
        return 4;
    }
    return 3;
}

void mostrarScoresTela(ListaScore* inicio) {
    // monta a matriz a partir da lista
    int mat[SCORE_ROWS][SCORE_COLS];
    buildScoresMatrix(inicio, mat);

    while (!WindowShouldClose()) {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        float scale = (float)sw / (float)WIDTH;
        int titleFont = (int)fmaxf(18, 34 * scale);
        int entryFont = (int)fmaxf(12, 20 * scale);

        BeginDrawing();
        // fundo esticado para toda a tela
        if (scoreTexture.id != 0) {
            Rectangle src = { 0.0f, 0.0f, (float)scoreTexture.width, (float)scoreTexture.height };
            Rectangle dst = { 0.0f, 0.0f, (float)sw, (float)sh };
            DrawTexturePro(scoreTexture, src, dst, (Vector2){0,0}, 0.0f, WHITE);
        } else {
            ClearBackground(RAYWHITE);
        }

        // título centralizado
        const char *title = "===== SCOREBOARD (Matriz) =====";
        DrawText(title, sw/2 - MeasureText(title, titleFont)/2, (int)(sh*0.06f), titleFont, WHITE);

        // cabeçalho de colunas
        const char *h0 = "PONTOS";
        const char *h1 = "DINHEIRO";
        const char *h2 = "TEMPO";
        int colGap = (int)(140 * scale);
        int cx = sw/2;
        int x0 = cx - colGap;
        int x1 = cx;
        int x2 = cx + colGap;
        int y = (int)(sh * 0.16f);
        DrawText(h0, x0 - MeasureText(h0, entryFont)/2, y, entryFont, YELLOW);
        DrawText(h1, x1 - MeasureText(h1, entryFont)/2, y, entryFont, YELLOW);
        DrawText(h2, x2 - MeasureText(h2, entryFont)/2, y, entryFont, YELLOW);

        // linhas da matriz
        int lineGap = (int)fmaxf(22, 30 * scale);
        y += lineGap;
        for (int r = 0; r < SCORE_ROWS; r++) {
            char s0[32], s1[32], s2[32];
            snprintf(s0, sizeof(s0), "%d", mat[r][0]);
            snprintf(s1, sizeof(s1), "%d", mat[r][1]);
            snprintf(s2, sizeof(s2), "%d", mat[r][2]);
            DrawText(s0, x0 - MeasureText(s0, entryFont)/2, y, entryFont, WHITE);
            DrawText(s1, x1 - MeasureText(s1, entryFont)/2, y, entryFont, WHITE);
            DrawText(s2, x2 - MeasureText(s2, entryFont)/2, y, entryFont, WHITE);
            y += lineGap;
        }

        // instrução
        const char *instr = "Pressione ENTER ou ESPACO para voltar";
        DrawText(instr, sw/2 - MeasureText(instr, entryFont)/2, sh - (int)(40 * scale), entryFont, WHITE);

        EndDrawing();

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) break;
    }

    // salva a versão em matriz no arquivo de scores padrão
    salvarScoresComoMatriz(inicio);
}

// tela de ajuda / comandos
void mostrarAjudaTela(void) {
    while (!WindowShouldClose()) {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        float scale = (float)sw / (float)WIDTH;
        int titleFont = (int)fmaxf(18, 34 * scale);
        int entryFont = (int)fmaxf(12, 18 * scale);

        BeginDrawing();
        ClearBackground(BLACK);
        const char *title = "COMANDOS";
        DrawText(title, sw/2 - MeasureText(title, titleFont)/2, (int)(sh*0.08f), titleFont, WHITE);

        const char *lines[] = {
            "MOVIMENTACAO: W (frente) | A (gira esquerda) | D (gira direita)",
            "ATIRAR: Q",
            "TROCAR MUNICAO: 1 - Normal | 2 - Pesada | 3 - Explosiva",
            "RECARREGAR/RESET MUNICAO: R",
            "PAINEL/MENU: ENTER/SPACE para confirmar | ESC para voltar/sair",
            "NA TELA: 2 - Ver pontuacoes | 3 - Sair | 4 - Ajuda",
            "OBS: Balas: Q, inimigos e boss sao colisivos via bounding boxes"
        };

        int y = (int)(sh * 0.18f);
        int gap = (int)fmaxf(20, 28 * scale);
        for (int i = 0; i < sizeof(lines)/sizeof(lines[0]); i++) {
            DrawText(lines[i], sw/2 - MeasureText(lines[i], entryFont)/2, y, entryFont, LIGHTGRAY);
            y += gap;
        }

        const char *instr = "Pressione ENTER, ESPACO ou ESC para voltar";
        DrawText(instr, sw/2 - MeasureText(instr, entryFont)/2, sh - (int)(60 * scale), entryFont, WHITE);
        EndDrawing();

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ESCAPE)) break;
    }
}

void telaGameOver(void) {
    while (!WindowShouldClose()) {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        int titleFont = (int)fmaxf(20, 48.0f * ((float)sw / (float)WIDTH));
        int instrFont  = (int)fmaxf(12, 20.0f * ((float)sw / (float)WIDTH));
        BeginDrawing();
        ClearBackground(BLACK);
        const char *title = "GAME OVER";
        DrawText(title, sw/2 - MeasureText(title, titleFont)/2, sh/2 - titleFont, titleFont, RED);
        const char *instr = "Pressione ENTER ou ESPACO para voltar ao menu";
        DrawText(instr, sw/2 - MeasureText(instr, instrFont)/2, sh/2 + (int)(24.0f * ((float)sw / (float)WIDTH)), instrFont, WHITE);
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
    // modo de câmera removido — uso fixo em 1ª pessoa

    // Carrega modelos/texturas/shaders
    Model mapa = LoadModel("models/mar1.glb");
    Model modeloPlayer = LoadModel("models/.glb");
    Model modeloInimigo = LoadModel("models/barco.glb");
    Model modeloBoss = LoadModel("models/boss.glb");

    // --- AUDIO: inicializa e carrega sons/music ---
    InitAudioDevice();
    Sound shotSound = LoadSound("audio/canhao.mp3");
    Sound hitSound  = LoadSound("audio/hit.mp3");
    Music bgMusic   = LoadMusicStream("audio/som_Mar.mp3");

    //Music fundoMenuMusic = LoadMusicStream("audio/fundo.mp3");
    SetSoundVolume(shotSound, 2.0f);
    SetSoundVolume(hitSound, 2.0f);
    SetMusicVolume(bgMusic, 0.7f);
    PlayMusicStream(bgMusic); // começa a tocar em loop
 
    // Player init
    Player player = {0};
    player.pos = (Vector3){0, 1, 0}; strcpy(player.nome, "Player");
    player.score = 0; player.hp = MAX_HP;
    player.municao[MUNI_NORMAL] = 50; player.municao[MUNI_PESADA] = 20; player.municao[MUNI_EXPLOSIVA] = 5;
    player.tipo_muni = MUNI_NORMAL; player.modelo = modeloPlayer; player.yaw = 0.0f;
    player.fireTimer = 0.0f;
    player.fireCooldown = 5.0f; // 5 segundos de delay entre tiros

    // Inimigos
    int totalInimigos = 5;
    Inimigo inimigos[5];
    for (int i = 0; i < totalInimigos; i++) {
        inimigos[i].pos = (Vector3){ GetRandomValue(-50,50), 1.0f, GetRandomValue(-50,50) };
        inimigos[i].speed = 0.05f;
        inimigos[i].hp = 30;
        inimigos[i].alive = true;
        inimigos[i].modelo = modeloInimigo;
        inimigos[i].fireRate = 10.0f;         // 10 segundos entre tiros
        inimigos[i].fireTimer = inimigos[i].fireRate; // opcional: espera 10s antes do 1º tiro
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

        // atualiza cooldown do jogador
        if (player.fireTimer > 0.0f) player.fireTimer -= dt;
        float moveSpeed = 5.0f * dt;
        float turnSpeed = 120.0f * dt;
        float yawRad = player.yaw * (PI / 180.0f);
        if (IsKeyDown(KEY_W)) { player.pos.x += sinf(yawRad) * moveSpeed; player.pos.z += cosf(yawRad) * moveSpeed; }
       
        if (IsKeyDown(KEY_D)) player.yaw -= turnSpeed;
        if (IsKeyDown(KEY_A)) player.yaw += turnSpeed;

        if (IsKeyPressed(KEY_ONE)) player.tipo_muni = MUNI_NORMAL;
        if (IsKeyPressed(KEY_TWO)) player.tipo_muni = MUNI_PESADA;
        if (IsKeyPressed(KEY_THREE)) player.tipo_muni = MUNI_EXPLOSIVA;
        if (IsKeyPressed(KEY_R)) {
            int cap = 50; // default para MUNI_NORMAL
            switch (player.tipo_muni) {
                case MUNI_NORMAL:    cap = 50; break;
                case MUNI_PESADA:    cap = 20; break;
                case MUNI_EXPLOSIVA: cap = 5;  break;
            }
            player.municao[player.tipo_muni] = cap;
        }
        // tecla C removida (somente 1ª pessoa)

        // CÂMERA: 1ª PESSOA - posição no casco/olhos e target à frente conforme yaw
        camera.position = (Vector3){ player.pos.x, player.pos.y + 2.0f, player.pos.z };
        camera.target = (Vector3){ player.pos.x + sinf(yawRad) * 5.0f, player.pos.y + 2.0f, player.pos.z + cosf(yawRad) * 5.0f };
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
        // player atira apenas se tiver munição e cooldown zerado
        if (IsKeyPressed(KEY_Q) && player.municao[player.tipo_muni] > 0 && player.fireTimer <= 0.0f) {
            player.municao[player.tipo_muni]--;
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!balas[i].active) {
                    balas[i].active = true;
                    // spawn na frente do player (considerando yaw)
                    balas[i].pos = (Vector3){ player.pos.x + sinf(yawRad)*1.5f, player.pos.y + 0.6f, player.pos.z + cosf(yawRad)*1.5f };
                    balas[i].dir = (Vector3){ sinf(yawRad), 0.0f, cosf(yawRad) };
                    balas[i].dano = dano;
                    PlaySound(shotSound);
                    break;
                }
            }
            // inicia cooldown após o tiro
            player.fireTimer = player.fireCooldown;
        }

        // Atualiza balas do jogador (usa dt)
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (!balas[i].active) continue;
            balas[i].pos.x += balas[i].dir.x * BULLET_SPEED * dt;
            balas[i].pos.y += balas[i].dir.y * BULLET_SPEED * dt;
            balas[i].pos.z += balas[i].dir.z * BULLET_SPEED * dt;
            if (fabs(balas[i].pos.x) > 2000 || fabs(balas[i].pos.z) > 2000) { balas[i].active = false; continue; }

            // colisão com inimigos
            for (int j = 0; j < totalInimigos; j++) {
                if (!inimigos[j].alive) continue;
                BoundingBox bbInimigo = TransformedBBox(GetModelBoundingBox(inimigos[j].modelo), inimigos[j].pos, 2.0f);
                if (CheckCollisionBoxSphere(bbInimigo, balas[i].pos, 0.5f)) {
                    inimigos[j].hp -= balas[i].dano; balas[i].active = false;
                    PlaySound(hitSound); // efeito de impacto/explosão
                    if (inimigos[j].hp <= 0) { inimigos[j].alive = false; player.score += 100; }
                    break;
                }
            }
            // colisão com boss
            //if (balas[i].active && CheckCollisionBoxSphere(bbBoss, balas[i].pos, 0.5f))
//→ se a bala está ativa e a esfera (centro = balas[i].pos, raio = 0.5) colide com o bounding box do boss.
//boss.hp -= balas[i].dano; balas[i].active = false;
//→ aplica o dano do projétil ao boss e desativa a bala (não será processada/desenhada).
//if (boss.hp <= 0) { boss.hp = 0; player.score += 10000; break; }
//→ se a vida do boss chegou a zero ou menos, zera o HP (sem negativos), adiciona pontos ao jogador e faz break.
            if (balas[i].active && CheckCollisionBoxSphere(bbBoss, balas[i].pos, 0.5f)) {
                boss.hp -= balas[i].dano; balas[i].active = false;
                if (boss.hp <= 0) { boss.hp = 0; player.score += 10000; break; }
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
        //Atualiza posição de cada bala inimiga usando direção * velocidade * dt.
        //Desativa a bala se sair muito longe (checa x e z vs 2000).
        //Verifica colisão com o jogador usando ver_batida(...); se bater reduz HP do jogador e desativa a bala.
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (!enemyBullets[i].active) continue;
            enemyBullets[i].pos.x += enemyBullets[i].dir.x * BULLET_SPEED * dt;
            enemyBullets[i].pos.y += enemyBullets[i].dir.y * BULLET_SPEED * dt;
            enemyBullets[i].pos.z += enemyBullets[i].dir.z * BULLET_SPEED * dt;
            // esse 2000 é o limite do mapa (fora disso, desativa a bala)
            if (fabs(enemyBullets[i].pos.x) > 2000 || fabs(enemyBullets[i].pos.z) > 2000) { 
                enemyBullets[i].active = false; continue; 
            }
            if (ver_batida(enemyBullets[i].pos, 0.3f, player.pos, PLAYER_SIZE)) {
                 player.hp -= enemyBullets[i].dano; enemyBullets[i].active = false; 
                }
        }

        // Espumas baseadas em movimento
        //moved é a distância percorrida (float).
        //moved * 100.0f escala esse valor para obter um número proporcional ao movimento.
        //fminf(4.0f, moved*100.0f) limita esse valor máximo a 4.0f.
        //fmaxf(1.0f, ...) garante que o mínimo seja 1.0f.
        //(int) converte para inteiro (trunca), resultado fica em pawnCount.
        //Ou seja: pawnCount = número de partículas (ou "espumas") a spawnar, proporcional ao movimento, sempre entre 1 e 4.
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
        // Atualiza streaming da música
        UpdateMusicStream(bgMusic);

        // DRAW
        BeginDrawing();
        ClearBackground(SKYBLUE);
        BeginMode3D(camera);

        BeginShaderMode(shaderAgua);
        for (int i = 0; i < 3; i++) {
            float offset = sinf(shaderTime * 0.5f + i) * 0.2f;
            DrawPlane((Vector3){0, -0.5f + offset, 0}, (Vector2){MAP_PLANE_SIZE, MAP_PLANE_SIZE}, (i==0)?BLUE:(Color){0,100,200,100});
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

        // HUD background for HP / Score / Ammo + cooldown bar
        int hudX = 6, hudY = 6, hudW = 260, hudH = 180;
        DrawRectangle(hudX, hudY, hudW, hudH, WHITE);
        DrawRectangleLines(hudX, hudY, hudW, hudH, GRAY);

        DrawText(TextFormat("HP: %d", player.hp), 10, 10, 20, BLACK);
        DrawText(TextFormat("Score: %d", player.score), 10, 40, 20, DARKGRAY);
        Color cores[] = {YELLOW, ORANGE, RED};
        const char* nomes[] = {"Normal","Pesada","Explosiva"};
        for (int i = 0; i < 3; i++) {
            Color cor = (i==player.tipo_muni)?cores[i]:GRAY;
            DrawText(TextFormat("%d: %s x%d", i+1, nomes[i], player.municao[i]), 10, 70 + i*25, 18, cor);
        }

        // Cooldown bar (player.fireTimer / player.fireCooldown)
        float cd = player.fireCooldown;
        float remaining = player.fireTimer;
        if (cd <= 0.0f) cd = 1.0f;
        float progress = 1.0f - (remaining / cd); // 0 = just fired, 1 = ready
        if (progress < 0.0f) progress = 0.0f;
        if (progress > 1.0f) progress = 1.0f;

        int barX = 10;
        int barW = 240;
        int barY = 70 + 3*25 + 10; // below ammo lines
        int barH = 18;
        // background
        DrawRectangle(barX - 4, barY - 4, barW + 8, barH + 8, Fade(LIGHTGRAY, 0.8f));
        // empty bar
        DrawRectangle(barX, barY, barW, barH, GRAY);
        // filled portion
        Color fill = (progress >= 1.0f) ? GREEN : ORANGE;
        DrawRectangle(barX, barY, (int)(barW * progress), barH, fill);
        // border and text
        DrawRectangleLines(barX, barY, barW, barH, BLACK);
        if (remaining > 0.01f) {
            DrawText(TextFormat("Cooldown: %.1fs", remaining), barX, barY - 20, 14, DARKGRAY);
        } else {
            DrawText("Cooldown: Ready", barX, barY - 20, 14, DARKGREEN);
        }

        if (boss.hp > 0) {
            DrawRectangle(WIDTH/2 - 150, 10, 300, 20, DARKGRAY);
            float pct = (float)boss.hp / MAX_HP_BOSS;
            DrawRectangle(WIDTH/2 - 150, 10, (int)(300 * pct), 20, RED);
            DrawText(TextFormat("BOSS: %d / %d", boss.hp, MAX_HP_BOSS), WIDTH/2 - 70, 12, 16, WHITE);
        }
        DrawText("Câmera: 1ª Pessoa", 10, HEIGHT - 30, 16, DARKGRAY);

        EndDrawing();
    }

    // Cleanup
    UnloadShader(shaderAgua);
    UnloadModel(mapa);
    UnloadModel(modeloPlayer);
    UnloadModel(modeloInimigo);
    UnloadModel(modeloBoss);

    // stop/unload audio
    StopMusicStream(bgMusic);
    UnloadMusicStream(bgMusic);
    UnloadSound(shotSound);
    UnloadSound(hitSound);
    CloseAudioDevice();

    int totalMuni = player.municao[0] + player.municao[1] + player.municao[2];
    add_ordenado_score(scoreBoard, player.score, totalMuni, tempoJogo/60);
    salvarScores(*scoreBoard);

    if (boss.hp <= 0) {
        // tela de vitória centralizada
        while (!WindowShouldClose()) {
            int sw = GetScreenWidth(), sh = GetScreenHeight();
            int titleFont = (int)fmaxf(24, 56.0f * ((float)sw / (float)WIDTH));
            int instrFont = (int)fmaxf(12, 20.0f * ((float)sw / (float)WIDTH));
            BeginDrawing();
            ClearBackground(BLACK);
            const char *msg = "VITÓRIA!";
            DrawText(msg, sw/2 - MeasureText(msg, titleFont)/2, sh/2 - titleFont, titleFont, GREEN);
            const char *instr = "Pressione ENTER ou ESPACO para voltar";
            DrawText(instr, sw/2 - MeasureText(instr, instrFont)/2, sh/2 + (int)(28.0f * ((float)sw / (float)WIDTH)), instrFont, WHITE);
            EndDrawing();
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) break;
        }
    } else {
        telaGameOver();
    }

    return 0;
}

// ───────── main ─────────
int main(void) {
    
    InitWindow(WIDTH, HEIGHT, "Navio 3D");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    srand(time(NULL));

    // carrega imagem do menu (coloque images/menu.png)
    if (FileExists("img/main.png")) menuTexture = LoadTexture("img/main.png");
    else { menuTexture = (Texture2D){0}; printf("images/menu.png not found\n"); }
    // carrega imagem para o painel/tabela de scores
    if (FileExists("img/main2.png")) scoreTexture = LoadTexture("img/main2.png");
    else { scoreTexture = (Texture2D){0}; /* opcional: printf("img/score.png not found\n"); */ }

    ListaScore* scoreBoard = NULL;
    carregarScores(&scoreBoard);

    while (!WindowShouldClose()) {
        int opcao = menu();
        if (opcao == 1) jogar(&scoreBoard);
        else if (opcao == 2) mostrarScoresTela(scoreBoard);
        else if (opcao == 4) mostrarAjudaTela();
        else if (opcao == 3) break;
    }

    salvarScores(scoreBoard);
    liberarScores(&scoreBoard);
    if (menuTexture.id != 0) UnloadTexture(menuTexture);
    if (scoreTexture.id != 0) UnloadTexture(scoreTexture);
    CloseWindow();
    return 0;
}

