/*
 Movement and wall collision, the infection meter, the camera,
   and the loader that turns a level's ASCII map into rectangles.
*/


#include "game.h"
#include <math.h>
#include <string.h>


Vector2 RectCenter(Rectangle r)
{
    Vector2 v = { r.x + r.width * 0.5f, r.y + r.height * 0.5f };
    return v;
}

float Dist(Vector2 a, Vector2 b)
{
    float dx = a.x - b.x, dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}

Vector2 Norm(Vector2 v)
{
    float len = sqrtf(v.x * v.x + v.y * v.y);
    if (len < 0.0001f) { Vector2 zero = { 0.0f, 0.0f }; return zero; }
    v.x /= len; v.y /= len;
    return v;
}

float Approach(float value, float target, float rate)
{
    if (value < target) { value += rate; if (value > target) value = target; }
    else                { value -= rate; if (value < target) value = target; }
    return value;
}

/* Walk each row and merge runs of '#' into one wide rectangle.
   A 30x16 map becomes ~50 rectangles instead of ~300. */


int BuildWallsFromMap(const char **rows, int rowCount,Rectangle *walls, int maxWalls)
{
    int r, c, count = 0;

    for (r = 0; r < rowCount; r++) 
    {
        int len = (int)strlen(rows[r]);
        c = 0;
        while (c < len)
         {
            if (rows[r][c] == '#') 
            {
                int start = c;
                while (c < len && rows[r][c] == '#') c++;
                if (count < maxWalls) {
                    walls[count].x = start * TILE;
                    walls[count].y = r * TILE;
                    walls[count].width = (c - start) * TILE;
                    walls[count].height = TILE;
                    count++;
                }
            } else c++;
        }
    }
    return count;
}

//player//



void ResetPlayer(float x, float y)
{
    int i;

    player.box.x = x; 
    player.box.y = y;
    player.box.width = PLAYER_SIZE;
    player.box.height = PLAYER_SIZE;
    player.speed = PLAYER_SPEED;
    player.maxHealth = PLAYER_MAX_HEALTH;
    player.health = PLAYER_MAX_HEALTH;

    player.infection = 0.0f;
    player.injectors = 0;
    player.injectTimer = 0.0f;

    for (i = 0; i < WEAP_COUNT; i++) 
    { 
        player.hasWeapon[i] = false; player.ammo[i] = 0; 
    }
    player.hasWeapon[WEAP_KNIFE] = true;
    player.hasWeapon[WEAP_PISTOL] = true;
    player.ammo[WEAP_PISTOL] = PISTOL_START_AMMO;
    player.weapon = WEAP_PISTOL;

    player.fireCooldown = 0.0f;
    player.attackTimer = 0.0f;
    player.hurtTimer = 0.0f;
    player.sprinting = false;
    player.hasKeycard = false;
    player.hasMap = false;
    player.knockback.x = 0.0f; player.knockback.y = 0.0f;
    player.aim.x = 1.0f; player.aim.y = 0.0f;
}

/* Move without touching health, ammo or infection. Levels 2 and 3 use
   this so you arrive carrying whatever you walked out with. */


void PlacePlayer(float x, float y)
{
    player.box.x = x; player.box.y = y;
    player.knockback.x = 0.0f; player.knockback.y = 0.0f;
    player.attackTimer = 0.0f;
    player.fireCooldown = 0.0f;
    player.hurtTimer = 0.0f;
    player.injectTimer = 0.0f;
}

void SaveLoadout(void)
{
    game.hasCarry       = true;
    game.carryHealth    = player.health;
    game.carryInfection = player.infection;
    game.carryInjectors = player.injectors;
    game.carryPistol    = player.ammo[WEAP_PISTOL];
    game.carryShells    = player.ammo[WEAP_SHOTGUN];
    game.carryShotgun   = player.hasWeapon[WEAP_SHOTGUN];
}

/* Floors on health and ammo so a retry is never unwinnable. */


void RestoreLoadout(void)
{
    if (!game.hasCarry) return;

    player.health    = game.carryHealth < RETRY_MIN_HEALTH? RETRY_MIN_HEALTH : game.carryHealth;
    player.infection = game.carryInfection;
    player.injectors = game.carryInjectors;

    player.ammo[WEAP_PISTOL] = game.carryPistol < RETRY_MIN_PISTOL_AMMO ? RETRY_MIN_PISTOL_AMMO : game.carryPistol;
    player.hasWeapon[WEAP_SHOTGUN] = game.carryShotgun;
    if (game.carryShotgun)
     {
        player.ammo[WEAP_SHOTGUN] = game.carryShells < RETRY_MIN_SHELLS ? RETRY_MIN_SHELLS : game.carryShells;
        player.weapon = WEAP_SHOTGUN;
    }
}

