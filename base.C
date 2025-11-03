#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
