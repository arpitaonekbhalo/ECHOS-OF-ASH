#include "player.h"

/*how fast we are currently moving,so movement can speed up and slow down smoothly instead of snapping*/

static float currentSpeedX=0;
static float currentSpeedY=0;

void Player_Init(Player *p,Vector2 startPos)
{
    p->position = startPos;
    p->size=(Vector2) {28,28};
    p->speed=220.0f;
    p->maxHealth=100;
    p->health=100;

    p->texture = LoadTexture("assets/player.png");

}
void Player_Update(Player *p,Rectangle roomBounds,float dt)
{
    /*keys working direction*/

    float wantX=0;
    float wantY=0;
    if(IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) wantY=-1;
    if(IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) wantY=1;
    if(IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) wantX=-1;
    if(IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) wantX=1;

    /*turn direction into a target speed*/

    float targetSpeedX=wantX*p->speed;
    float targetSpeedY=wantY*p->speed;

    /*smoothly move current speed 
    changing 0.15f-bigger=snappier,smaller=floater*/

    float nudgeAmount=0.15f;
    currentSpeedX=currentSpeedX+(targetSpeedX-currentSpeedX)*nudgeAmount;
    currentSpeedY=currentSpeedY+(targetSpeedY-currentSpeedY)*nudgeAmount;

    /*moving the player */
    p->position.x+=currentSpeedX*dt;
    p->position.y+=currentSpeedY*dt;

    /*not letting the player leave the room*/

    if(p->position.x<roomBounds.x)
    p->position.x=roomBounds.x;
    if(p->position.y<roomBounds.y)
    p->position.y=roomBounds.y;
    if(p->position.x+p->size.x >roomBounds.x+roomBounds.width)
    p->position.x=roomBounds.x+roomBounds.width - p->size.x;
    if(p->position.y +p->size.y>roomBounds.y +roomBounds.height)
    p->position.y=roomBounds.y+roomBounds.height - p->size.y;

}

void Player_Draw(Player *p)
{
    float scale = 1.5f;   //change to adjust the size
    Vector2 drawPos = {
        p->position.x - (p->texture.width * scale - p->size.x) / 2,
        p->position.y - (p->texture.height * scale - p->size.y)
    };

    DrawTextureEx(
        p->texture,
        drawPos,
        0.0f,
        scale,
        WHITE
    );
}