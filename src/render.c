#include "game.h"
#include <math.h>
#include <string.h>

/* drawing */
void DrawFloor(int cols, int rows)
{
    int x, y;
    DrawRectangle(0, 0, (int)(cols * TILE), (int)(rows * TILE),
                  (Color){ 26, 25, 29, 255 });
    /* faint grid so movement reads clearly */
    for (x = 0; x <= cols; x++)
        DrawRectangle((int)(x * TILE), 0, 1, (int)(rows * TILE),
                      (Color){ 36, 35, 40, 255 });
    for (y = 0; y <= rows; y++)
        DrawRectangle(0, (int)(y * TILE), (int)(cols * TILE), 1,
                      (Color){ 36, 35, 40, 255 });
}

void DrawWalls(Rectangle *walls, int count)
{
    int i;
    for (i = 0; i < count; i++) {
        DrawRectangleRec(walls[i], (Color){ 40, 38, 45, 255 });
        DrawRectangleLinesEx(walls[i], 2.0f, (Color){ 66, 62, 73, 255 });
        DrawRectangle((int)walls[i].x, (int)walls[i].y,
                      (int)walls[i].width, 3, (Color){ 80, 76, 88, 255 });
    }
}

void DrawPlayer(void)
{
    Color body = (Color){ 92, 162, 212, 255 };
    Vector2 c = RectCenter(player.box);

    if (player.hurtTimer > 0.55f) body = (Color){ 240, 100, 100, 255 };

    DrawEllipse((int)c.x, (int)(player.box.y + player.box.height - 1),
                player.box.width * 0.55f, 6.0f, (Color){ 0, 0, 0, 110 });
    DrawRectangleRec(player.box, body);
    DrawRectangleLinesEx(player.box, 2.0f, RAYWHITE);

    /* the weapon in your hands, pointing where you aim */
    if (player.weapon == WEAP_SHOTGUN)
        DrawLineEx(c, (Vector2){ c.x + player.aim.x * 30.0f, c.y + player.aim.y * 30.0f },
                   6.0f, (Color){ 210, 180, 140, 255 });
    else if (player.weapon == WEAP_PISTOL)
        DrawLineEx(c, (Vector2){ c.x + player.aim.x * 22.0f, c.y + player.aim.y * 22.0f },
                   4.0f, (Color){ 225, 225, 232, 255 });
    else
        DrawLineEx(c, (Vector2){ c.x + player.aim.x * 18.0f, c.y + player.aim.y * 18.0f },
                   3.0f, (Color){ 200, 225, 235, 255 });

    if (player.attackTimer > 0.0f)
        DrawRectangleRec(MeleeHitbox(),
                         (Color){ 255, 255, 255,
                                  (unsigned char)(120.0f * (player.attackTimer / 0.18f)) });
}

void DrawPickups(Pickup *p, int count, float dt)
{
    int i;
    for (i = 0; i < count; i++) {
        Vector2 c;

        if (p[i].taken) continue;
        p[i].bob += dt * 3.0f;
        c = RectCenter(p[i].box);
        c.y += sinf(p[i].bob) * 3.0f;

        DrawEllipse((int)c.x, (int)(p[i].box.y + p[i].box.height), 9.0f, 3.5f,
                    (Color){ 0, 0, 0, 80 });

        switch (p[i].type) {
            case PICK_COIN:
                DrawCircleV(c, 9.0f, (Color){ 240, 196, 62, 255 });
                DrawCircleV(c, 5.0f, (Color){ 255, 230, 132, 255 });
                break;
            case PICK_AMMO:
                DrawRectangle((int)c.x - 10, (int)c.y - 7, 20, 14,
                              (Color){ 198, 168, 108, 255 });
                DrawText("AMMO", (int)c.x - 18, (int)c.y - 26, 12,
                         (Color){ 210, 190, 140, 220 });
                break;
            case PICK_INJECTOR:
                DrawRectangle((int)c.x - 4, (int)c.y - 12, 8, 24,
                              (Color){ 130, 235, 200, 255 });
                DrawRectangle((int)c.x - 8, (int)c.y - 6, 16, 6,
                              (Color){ 90, 190, 165, 255 });
                break;
            case PICK_KEYCARD:
                DrawRectangle((int)c.x - 11, (int)c.y - 7, 22, 14,
                              (Color){ 235, 210, 110, 255 });
                DrawText("KEY", (int)c.x - 12, (int)c.y - 26, 13,
                         (Color){ 240, 220, 150, 230 });
                break;
            case PICK_SHOTGUN:
                DrawRectangle((int)c.x - 16, (int)c.y - 4, 32, 8,
                              (Color){ 214, 178, 132, 255 });
                DrawText("SHOTGUN", (int)c.x - 28, (int)c.y - 28, 14,
                         (Color){ 240, 210, 170, 240 });
                break;
            case PICK_MAP:
                DrawRectangle((int)c.x - 13, (int)c.y - 10, 26, 20,
                              (Color){ 226, 220, 196, 255 });
                DrawRectangle((int)c.x - 13, (int)c.y - 10, 26, 4,
                              (Color){ 180, 60, 60, 255 });
                DrawText("MAP", (int)c.x - 15, (int)c.y - 30, 16,
                         (Color){ 245, 240, 220, 250 });
                break;
            default: break;
        }
    }
}

