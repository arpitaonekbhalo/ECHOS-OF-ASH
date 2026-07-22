#include "level1.h"
#include "player.h"
#include "zombie.h"
#include <math.h>
static Texture2D levelBackground;
/*player is locked in a room.Must pick a lock*/
#define STAGES_TO_WIN 3
#define TIME_LIMIT 45.0f
#define DIAL_SPEED 2.5f
static Rectangle roomBounds;
static int stagesCompleted;
static float timer;
static float targetMin, targetMax;
static bool levelFinished;
/*player picks a random safe zone */
static void PickNewTargetZone(void)
{
    int start = GetRandomValue(0, 0);
    targetMin = start / 100.0f;
    targetMax = (start + 15) / 100.0f;
}
void Level1_Init(GameData *gd)
{
  levelBackground = LoadTexture("assets/level1_bg.png");

  TraceLog(LOG_INFO, "Texture ID: %d", levelBackground.id);
  TraceLog(LOG_INFO, "BG size: %d x %d", levelBackground.width, levelBackground.height);
  roomBounds = (Rectangle){40,90, SCREEN_WIDTH - 80, SCREEN_HEIGHT - 130};
  Player_Init(&gd->player, (Vector2){SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f});
  gd->zombieCount = 0;
  Zombie_Spawn(gd->zombies, &gd->zombieCount, (Vector2){100,120}, 60.0f);
  Zombie_Spawn(gd->zombies, &gd->zombieCount, (Vector2){800,400}, 70.0f);
  stagesCompleted = 0;
  timer = TIME_LIMIT;
  levelFinished = false;
  PickNewTargetZone();
}
void Level1_Update(GameData *gd, float dt)
{
    if (levelFinished) return;
    timer -= dt;
    Player_Update(&gd->player, roomBounds, dt);
    Zombie_UpdateAll(gd->zombies, gd->zombieCount, &gd->player, roomBounds, dt);
    if(IsKeyPressed(KEY_SPACE))
    {
        /*location of the marker,0=left edge,1=right edge*/
        float dialValue = (sinf((float)GetTime() * DIAL_SPEED) + 1.0f) / 2.0f;
        if(dialValue>= targetMin && dialValue<= targetMax)
        {
            /*successful*/
            stagesCompleted++;
            if(stagesCompleted>= STAGES_TO_WIN)
            {
                int bonus = (int)(timer * 4.0f);/*quicker escape,more coins*/
                if (bonus < 0) bonus = 0;
                gd->totalCoins += bonus;
                levelFinished = true;
                gd->currentScreen = SCREEN_WIN;
                return;            
            }
            PickNewTargetZone();
        }
        else
        {
            /*failed-time penalty*/
            timer -= 3.0f;     
        }
    }
    if(gd->player.health <= 0 || timer <= 0.0f)
    {
        gd->totalCoins -= 20;
        if(gd->totalCoins < 0) gd->totalCoins = 0;
        levelFinished = true;
        gd->currentScreen = SCREEN_GAMEOVER;
    }
}
void Level1_Draw(GameData *gd)
{
    ClearBackground(BLACK);
    DrawTexturePro(
    levelBackground,
    (Rectangle){0, 0, levelBackground.width, levelBackground.height},
    (Rectangle){0, 29, SCREEN_WIDTH, SCREEN_HEIGHT},
    (Vector2){0, 0},
    0,
    WHITE
    );
    DrawRectangleLinesEx(roomBounds, 3, GRAY);
    DrawText("LEVEL 1 - ESCAPE THE ROOM", 40, 20, 22, RED);
    DrawText(TextFormat("Coins: %d", gd->totalCoins), 40, 50, 18, GOLD);
    DrawText(TextFormat("Health: %d", gd->player.health), 780, 50, 18, GREEN);
    DrawText(TextFormat("Time left: %.1f", timer), SCREEN_WIDTH /2 - 70, 50, 18, WHITE);
    Zombie_DrawAll(gd->zombies, gd->zombieCount);
    Player_Draw(&gd->player);
    /*draw the marker at the bottom of the screen*/
    int barX = SCREEN_WIDTH / 2 - 150, barY = SCREEN_HEIGHT - 60, barW = 300, barH = 20;
    DrawRectangle(barX, barY, barW, barH, DARKGRAY);
    DrawRectangle(barX + (int)(targetMin * barW), barY, (int)((targetMax - targetMin) * barW), barH, GREEN);
    float dialValue = (sinf((float)GetTime() * DIAL_SPEED) + 1.0f) / 2.0f;
    int markerX = barX + (int)(dialValue * barW);
    DrawRectangle(markerX - 2, barY - 6, 4, barH + 12, WHITE);
    DrawText(TextFormat("Stage %d / %d - Press SPACE when marker is in green zone", stagesCompleted, STAGES_TO_WIN), barX - 120, barY - 30, 16, LIGHTGRAY);
}