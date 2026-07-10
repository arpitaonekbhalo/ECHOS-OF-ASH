#ifndef ZOMBIE_H
#define ZOMBIE_H
#include "game.h"
void Zombie_Spawn(Zombie z[], int *count, Vector2 pos, float speed);
void Zombie_UpdateAll(Zombie z[], int count, Player *player, Rectangle roomBounds, float dt);
void Zombie_DrawAll(Zombie z[], int count);
#endif