#include "zombie.h"
#include<math.h>

/*Adds one new zombie to the list, if there's room*/
void Zombie_Spawn(Zombie z[], int *count, Vector2 pos, float speed){
  if(*count >= MAX_ZOMBIES) return;
  z[*count].position =pos;
  z[*count].size = (Vector2){30,30};
  z[*count].speed = speed;
  z[*count].alive = true;
  z[*count].health = 30;
  z[*count].hitCooldown= 0.0f;
  (*count)++;
}

/*Moves every zombie a little bit toward the player(chasing)*/
void Zombie_UpdateAll(Zombie z[], int count, Player *player, Rectangle roomBounds, float dt){
  Vector2 playerCenter={
    player->position.x + player->size.x/2,
    player->position.y + player->size.y/2
  };
  for(int i=0;i<count;i++){
    if(!z[i].alive) continue; /*skips dead zombies*/

    if(z[i].hitCooldown>0.0f) z[i].hitCooldown -= dt;

    Vector2 zCenter ={
      z[i].position.x + z[i].size.x/2,
      z[i].position.y + z[i].size.y/2
    };

    /*direction pointing from zombie toward player*/
    float dx= playerCenter.x -zCenter.x;
    float dy= playerCenter.y -zCenter.y;
    float dist = sqrtf(dx*dx + dy*dy);
    
    if(dist>1.0f){  /*avoid dividing by 0 if they are at the same spot*/
      dx/=dist;
      dy/=dist;
      z[i].position.x += dx*z[i].speed*dt;
      z[i].position.y += dy*z[i].speed*dt;
    }

    /*keep zombie in the room*/
    if(z[i].position.x<roomBounds.x) z[i].position.x = roomBounds.x;
    if(z[i].position.y<roomBounds.y) z[i].position.y = roomBounds.y;
    if(z[i].position.x + z[i].size.x>roomBounds.x+roomBounds.width){
      z[i].position.x=roomBounds.x+roomBounds.width-z[i].size.x;
    }
    if(z[i].position.y + z[i].size.y>roomBounds.y+roomBounds.height){
      z[i].position.y=roomBounds.y+roomBounds.height-z[i].size.y;
    }

    /*if zombie touches player, take damage(but not every single frame)*/
    Rectangle pRect={player->position.x, player->position.y, player->size.x, player->size.y};
    Rectangle zRect={z[i].position.x, z[i].position.y, z[i].size.x, z[i].size.y};
    if(CheckCollisionRecs(pRect, zRect)&&z[i].hitCooldown<=0.0f){
      player->health-=10;
      z[i].hitCooldown = 1.0f;
    }
  }
}

void Zombie_DrawAll(Zombie z[], int count){
  for(int i=0;i<count;i++){
    if(!z[i].alive) continue;
    DrawRectangleV(z[i].position, z[i].size,MAROON);
    DrawRectangleLines((int)z[i].position.x, (int)z[i].position.y, (int)z[i].size.x, (int)z[i].size.y, BLACK);
  }
}