/* Move one axis at a time and push back out of walls.
   X first then Y is what lets you slide along a wall instead of
   sticking to it. Most reused function in the game. */


void MoveBox(Rectangle *box, float dx, float dy, Rectangle *walls, int wallCount)
{
    int i;

    box->x += dx;
    for (i = 0; i < wallCount; i++) 
    {
        if (CheckCollisionRecs(*box, walls[i]))
         {
            if (dx > 0.0f)      box->x = walls[i].x - box->width;
            else if (dx < 0.0f) box->x = walls[i].x + walls[i].width;
        }
    }

    box->y += dy;
    for (i = 0; i < wallCount; i++) 
    {
        if (CheckCollisionRecs(*box, walls[i])) 
        {
            if (dy > 0.0f)      box->y = walls[i].y - box->height;
            else if (dy < 0.0f) box->y = walls[i].y + walls[i].height;
        }
    }
}

void UpdatePlayerMovement(Rectangle *walls, int wallCount, float dt)
{
    Vector2 dir = { 0.0f, 0.0f };
    float sp;
    static float stepTimer = 0.0f;

    if (player.attackTimer  > 0.0f) player.attackTimer  -= dt;
    if (player.fireCooldown > 0.0f) player.fireCooldown -= dt;
    if (player.hurtTimer    > 0.0f) player.hurtTimer    -= dt;

    player.knockback.x = Approach(player.knockback.x, 0.0f, 900.0f * dt);
    player.knockback.y = Approach(player.knockback.y, 0.0f, 900.0f * dt);
    if (player.knockback.x != 0.0f || player.knockback.y != 0.0f)
        MoveBox(&player.box, player.knockback.x * dt, player.knockback.y * dt,walls, wallCount);

    /* holding E to inject locks you in place - that IS the cost */

    if (player.injectTimer > 0.0f) return;

    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    dir.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  dir.y += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  dir.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) dir.x += 1.0f;
    dir = Norm(dir);

    player.sprinting = IsKeyDown(KEY_LEFT_SHIFT);
    sp = player.sprinting ? player.speed * PLAYER_SPRINT_MULT : player.speed;

    /* high infection makes you slow. the punishment must be felt.
       Thresholds are low (15/35) because infection only comes from hits,
       so a whole level lands somewhere around 20-40%, not 80%. */


    if (player.infection > INFECT_HEAVY_START) 
    {
        float t = (player.infection - INFECT_HEAVY_START) / 40.0f;
        if (t > 1.0f) t = 1.0f;
        sp *= (1.0f - INFECT_SLOW_AMOUNT * t);
    }

    if (dir.x != 0.0f || dir.y != 0.0f) 
    {
        MoveBox(&player.box, dir.x * sp * dt, dir.y * sp * dt, walls, wallCount);
        stepTimer -= dt * (sp / 200.0f);
        if (stepTimer <= 0.0f) 
        {
            stepTimer = 0.30f;
            SpawnDust((Vector2){ player.box.x + player.box.width * 0.5f,player.box.y + player.box.height });
        }
    }
}

void AimAtMouse(Camera2D cam)
{
    Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), cam);
    Vector2 c = RectCenter(player.box);
    Vector2 d;
    float a;

    d.x = mouseWorld.x - c.x;
    d.y = mouseWorld.y - c.y;
    d = Norm(d);

    /* infection makes your hands shake */


    if (player.infection > INFECT_VISUAL_START) 
    
    {
        float t = (player.infection - INFECT_VISUAL_START) / 45.0f;
        if (t > 1.0f) t = 1.0f;
        float sway = sinf((float)GetTime() * 3.4f) * 0.16f * t+ sinf((float)GetTime() * 8.1f) * 0.05f * t;
        a = atan2f(d.y, d.x) + sway;
        d.x = cosf(a); d.y = sinf(a);
    }
    player.aim = d;
}

/* ---------------- infection ---------------- */


void AddInfection(float amount)
{
    player.infection += amount;
    if (player.infection > 100.0f) player.infection = 100.0f;
    if (player.infection < 0.0f)   player.infection = 0.0f;
    if (player.infection > game.peakInfection) game.peakInfection = player.infection;
}

