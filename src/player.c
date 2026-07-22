#include "player.h"

/*how fast we are currently moving,so movement can speed up and slow down smoothly instead of snapping IN 3D: X=left/right,Z=forward/back(the floor).Y=up/down
we only ever change x and z here the character stays on the ground*/

static float currentSpeedX=0;
static float currentSpeedZ=0;

void Player_Init(Player *p,Vector3 startPos)
{
    p->position = startPos;
    p->radius=0.4f; /*half a meter wide roughly human sized*/
    p->height=1.8f; /* about 1.8 m tall*/
    p->speed=4.0f;
    p->maxHealth=100;
    p->health=100;

    //p->texture = LoadTexture("assets/player.png");

}
void Player_Update(Player *p,Vector3 roomMin,Vector3 roomMax, float dt);
{
    /*keys working direction*/

    float wantX=0;
    float wantZ=0;
    /* W/UP =MOVE TOWARD -Z (RAYLIB'S CAMERA CONVENTION:FORWARD IS -Z)*/
    if(IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) wantZ=-1;
    if(IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) wantZ=1;
    if(IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) wantX=-1;
    if(IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) wantX=1;

    /*turn direction into a target speed*/

    float targetSpeedX=wantX*p->speed;
    float targetSpeedZ=wantZ*p->speed;

    /*smoothly move current speed 
    changing 0.15f-bigger=snappier,smaller=floater*/

    float nudgeAmount=0.2f;
    currentSpeedX=currentSpeedX+(targetSpeedX-currentSpeedX)*nudgeAmount;
    currentSpeedZ=currentSpeedZ+(targetSpeedZ-currentSpeedZ)*nudgeAmount;

    /*moving the player */
    p->position.x+=currentSpeedX*dt;
    p->position.z+=currentSpeedZ*dt;

    /*not letting the player leave the room*/

    if(p->position.x - p->radius < roomMin.x)
    p->position.x=roomMin.x + p->radius;
    if(p->position.z - p->radius < roomMin.z)
    p->position.z=roomMin.z + p->radius;
    if(p->position.x + p->radius > roomMax.x)
    p->position.x=roomMax.x - p->radius;
    if(p->position.z + p->radius > roomMax.z)
    p->position.z=roomMin.z- p->radius;

}
/*temporary placeholder art: a pink capsule standing for girl*/

void Player_Draw(Player *p)
{
    Vector3 bottom={ p->position.x,p->position.y,p->position.z};
    Vector3 top={ p->position.x,p->position.y+p->height - p->radius*2,p->position.z};
    DrawCapsule(bottom,top,p->radius,12,8,PINK);
    DrawCapsulWires(bottom,top,p->radius,12,8,MAROON);
   
    }

    /*DrawTextureEx(
        p->texture,
        drawPos,
        0.0f,
        scale,
        WHITE
    );*/
