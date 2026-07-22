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
/*in 3d,"position" needs 3 numbers (x,y,z) x=left/right ,z=forward/back(the floor plane),y=up/down(height) for a character walking on flat ground,y usually barely changes*/


typedef struct Player {

    // collision box
    Vector3 position;
    float radius; /* how wide the character's collision*/
    float height;
    float speed;
    bool alive;
    int health;
    int maxHealth;

   // Texture2D texture;

} Player;

typedef struct Zombie {

    Vector3 position;
    float radius;
    float height;
    float speed;
    bool alive;
    int health;
    float hitCooldown;

   // Texture2D texture;

} Zombie;

typedef struct GameData {
    Player  player;
    Zombie zombies[MAX_ZOMBIES];
    int zombieCount;
    int  totalCoins;
    GameScreen currentScreen;
    Camera3D camera; /*the 3d eye watching the scene*/
} GameData;

#endif