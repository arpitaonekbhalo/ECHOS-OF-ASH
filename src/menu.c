/* menu.c  -  title, results, game over, victory. */
#include "game.h"

/*  TITLE */
void Menu_Update(float dt)
{
    (void)dt;
    if (IsKeyPressed(KEY_ENTER)) {
        game.coins = 0;
        game.deaths = 0;
        game.level = 1;
        game.hasCarry = false;
        Story_ShowIntro();
    }
}

void Menu_Draw(void)
{
    const char *title = "ECHOES OF ASH";
    const char *sub   = "nine days. one base. one way out.";
    int tw = MeasureText(title, 54);
    int sw = MeasureText(sub, 20);

    DrawText(title, SCREEN_W / 2 - tw / 2, 110, 54, (Color){ 214, 58, 58, 255 });
    DrawText(sub,   SCREEN_W / 2 - sw / 2, 180, 20, (Color){ 155, 155, 168, 255 });

    if (ButtonUI((Rectangle){ SCREEN_W / 2 - 130, 260, 260, 54 }, "START  [ENTER]")) {
        game.coins = 0;
        game.deaths = 0;
        game.level = 1;
        game.hasCarry = false;
        Story_ShowIntro();
    }
    if (ButtonUI((Rectangle){ SCREEN_W / 2 - 130, 330, 260, 54 }, "QUIT"))
        game.quitRequested = true;

    DrawText("WASD move    MOUSE aim    LEFT CLICK attack    SHIFT sprint",
             SCREEN_W / 2 - 300, 440, 19, (Color){ 130, 130, 142, 255 });
    DrawText("1 knife   2 pistol   3 shotgun   F interact   HOLD E to inject",
             SCREEN_W / 2 - 300, 468, 19, (Color){ 130, 130, 142, 255 });

    DrawText("INFECTION rises when you're hit and never falls on its own.",
             SCREEN_W / 2 - 285, 530, 18, (Color){ 190, 150, 90, 255 });
    DrawText("Injecting takes 1.5s standing still. Choose your moment.",
             SCREEN_W / 2 - 268, 556, 18, (Color){ 190, 150, 90, 255 });

    DrawVignette(90);
}

/*  RESULTS (after each level)  */
void Results_Update(float dt)
{
    (void)dt;
}

static void StatRow(const char *label, const char *value, int y, bool bold)
{
    Color k = bold ? (Color){ 240, 196, 62, 255 } : (Color){ 155, 155, 168, 255 };
    Color v = bold ? (Color){ 240, 196, 62, 255 } : (Color){ 230, 225, 212, 255 };
    DrawText(label, SCREEN_W / 2 - 230, y, 20, k);
    DrawText(value, SCREEN_W / 2 + 150, y, 20, v);
}

void Results_Draw(void)
{
    const char *title = (game.level >= 3) ? "THE WORLD COMES BACK"
                      : (game.level == 2) ? "ANTIDOTE SECURED"
                                          : "LEVEL 1 CLEAR";
    int y = 210;

    DrawText(title, SCREEN_W / 2 - MeasureText(title, 48) / 2, 120, 48,
             (Color){ 120, 220, 150, 255 });

    StatRow("time",           TextFormat("%.1fs", game.levelTime), y, false); y += 32;
    StatRow("kills",          TextFormat("%d", game.kills),        y, false); y += 32;
    StatRow("times hit",      TextFormat("%d", game.hits),         y, false); y += 32;
    StatRow("peak infection", TextFormat("%d%%", (int)game.peakInfection), y, false); y += 32;
    StatRow("end infection",  TextFormat("%d%%", (int)player.infection),   y, false); y += 40;
    StatRow("TOTAL COINS",    TextFormat("%d", game.coins),        y, true);

    if (game.level >= 3) {
        const char *rank;
        if (game.coins >= 500)      rank = "RANK: GHOST";
        else if (game.coins >= 300) rank = "RANK: OPERATOR";
        else if (game.coins >= 150) rank = "RANK: SURVIVOR";
        else                        rank = "RANK: BARELY BREATHING";
        DrawText(rank, SCREEN_W / 2 - MeasureText(rank, 22) / 2, 556, 22,
                 (Color){ 220, 220, 232, 255 });
        if (ButtonUI((Rectangle){ SCREEN_W / 2 - 130, 600, 260, 54 }, "MENU"))
            GoToScene(SCENE_MENU);
    } else if (game.level == 2) {
        if (ButtonUI((Rectangle){ SCREEN_W / 2 - 130, 600, 260, 54 }, "CONTINUE"))
            Story_ShowCard3();
    } else {
        if (ButtonUI((Rectangle){ SCREEN_W / 2 - 130, 600, 260, 54 }, "CONTINUE"))
            Story_ShowCard2();
    }

    DrawVignette(80);
}

/*  GAME OVER  */
void GameOver_Update(float dt)
{
    (void)dt;
    if (IsKeyPressed(KEY_R)) GoToScene(game.retryScene);
    if (IsKeyPressed(KEY_M)) GoToScene(SCENE_MENU);
}

void GameOver_Draw(void)
{
    const char *title = "YOU DIED";
    int tw = MeasureText(title, 64);

    DrawText(title, SCREEN_W / 2 - tw / 2, 190, 64, (Color){ 198, 40, 40, 255 });
    DrawText(TextFormat("deaths: %d      coins: %d", game.deaths, game.coins),
             SCREEN_W / 2 - 150, 280, 24, RAYWHITE);

    if (ButtonUI((Rectangle){ SCREEN_W / 2 - 130, 340, 260, 56 }, "RETRY  [R]"))
        GoToScene(game.retryScene);
    if (ButtonUI((Rectangle){ SCREEN_W / 2 - 130, 410, 260, 56 }, "MENU  [M]"))
        GoToScene(SCENE_MENU);

    DrawVignette(140);
}

/*  VICTORY (unused - results handles the ending)  */
void Victory_Update(float dt)
{
    (void)dt;
    if (IsKeyPressed(KEY_M)) GoToScene(SCENE_MENU);
}

void Victory_Draw(void)
{
    const char *title = "THE WORLD COMES BACK";
    int tw = MeasureText(title, 50);
    const char *rank;

    if (game.coins >= 500)      rank = "RANK: GHOST";
    else if (game.coins >= 300) rank = "RANK: OPERATOR";
    else if (game.coins >= 150) rank = "RANK: SURVIVOR";
    else                        rank = "RANK: BARELY BREATHING";

    DrawText(title, SCREEN_W / 2 - tw / 2, 180, 50, (Color){ 120, 220, 150, 255 });
    DrawText(TextFormat("final coins: %d", game.coins),
             SCREEN_W / 2 - 110, 270, 26, (Color){ 240, 196, 62, 255 });
    DrawText(rank, SCREEN_W / 2 - MeasureText(rank, 30) / 2, 330, 30,
             (Color){ 220, 220, 232, 255 });

    if (ButtonUI((Rectangle){ SCREEN_W / 2 - 130, 410, 260, 56 }, "MENU  [M]"))
        GoToScene(SCENE_MENU);
}