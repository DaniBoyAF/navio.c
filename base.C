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