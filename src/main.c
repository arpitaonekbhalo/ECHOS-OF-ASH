/* ECHOES OF ASH
   main.c  -  window, the game loop, and the scene switcher.
   

   The whole game is ONE loop:
       input  ->  update the current scene  ->  draw it */
#include "game.h"

GameData game;
Player   player;

void GoToScene(Scene s)
{
    game.scene = s;
    if (s == SCENE_LEVEL1 || s == SCENE_LEVEL2 || s == SCENE_LEVEL3)
        game.levelTime = 0.0f;

    switch (s) {
        case SCENE_LEVEL1: Level1_Init(); break;
        case SCENE_LEVEL2: Level2_Init(); break;
        case SCENE_LEVEL3: Level3_Init(); break;
        default: break;
    }
}

int main(void)
{
    InitWindow(SCREEN_W, SCREEN_H, "ECHOES OF ASH");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);          /* ESC must not kill the game */

    /* Audio stays off so the game runs with zero asset files.
       Uncomment once assets/sounds exists.
       InitAudioDevice(); */

    FxReset();
    game.coins = 0;
    game.deaths = 0;
    game.level = 1;
    GoToScene(SCENE_MENU);

    while (!WindowShouldClose() && !game.quitRequested) {
        float raw = GetFrameTime();
        float dt;
        if (raw > 0.05f) raw = 0.05f;   /* stops physics exploding on a lag spike */

        dt = FxTick(raw);               /* returns 0 while hitstop is running */

        if (game.msgTimer > 0.0f) game.msgTimer -= raw;

        switch (game.scene) {
            case SCENE_MENU:     Menu_Update(raw);    break;
            case SCENE_STORY:    Story_Update(raw);   break;
            case SCENE_LEVEL1:   Level1_Update(dt);   break;
            case SCENE_LEVEL2:   Level2_Update(dt);   break;
            case SCENE_LEVEL3:   Level3_Update(dt);   break;
            case SCENE_RESULTS:  Results_Update(raw); break;
            case SCENE_GAMEOVER: GameOver_Update(raw);break;
            case SCENE_VICTORY:  Victory_Update(raw); break;
            default: break;
        }

        BeginDrawing();
            ClearBackground((Color){ 10, 9, 12, 255 });

            switch (game.scene) {
                case SCENE_MENU:     Menu_Draw();     break;
                case SCENE_STORY:    Story_Draw();    break;
                case SCENE_LEVEL1:   Level1_Draw();   break;
                case SCENE_LEVEL2:   Level2_Draw();   break;
                case SCENE_LEVEL3:   Level3_Draw();   break;
                case SCENE_RESULTS:  Results_Draw();  break;
                case SCENE_GAMEOVER: GameOver_Draw(); break;
                case SCENE_VICTORY:  Victory_Draw();  break;
                default: break;
            }

            DrawMessage();
        EndDrawing();
    }

    /* CloseAudioDevice(); */
    CloseWindow();
    return 0;
}