/* Fake dynamic lighting without shaders: 3 soft gradient circles
   (clear centre, dark rim) + solid black outside a square that fits
   inside the circle. The overlap hides the seam. */
void DrawDarkness(Camera2D cam, Vector2 worldLight, float radius, unsigned char alpha)
{
    Vector2 s = GetWorldToScreen2D(worldLight, cam);
    Color clear = { 0, 0, 0, 0 };
    Color dark  = { 0, 0, 0, alpha };
    Color solid = { 0, 0, 0, 250 };
    int L, R, T, B, i;
    float half;

    for (i = 0; i < 3; i++)
        DrawCircleGradient((int)s.x, (int)s.y, radius, clear, dark);

    half = radius * 0.70f;
    L = (int)(s.x - half); R = (int)(s.x + half);
    T = (int)(s.y - half); B = (int)(s.y + half);
    if (L < 0) L = 0;
    if (T < 0) T = 0;
    if (R > SCREEN_W) R = SCREEN_W;
    if (B > SCREEN_H) B = SCREEN_H;

    DrawRectangle(0, 0, SCREEN_W, T, solid);
    DrawRectangle(0, B, SCREEN_W, SCREEN_H - B, solid);
    DrawRectangle(0, T, L, B - T, solid);
    DrawRectangle(R, T, SCREEN_W - R, B - T, solid);
}

/* Colour drains out of the world as you get sicker */
void DrawInfectionOverlay(void)
{
    float inf = player.infection;
    float t;

    if (inf <= INFECT_VISUAL_START) return;

    t = (inf - INFECT_VISUAL_START) / 45.0f;
    if (t > 1.0f) t = 1.0f;

    DrawRectangle(0, 0, SCREEN_W, SCREEN_H,
                  (Color){ 122, 128, 118, (unsigned char)(165.0f * t) });

    if (inf > INFECT_HEAVY_START) {
        float p = (inf - INFECT_HEAVY_START) / 40.0f;
        if (p > 1.0f) p = 1.0f;
        float beat = 0.5f + 0.5f * sinf((float)GetTime() * 6.5f);
        DrawVignette((unsigned char)(120.0f * p + 60.0f * p * beat));
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H,
                      (Color){ 110, 20, 20, (unsigned char)(30.0f * p * beat) });
    }
}

