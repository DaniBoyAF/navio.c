# Navio 3D — README

Resumo
- Jogo em C usando raylib. Projeto single-file principal: `main.c`.
- Implementa: structs, ponteiros, alocação dinâmica, listas encadeadas, matrizes, leitura/escrita em arquivo (scores).

Estrutura e onde olhar (principais pontos em main.c)
- Menu e telas:
  - menu(): mostra opções (Iniciar, Scores, Ajuda, Sair).
  - mostrarScoresTela(): desenha o scoreboard e salva matriz de scores (`scores_matrix.txt`).
  - mostrarAjudaTela(): lista comandos.
- Jogo:
  - jogar(): loop principal do jogo, render, física, colisões.
  - Hitboxes: BoundingBox calculados com `TransformedBBox(GetModelBoundingBox(...), pos, scale)`.
- Scores:
  - Lista ligada `ListaScore` com funções `carregarScores()`, `salvarScores()`, `add_ordenado_score()`, `liberarScores()`.
  - `buildScoresMatrix()` / `salvarScoresComoMatriz()` → cria e grava matriz 10x3.
- Matrizes:
  - `mapGrid[20][20]` (se aplicada) — minimapa simples, ou a matriz temporária de scores em `buildScoresMatrix()`.
- Debug:
  - Flag `showHitboxes` (ativa via tecla F1 se implementada) para desenhar caixas wireframe.

Dependências (Linux)
- gcc, make, raylib (instale via source ou pacote do sistema).
- Pacotes recomendados: build-essential, cmake, libglfw3-dev, libopenal-dev, libasound2-dev, libx11-dev, libxrandr-dev, libxcursor-dev, libxi-dev

Como compilar (exemplo Ubuntu)
```bash
# instale raylib (se não tiver)
git clone https://github.com/raysan5/raylib.git
cd raylib/src
make PLATFORM=PLATFORM_DESKTOP
sudo make install

# compilar o projeto (no diretório do Makefile)
make
# ou
mingw32-make jogo   # no Windows/mingw
```

Como executar
Terminal
Comando para Windows:
C:\raylib\w64devkit\bin\mingw32-make.exe RAYLIB_PATH=C:/raylib/raylib PROJECT_NAME=main OBJS=main.c BUILD_MODE=DEBUG
Comando para Linux:
gcc main.c -o main -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

E depois vc tem q ir pra a pasta e abri o arquivo jogo.exe ou main.exe.

Controles principais
- Menu: 1 = jogar, 2 = scores, 3 = sair, 4 = ajuda
- Movimento: W (frente), A/D (girar)
- Atirar: Q
- Trocar munição: 1 / 2 / 3
- Pausar/confirmar/voltar: ENTER, SPACE, ESC
- Debug hitboxes: F1 (se habilitado)

Arquivos / assets necessários
- models/*.glb (player, inimigos, boss) — corrija os caminhos em `LoadModel(...)`.
- img/*.png para menu e scores (opcional).
- audio/* (música/efeitos).
- shaders/agua.fs (se usado).
- `scores.txt`, `scores_matrix.txt` (serão criados no diretório do jogo).

Dicas rápidas / problemas comuns
- Se o conteúdo aparece pequeno no canto, verifique `InitWindow(...)` e se o desenho usa `WIDTH/HEIGHT` ou `GetScreenWidth()/GetScreenHeight()`.
- Erro de link "Permission denied": feche o executável em execução e remova o arquivo antes de compilar (tasklist/taskkill no Windows).
- Se `fmaxf` der erro, adicione `#include <math.h>` (já presente) ou substitua por ternário.
- Se modelos não aparecem, ajuste os nomes/paths em `main.c`.

Editar configurações
- Resolução base: altere `#define WIDTH` e `#define HEIGHT` no topo de `main.c`.
- Ajustar hitbox do barco: altere o scale ou expanda `bbPlayer.min/max` após `TransformedBBox`.


Precisa de algo automático?
- Posso adicionar:
  - matriz global de scores (dinâmica),
  - debug visual de hitboxes,
  - script de build adaptado para Linux/Windows.
Video teste do jogo:


https://github.com/user-attachments/assets/a3299bd2-0083-4d9e-8a2a-a28d05952d43


```
