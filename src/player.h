#ifndef PLAYER_H
#define PLAYER_H
#include "game.h"

void Player_Init(Player *p, Vector3 startPos);
void Player_Update(Player *p,Vector3 roomMin,Vector3 roomMax, float dt);
void Player_Draw(Player *p);

#endif