void DrawHUD(const char *objective)
{
    float hpPct = (float)player.health / (float)player.maxHealth;
    float infPct = player.infection / 100.0f;
    Color bar = (Color){ 205, 60, 60, 255 };
    Color inf = (Color){ 120, 200, 120, 255 };

    if (hpPct > 0.6f)      bar = (Color){ 96, 190, 104, 255 };
    else if (hpPct > 0.3f) bar = (Color){ 222, 182, 72, 255 };

    if (player.infection > INFECT_HEAVY_START)  inf = (Color){ 225, 70, 70, 255 };
    else if (player.infection > INFECT_VISUAL_START) inf = (Color){ 220, 175, 70, 255 };

    /* health */
    DrawRectangle(20, 20, 250, 24, (Color){ 18, 18, 22, 220 });
    DrawRectangle(20, 20, (int)(250 * hpPct), 24, bar);
    DrawRectangleLines(20, 20, 250, 24, (Color){ 200, 200, 205, 255 });
    DrawText(TextFormat("HP %d", player.health), 27, 23, 18, RAYWHITE);

    /* infection */
    DrawRectangle(20, 50, 250, 20, (Color){ 18, 18, 22, 220 });
    DrawRectangle(20, 50, (int)(250 * infPct), 20, inf);
    DrawRectangleLines(20, 50, 250, 20, (Color){ 200, 200, 205, 255 });
    DrawText(TextFormat("INFECTION %d%%", (int)player.infection), 27, 52, 16,
             player.infection > INFECT_VISUAL_START ? BLACK : RAYWHITE);

    DrawText(TextFormat("INJECTORS  %d   [hold E]", player.injectors), 20, 76, 18,
             player.injectors > 0 ? (Color){ 130, 235, 200, 255 }
                                  : (Color){ 110, 110, 118, 255 });

    /* weapon */
    DrawText(TextFormat("%s", WeaponName(player.weapon)), 20, 102, 22, RAYWHITE);
    if (player.weapon != WEAP_KNIFE)
        DrawText(TextFormat("x%d", player.ammo[player.weapon]),
                 130, 104, 20, (Color){ 220, 200, 150, 255 });
    DrawText("[1] knife  [2] pistol  [3] shotgun", 20, 128, 15,
             (Color){ 120, 120, 130, 255 });

    DrawText(TextFormat("COINS %d", game.coins), 20, 150, 20,
             (Color){ 240, 196, 62, 255 });
    DrawText(TextFormat("%.1fs", game.levelTime), 20, 174, 17,
             (Color){ 140, 140, 150, 255 });

    if (objective != NULL) {
        int w = MeasureText(objective, 20);
        DrawRectangle(SCREEN_W / 2 - w / 2 - 14, 18, w + 28, 30,
                      (Color){ 0, 0, 0, 150 });
        DrawText(objective, SCREEN_W / 2 - w / 2, 23, 20,
                 (Color){ 224, 224, 234, 255 });
    }
}

void ShowMessage(const char *text)
{
    strncpy(game.msg, text, sizeof(game.msg) - 1);
    game.msg[sizeof(game.msg) - 1] = '\0';
    game.msgTimer = 2.8f;
}

void DrawMessage(void)
{
    int w;
    float a;
    if (game.msgTimer <= 0.0f) return;
    a = game.msgTimer > 1.0f ? 1.0f : game.msgTimer;
    w = MeasureText(game.msg, 24);
    DrawRectangle(SCREEN_W / 2 - w / 2 - 18, 62, w + 36, 38,
                  (Color){ 0, 0, 0, (unsigned char)(190 * a) });
    DrawText(game.msg, SCREEN_W / 2 - w / 2, 70, 24,
             (Color){ 250, 232, 168, (unsigned char)(255 * a) });
}

void DrawPrompt(const char *text)
{
    int w = MeasureText(text, 22);
    DrawRectangle(SCREEN_W / 2 - w / 2 - 14, SCREEN_H - 150, w + 28, 34,
                  (Color){ 0, 0, 0, 180 });
    DrawText(text, SCREEN_W / 2 - w / 2, SCREEN_H - 143, 22,
             (Color){ 240, 240, 245, 255 });
}

void DrawHoldBar(float progress, const char *label)
{
    int w = 260, x = SCREEN_W / 2 - 130, y = SCREEN_H - 108;
    DrawRectangle(x - 3, y - 3, w + 6, 24, (Color){ 0, 0, 0, 200 });
    DrawRectangle(x, y, w, 18, (Color){ 40, 40, 48, 255 });
    DrawRectangle(x, y, (int)(w * progress), 18, (Color){ 120, 220, 190, 255 });
    DrawRectangleLines(x, y, w, 18, (Color){ 190, 190, 200, 255 });
    DrawText(label, x, y - 24, 20, (Color){ 235, 235, 240, 255 });
}

bool ButtonUI(Rectangle r, const char *label)
{
    Vector2 m = GetMousePosition();
    bool hover = CheckCollisionPointRec(m, r);
    int fs = 22;

    DrawRectangleRec(r, hover ? (Color){ 82, 26, 26, 255 } : (Color){ 28, 28, 34, 255 });
    DrawRectangleLinesEx(r, 2.0f,
        hover ? (Color){ 228, 72, 72, 255 } : (Color){ 92, 92, 102, 255 });
    DrawText(label,
             (int)(r.x + r.width * 0.5f - MeasureText(label, fs) * 0.5f),
             (int)(r.y + r.height * 0.5f - fs * 0.5f),
             fs, RAYWHITE);

    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}