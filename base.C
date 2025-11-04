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
void mover_em_direcao(Vector2 * origem,Vector destino,float speed ){
    float dx= destino.X-origem->X;
    float dy= destino.Y-origem->Y;
    float dist=sqrt(dx*dx+ dy*dy)
    if(dist>0){
        origem->X= (dx/dist)*speed;
        origem->y= (dy/dist)*speed;
    }
}