/* ambientPerSecond exists so a later level could add background
   contamination. Every level currently passes 0.0f: infection comes
   only from being bitten. */


void UpdateInfection(float ambientPerSecond, float dt)
{
    if (ambientPerSecond > 0.0f) AddInfection(ambientPerSecond * dt);
    if (player.infection >= 100.0f) {
        player.health = 0;                 /* you turn */
        ShowMessage("INFECTION CRITICAL - YOU TURNED");
    }
}

/* Hold E for 1.5s, standing completely still, to burn one injector.
   Moving cancels it. The meter is never about the meter - it's about
   whether you can afford to stop. */


void UpdateInjector(float dt)
{
    if (player.injectors <= 0) { player.injectTimer = 0.0f; return; }

    if (IsKeyDown(KEY_E) && player.infection > 0.0f) {
        bool moving = IsKeyDown(KEY_W) || IsKeyDown(KEY_A) ||  IsKeyDown(KEY_S) || IsKeyDown(KEY_D) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT);
        if (moving) 
        {
            player.injectTimer = 0.0f;
            return;
        }

        player.injectTimer += dt;
        if (player.injectTimer >= INJECT_HOLD_TIME)
         {
            player.injectTimer = 0.0f;
            player.injectors--;
            AddInfection(-INJECT_CLEARS);
            FlashScreen((Color){ 120, 220, 190, 90 }, 0.3f);
            SpawnParticles(RectCenter(player.box), 14, (Color){ 130, 235, 200, 255 }, 120.0f, 0.5f, 3.0f);
            ShowMessage("injector used  -25% infection");
        }
    } 
    else 
    {
        player.injectTimer = 0.0f;
    }
}

void DamagePlayer(int amount, int infectAmount, Vector2 fromPos)
{
    Vector2 c, away;

    if (player.hurtTimer > 0.0f) return;      /* invincibility frames */

    player.health -= amount;
    AddInfection((float)infectAmount);
    game.hits++;                              /* for the results screen */
    player.hurtTimer = PLAYER_IFRAMES;
    player.injectTimer = 0.0f;                /* getting hit ruins the injection */
    if (player.health < 0) player.health = 0;

    c = RectCenter(player.box);
    away = Norm((Vector2){ c.x - fromPos.x, c.y - fromPos.y });
    player.knockback.x = away.x * PLAYER_KNOCKBACK;
    player.knockback.y = away.y * PLAYER_KNOCKBACK;

    AddShake(SHAKE_ON_HURT);
    AddHitstop(0.06f);
    FlashScreen((Color){ 180, 20, 20, 110 }, 0.25f);
    SpawnBlood(c, away, 10);
}

void AddCoins(int amount)
{
    game.coins += amount;
    if (game.coins < 0) game.coins = 0;
    PushFloatingText(RectCenter(player.box),TextFormat("%s%d", amount >= 0 ? "+" : "", amount),amount >= 0 ? (Color){ 245, 200, 70, 255 }: (Color){ 235, 90, 90, 255 });
}

void PlayerDied(Scene retryLevel)
{
    game.deaths++;
    game.coins -= COIN_DEATH_PENALTY;
    if (game.coins < 0) game.coins = 0;
    game.retryScene = retryLevel;
    FlashScreen((Color){ 140, 0, 0, 200 }, 0.5f);
    AddShake(16.0f);
    game.scene = SCENE_GAMEOVER;
}

/* ---------------- camera ---------------- */

Camera2D MakeCamera(float zoom)
{
    Camera2D cam;
    cam.offset.x = SCREEN_W * 0.5f;
    cam.offset.y = SCREEN_H * 0.5f;
    cam.target   = RectCenter(player.box);
    cam.rotation = 0.0f;
    cam.zoom     = zoom;
    return cam;
}

/* Smooth follow plus a lean toward the mouse, so you see further in
   the direction you're looking. Makes aiming in the dark deliberate. */

   
void UpdateCameraFollow(Camera2D *cam, float dt, float leanAmount)
{
    Vector2 target = RectCenter(player.box);
    float t = CAMERA_FOLLOW_SPEED * dt;
    if (t > 1.0f) t = 1.0f;

    target.x += player.aim.x * leanAmount;
    target.y += player.aim.y * leanAmount;

    cam->target.x += (target.x - cam->target.x) * t;
    cam->target.y += (target.y - cam->target.y) * t;
    ApplyShake(cam);
}