/* 1. hitstop  - freeze the world for 60ms when you connect
2. shake    - the camera jolts
3. particles- blood, dust, sparks
4. flash    - full screen colour pop
5. vignette - dark corners, instant horror */
#include "game.h"
#include <math.h>
#include <string.h>

#define MAX_FLOATERS 24

typedef struct Floater {
    Vector2 pos;
    char    text[24];
    Color   color;
    float   life;
    bool    active;
} Floater;

static Particle particles[MAX_PARTICLES];
static Floater  floaters[MAX_FLOATERS];
static float    shakePower;
static float    hitstop;
static float    flashTime, flashMax;
static Color    flashColor;
static Vector2  shakeOffset;

void FxReset(void)
{
    int i;
    for (i = 0; i < MAX_PARTICLES; i++) particles[i].active = false;
    for (i = 0; i < MAX_FLOATERS; i++)  floaters[i].active = false;
    shakePower = 0.0f;
    hitstop = 0.0f;
    flashTime = 0.0f;
    flashMax = 1.0f;
    shakeOffset.x = 0.0f; shakeOffset.y = 0.0f;
    flashColor = (Color){ 255, 255, 255, 255 };
}

void AddShake(float power)
{
    shakePower += power;
    if (shakePower > 26.0f) shakePower = 26.0f; 
}

/* Freeze every moving thing for a few frames. A hit with hitstop reads as heavy,
   the same hit without it reads as nothing. */
void AddHitstop(float seconds)
{
    if (seconds > hitstop) hitstop = seconds;
}

void FlashScreen(Color c, float duration)
{
    flashColor = c;
    flashTime = duration;
    flashMax = duration;
}

static void UpdateParticles(float dt)
{
    int i;
    for (i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;
        particles[i].pos.x += particles[i].vel.x * dt;
        particles[i].pos.y += particles[i].vel.y * dt;
        particles[i].vel.x *= (1.0f - particles[i].drag * dt);
        particles[i].vel.y *= (1.0f - particles[i].drag * dt);
        particles[i].life -= dt;
        if (particles[i].life <= 0.0f) particles[i].active = false;
    }
}

/* Call this once per frame from main.c with the real frame time.
   It returns the delta the rest of the game should use, which is
   zero while hitstop is running. */
float FxTick(float rawDt)
{
    int i;

    shakePower -= 34.0f * rawDt;
    if (shakePower < 0.0f) shakePower = 0.0f;
    shakeOffset.x = (float)GetRandomValue(-100, 100) / 100.0f * shakePower;
    shakeOffset.y = (float)GetRandomValue(-100, 100) / 100.0f * shakePower;

    if (flashTime > 0.0f) flashTime -= rawDt;

    for (i = 0; i < MAX_FLOATERS; i++) {
        if (!floaters[i].active) continue;
        floaters[i].pos.y -= 34.0f * rawDt;
        floaters[i].life -= rawDt;
        if (floaters[i].life <= 0.0f) floaters[i].active = false;
    }

    if (hitstop > 0.0f) {
        hitstop -= rawDt;
        UpdateParticles(rawDt * 0.12f);   /* particles crawl, don't stop dead */
        return 0.0f;                      /* the world is frozen */
    }

    UpdateParticles(rawDt);
    return rawDt;
}

void ApplyShake(Camera2D *cam)
{
    cam->offset.x = SCREEN_W * 0.5f + shakeOffset.x;
    cam->offset.y = SCREEN_H * 0.5f + shakeOffset.y;
}

void SpawnParticles(Vector2 pos, int count, Color c,
                    float speed, float life, float size)
{
    int i, spawned = 0;
    for (i = 0; i < MAX_PARTICLES && spawned < count; i++) {
        if (particles[i].active) continue;
        {
            float ang = (float)GetRandomValue(0, 628) / 100.0f;
            float spd = speed * (float)GetRandomValue(40, 130) / 100.0f;
            particles[i].active = true;
            particles[i].pos = pos;
            particles[i].vel.x = cosf(ang) * spd;
            particles[i].vel.y = sinf(ang) * spd;
            particles[i].maxLife = life * (float)GetRandomValue(60, 140) / 100.0f;
            particles[i].life = particles[i].maxLife;
            particles[i].size = size;
            particles[i].drag = 3.0f;
            particles[i].color = c;
        }
        spawned++;
    }
}

void SpawnBlood(Vector2 pos, Vector2 dir, int count)
{
    int i, spawned = 0;
    for (i = 0; i < MAX_PARTICLES && spawned < count; i++) {
        if (particles[i].active) continue;
        {
            float spread = (float)GetRandomValue(-60, 60) / 100.0f;
            float ang = atan2f(dir.y, dir.x) + spread;
            float spd = (float)GetRandomValue(90, 320);
            particles[i].active = true;
            particles[i].pos = pos;
            particles[i].vel.x = cosf(ang) * spd;
            particles[i].vel.y = sinf(ang) * spd;
            particles[i].maxLife = (float)GetRandomValue(30, 75) / 100.0f;
            particles[i].life = particles[i].maxLife;
            particles[i].size = (float)GetRandomValue(2, 5);
            particles[i].drag = 4.5f;
            particles[i].color = (Color){ (unsigned char)GetRandomValue(140, 200), 20, 28, 255 };
        }
        spawned++;
    }
}

void SpawnDust(Vector2 pos)
{
    SpawnParticles(pos, 2, (Color){ 120, 115, 110, 130 }, 26.0f, 0.4f, 2.0f);
}

void DrawParticles(void)
{
    int i;
    for (i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;
        {
            float t = particles[i].life / particles[i].maxLife;
            Color c = particles[i].color;
            c.a = (unsigned char)(c.a * t);
            DrawCircleV(particles[i].pos, particles[i].size * t, c);
        }
    }
}

void PushFloatingText(Vector2 worldPos, const char *text, Color c)
{
    int i;
    for (i = 0; i < MAX_FLOATERS; i++) {
        if (floaters[i].active) continue;
        floaters[i].active = true;
        floaters[i].pos = worldPos;
        floaters[i].color = c;
        floaters[i].life = 0.9f;
        strncpy(floaters[i].text, text, sizeof(floaters[i].text) - 1);
        floaters[i].text[sizeof(floaters[i].text) - 1] = '\0';
        return;
    }
}

void DrawFloatingText(Camera2D cam)
{
    int i;
    for (i = 0; i < MAX_FLOATERS; i++) {
        if (!floaters[i].active) continue;
        {
            Vector2 s = GetWorldToScreen2D(floaters[i].pos, cam);
            Color c = floaters[i].color;
            c.a = (unsigned char)(255.0f * (floaters[i].life / 0.9f));
            DrawText(floaters[i].text,
                     (int)s.x - MeasureText(floaters[i].text, 20) / 2,
                     (int)s.y, 20, c);
        }
    }
}

void DrawVignette(unsigned char strength)
{
    Color edge = { 0, 0, 0, strength };
    Color clear = { 0, 0, 0, 0 };
    DrawCircleGradient(SCREEN_W / 2, SCREEN_H / 2, 900.0f, clear, edge);
    DrawCircleGradient(SCREEN_W / 2, SCREEN_H / 2, 900.0f, clear, edge);
}

void DrawFlash(void)
{
    if (flashTime <= 0.0f) return;
    {
        Color c = flashColor;
        c.a = (unsigned char)(c.a * (flashTime / flashMax));
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H, c);
    }
}