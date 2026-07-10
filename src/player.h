#ifndef PLAYER_H
#define PLAYER_H
#include "game.h"

void Player_Init(Player *p, Vector2 startPos);
void Player_Update(Player *p, Rectangle roomBounds, float dt);
void Player_Draw(Player *p);

#endif