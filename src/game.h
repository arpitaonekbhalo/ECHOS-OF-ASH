#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include <stdbool.h>

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 540
#define MAX_ZOMBIES 6

typedef enum GameScreen {
    SCREEN_MENU=0,
    SCREEN_LEVEL1,
    SCREEN_LEVEL2,
    SCREEN_LEVEL3,
    SCREEN_GAMEOVER,
    SCREEN_WIN
} GameScreen;

typedef struct Player {

    Vector2 position;
    Vector2 size;
    float speed;
    bool alive;
    int health;
    int maxHealth;
    float hitCooldown;

} Player;

typedef struct Zombie {

    Vector2 position;
    Vector2 size;
    float speed;
    bool alive;
    int health;
    float hitCooldown;

} Zombie;

typedef struct GameData {
    Player  player;
    Zombie zombies[MAX_ZOMBIES];
    int zombieCount;
    int  totalCoins;
    float levelTimer;
    GameScreen currentScreen;
} GameData;

#endif