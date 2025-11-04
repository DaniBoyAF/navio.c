#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WIDTH 800
#define HEIGHT 450
#define PLAYER_SIZE 32
#define MAX_HP 3

typedef struct {
    char nome[50];
    int score;
    int hp;
    int X;
    int y;
    double tempo;
}Player;
typedef struct {
    int X;
    int Y;
    int hp;
    int speed;
}inimigo;
typedef struct{
    int X;
    int Y;
    int speed;
    int hp;
}boss;
void MoveIni(Inimigo *inimigo){
    inimigo->y+=inimigo->speed;
    if(inimigo->y > HEIGHT){
        inimigo->y=0;
        inimigo->x = GetRandomValue(0,WIDTH- 30);
    }
}