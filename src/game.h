#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include <stdbool.h>

/*SCREEN SIZE AND HOW MANY ZOMBIES AND HOW MANY ZOMBIES  WE ALLOW AT ONCE */



#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 540
#define MAX_ZOMBIES 6

/*THIS TELLS THE GAME WHICH 'SCREEN' IS CURRENTLY SHOWING */

typedef enum GameScreen {
    SCREEN_MENU=0,
    SCREEN_LEVEL1,
    SCREEN_LEVEL2,
    SCREEN_LEVEL3,
    SCREEN_GAMEOVER,
    SCREEN_WIN
} GameScreen;

/*NOW COMES THE EVERYTHING OF THE PLAYER CHARACTER*/


typedef struct Player {

    Vector2 position; /*player in (x,y) coordinate*/
    Vector2 size; /*how big the player's box is*/
    float speed;
    bool alive;
    int health;
    float hitCooldown; /* stops one zombie hitting us many times per second */


} Zombie;

/*This holds everything to remember like coins players health.main.c creates and every level function gets a pointer to it*/


typedef struct GameData {
    Player  player;
    Zombie zombies[MAX_ZOMBIES];
    int zombieCount;
    int  totalCoins;
    float levelTimer;
    GameScreen currentScreen;
} GameData;

#endif
