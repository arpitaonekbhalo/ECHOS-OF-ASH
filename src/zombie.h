#ifndef ZOMBIE_H
#define ZOMBIE_H
#include "game.h"

void Zombie_Spawn(Zombie z[], int *count, Vector3 pos, float speed);
void Zombie_UpdateAll(Zombie z[], int count, Player *player, Vector3 roomMin, Vector3 roomMax, float dt);
void Zombie_DrawAll(Zombie z[], int count);

#endif