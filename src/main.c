#include "raylib.h"
#include "game.h"
#include "level1.h"

/*for starting or restarting the game from level 1*/

static void restartgame(GameData *gd)
{
    gd->totalCoins=0;
    gd->currentScreen=SCREEN_LEVEL1;
    Level1_Init(gd);
}

int main(void)
{
    InitWindow(SCREEN_WIDTH,SCREEN_HEIGHT,"Echoes of Ash");
    SetTargetFPS(60);

    GameData gd={0};
    gd.currentScreen=SCREEN_MENU;

    while(!WindowShouldClose())
    {
        float dt=GetFrameTime();

        /*current screen features*/

        switch(gd.currentScreen)
        {
            case SCREEN_MENU:
            if(IsKeyPressed(KEY_ENTER))
            restartgame(&gd);
            break;

            case SCREEN_LEVEL1:
            Level1_Update(&gd,dt);
            break;

            case SCREEN_GAMEOVER:
            case SCREEN_WIN:
            if(IsKeyPressed(KEY_ENTER))
            restartgame(&gd);
            break;

            default:
            break;
        }

        /*Frame for the screen*/

        BeginDrawing();
        switch(gd.currentScreen)
        {
            case SCREEN_MENU:
            ClearBackground(BLACK);
            DrawText("ECHOES OF ASH",SCREEN_WIDTH/2 - 170,180,46,RED);
            DrawText("PRESS ENTER TO START",SCREEN_WIDTH/2 - 110,260,20,WHITE);
            DrawText("WASD/ARROWS to move,SPACE TO interact/attack",SCREEN_WIDTH/2 - 190,300,16,GRAY);
            break;

            case SCREEN_LEVEL1:
            Level1_Draw(&gd);
            break;

            case SCREEN_GAMEOVER:
            ClearBackground(BLACK);
            DrawText("YOU DIED",SCREEN_WIDTH/2 -100,200,40,RED);
            DrawText(TextFormat("Coins: %d",gd.totalCoins),SCREEN_WIDTH/2 - 60,260,20,GOLD);
            DrawText("PRESS ENTER TO RETRY",SCREEN_WIDTH/2 - 100,300,18,WHITE);
            break;

            case SCREEN_WIN:
            ClearBackground(BLACK);
            DrawText("YOU SURVIVED" , SCREEN_WIDTH/2 - 140,200,40,GREEN);
            DrawText(TextFormat("Final Coins: %d", gd.totalCoins),SCREEN_WIDTH/2-80,260,20,GOLD);
            DrawText("PRESS ENTER TO PLAY AGAIN",SCREEN_WIDTH/2 - 130,300,18,WHITE);
            break;

            default:
            break;
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}