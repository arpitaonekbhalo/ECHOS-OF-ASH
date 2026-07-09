#include "player.h"

/*how fast we are currently moving,so movement can speed up and slow down smoothly instead of snapping*/

static float currentSpeedX=0;
static float currentSpeedY=0;

void Player_Init(Player *p,Vector2 startPos)
{
    p->position = startPos;
    p->size=(Vector2) {28,28}
    p->speed=220.0f;
    p->maxHealth=100;
    p->health=100;

}
void Player_Update(Player *p,Rectangle roomBounds,float dt)
{
    /*keys working direction*/

    float wantX=0;
    float wantY=0;
    if(iskeydown(key_w) || iskeydown(key_up)) wanty=-1;
    if(iskeydown(key_s) || iskeydown(key_down)) wanty=1;
    if(iskeydown(key_a) || iskeydown(key_left)) wantx=-1;
    if(iskeydown(key_d) || iskeydown(key_right)) wantx=1;

    /*turn direction into a target speed*/

    float targetSpeedX=wantX*p->speed;
    float targetSpeedY=wantY*p->speed;
}