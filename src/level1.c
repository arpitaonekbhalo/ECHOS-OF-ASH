/*  #  wall              .  floor
    P  player start      Z  shambler (dies to the pistol)
    D  armoury door      G  soldier  (wants the shotgun)
    T  the gate          K  keycard
    S  shotgun           A  ammo
    I  injector          C  coin
    M  the map           E  exit
    Every row must be exactly 30 characters and there must be 16. */
#include "game.h"
#include <math.h>
#include <string.h>

#define MAP_COLS 30
#define MAP_ROWS 16

static const char *MAP[MAP_ROWS] = {
"##############################",
"#.......#########............#",
"#.P..I..#########.....C....E.#",
"#.......#########..G.........#",
"#.....C.#########............#",
"#..C....#########........G...#",
"#.......#########....A.......#",
"#..............##.........C..#",
"#......Z..C...K.T...C........#",
"#.........Z..C.##.......C....#",
"###########DD####.........G..#",
"#########......##...G........#",
"#########.A..I.##............#",
"#########..S.C.##.I....M.....#",
"#########......##............#",
"##############################",
};


static Rectangle walls[MAX_WALLS];      static int wallCount;
static Zombie    zombies[MAX_ZOMBIES];  static int zombieCount;
static Pickup    pickups[MAX_PICKUPS];  static int pickupCount;
static Bullet    bullets[MAX_BULLETS];
static Camera2D  cam;

static Rectangle armouryDoor;
static int       armouryDoorWall;
static bool      armouryOpen;
static bool      armouryFound;

static Rectangle messGate;              /* the one tile into the mess hall */
static bool      gateCrossed;

static Rectangle exitDoor;
static int       mapPickupIndex;
static bool      roomCleared;
static float     lastDt;

static void AddPickup(float x, float y, int type, int amount)
{
    if (pickupCount >= MAX_PICKUPS) return;
    pickups[pickupCount].box.x = x + 16.0f;
    pickups[pickupCount].box.y = y + 16.0f;
    pickups[pickupCount].box.width  = 32.0f;
    pickups[pickupCount].box.height = 32.0f;
    pickups[pickupCount].type   = type;
    pickups[pickupCount].amount = amount;
    pickups[pickupCount].taken  = false;
    pickups[pickupCount].bob    = (float)GetRandomValue(0, 300) / 100.0f;
    pickupCount++;
}

static int AddDynamicWall(Rectangle r)
{
    if (wallCount >= MAX_WALLS) return -1;
    walls[wallCount] = r;
    wallCount++;
    return wallCount - 1;
}

static void DisableWall(int index)
{
    if (index < 0) return;
    walls[index].x = -99999.0f;
    walls[index].y = -99999.0f;
}

static int SoldiersLeft(void)
{
    int i, n = 0;
    for (i = 0; i < zombieCount; i++)
        if (zombies[i].alive && zombies[i].type == ZT_SOLDIER) n++;
    return n;
}

/* setup*/
void Level1_Init(void)
{
    int r, c;
    float px = 150.0f, py = 150.0f;
    int i;

    wallCount = 0; zombieCount = 0; pickupCount = 0;
    armouryOpen = false; armouryFound = false;
    gateCrossed = false; roomCleared = false;
    mapPickupIndex = -1; lastDt = 0.0f;

    game.level = 1;
    game.kills = 0; game.hits = 0; game.peakInfection = 0.0f;

    for (i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
    FxReset();

    wallCount = BuildWallsFromMap(MAP, MAP_ROWS, walls, MAX_WALLS);

    for (r = 0; r < MAP_ROWS; r++) {
        for (c = 0; c < MAP_COLS; c++) {
            float x = c * TILE, y = r * TILE;
            switch (MAP[r][c]) {
                case 'P': px = x + 19.0f; py = y + 19.0f; break;

                case 'Z':   /* awake these two guard the armoury door */
                    SpawnZombie(zombies, &zombieCount, x + 18.0f, y + 18.0f, ZT_SHAMBLER);
                    break;

                case 'G':   /* asleep until you step through the gate */
                    if (SpawnZombie(zombies, &zombieCount,
                                    x + 16.0f, y + 16.0f, ZT_SOLDIER) >= 0)
                        zombies[zombieCount - 1].dormant = true;
                    break;

                case 'T':
                    messGate.x = x; messGate.y = y;
                    messGate.width = TILE; messGate.height = TILE;
                    break;

                case 'C': AddPickup(x, y, PICK_COIN, COIN_PICKUP_VALUE);    break;
                case 'A': AddPickup(x, y, PICK_AMMO, 10);                   break;
                case 'I': AddPickup(x, y, PICK_INJECTOR, 1);                break;
                case 'K': AddPickup(x, y, PICK_KEYCARD, 1);                 break;
                case 'S': AddPickup(x, y, PICK_SHOTGUN, SHOTGUN_START_AMMO);break;
                case 'M': AddPickup(x, y, PICK_MAP, 1);
                          mapPickupIndex = pickupCount - 1;                 break;
                case 'E':
                    exitDoor.x = x; exitDoor.y = y;
                    exitDoor.width = TILE; exitDoor.height = TILE;
                    break;
                default: break;
            }
        }
    }

    /* the armoury door: the two 'D' tiles at row 10, columns 11 and 12.
       If you move them on the map, change these two numbers to match. */
    armouryDoor.x = 11.0f * TILE; armouryDoor.y = 10.0f * TILE;
    armouryDoor.width = 2.0f * TILE; armouryDoor.height = TILE;
    armouryDoorWall = AddDynamicWall(armouryDoor);

    ResetPlayer(px, py);
    cam = MakeCamera(1.0f);
    ShowMessage("WASD move   MOUSE aim   CLICK shoot   HOLD E to inject");
}

/*  update */
void Level1_Update(float dt)
{
    int i;

    lastDt = dt;
    game.levelTime += dt;
    game.retryScene = SCENE_LEVEL1;

    AimAtMouse(cam);
    UpdatePlayerMovement(walls, wallCount, dt);
    UpdateCameraFollow(&cam, dt, CAMERA_LEAN);

    UpdateInfection(0.0f, dt);   
    UpdateInjector(dt);

    if (IsKeyPressed(KEY_ONE))   SwitchWeapon(WEAP_KNIFE);
    if (IsKeyPressed(KEY_TWO))   SwitchWeapon(WEAP_PISTOL);
    if (IsKeyPressed(KEY_THREE)) SwitchWeapon(WEAP_SHOTGUN);
    if (IsKeyPressed(KEY_Q))     CycleWeapon();

    /* Hold to keep firing - the weapon cooldown paces it.Blocked while injecting */
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && player.injectTimer <= 0.0f)
        FireWeapon(bullets, zombies, zombieCount);

    UpdateBullets(bullets, walls, wallCount, dt);
    {
        int kills = BulletsHitZombies(bullets, zombies, zombieCount);
        if (kills > 0) { AddCoins(COIN_PER_KILL * kills); game.kills += kills; }
    }

    UpdateZombies(zombies, zombieCount, walls, wallCount, dt, ZOMBIE_AGGRO_RANGE);

    /* four zombies waking after this */
    if (!gateCrossed && CheckCollisionRecs(player.box, messGate)) {
        int woke = 0;
        gateCrossed = true;
        for (i = 0; i < zombieCount; i++) {
            if (!zombies[i].dormant) continue;
            zombies[i].dormant = false;
            zombies[i].state = Z_CHASE;
            woke++;
        }
        if (woke > 0) {
            AddShake(13.0f);
            FlashScreen((Color){ 190, 60, 60, 100 }, 0.45f);
            ShowMessage("they were waiting for you");
        }
    }

    /* last soldier down, the map is yours */
    if (gateCrossed && !roomCleared && SoldiersLeft() == 0) {
        roomCleared = true;
        AddCoins(COIN_ROOM_CLEARED);
        AddShake(14.0f);
        AddHitstop(0.18f);
        FlashScreen((Color){ 255, 255, 255, 90 }, 0.35f);
        ShowMessage("ROOM CLEAR  +40 coins  -  take the map");
    }

    for (i = 0; i < pickupCount; i++) {
        if (pickups[i].taken) continue;
        if (!CheckCollisionRecs(player.box, pickups[i].box)) continue;

        /* the map is guarded, cant have it w/o killing the zombies */
        if (pickups[i].type == PICK_MAP && !roomCleared) {
            if (game.msgTimer <= 0.0f)
                ShowMessage("still moving in here  -  clear the room first");
            continue;
        }

        pickups[i].taken = true;
        switch (pickups[i].type) {
            case PICK_COIN:
                AddCoins(pickups[i].amount);
                break;
            case PICK_AMMO:
                player.ammo[WEAP_PISTOL] += pickups[i].amount;
                if (player.hasWeapon[WEAP_SHOTGUN]) {
                    player.ammo[WEAP_SHOTGUN] += 3;
                    ShowMessage(TextFormat("+%d rounds, +3 shells", pickups[i].amount));
                } else {
                    ShowMessage(TextFormat("+%d rounds", pickups[i].amount));
                }
                break;
            case PICK_INJECTOR:
                player.injectors++;
                ShowMessage("injector picked up  -  HOLD E to use one");
                break;
            case PICK_KEYCARD:
                player.hasKeycard = true;
                ShowMessage("armoury keycard  -  the door is south");
                break;
            case PICK_SHOTGUN:
                player.hasWeapon[WEAP_SHOTGUN] = true;
                player.ammo[WEAP_SHOTGUN] += pickups[i].amount;
                SwitchWeapon(WEAP_SHOTGUN);
                armouryFound = true;
                AddCoins(COIN_SHOTGUN_FOUND);
                FlashScreen((Color){ 255, 230, 170, 110 }, 0.4f);
                ShowMessage("SHOTGUN  +30 coins  -  press 3 to equip");
                break;
            case PICK_MAP:
                player.hasMap = true;
                AddCoins(COIN_MAP_TAKEN);
                FlashScreen((Color){ 255, 255, 255, 120 }, 0.4f);
                ShowMessage("MAP ACQUIRED  -  get to the exit");
                break;
            default: break;
        }
    }

    /* the armoury door */
    if (!armouryOpen) {
        Rectangle zone = { armouryDoor.x - 24.0f, armouryDoor.y - 24.0f,
                           armouryDoor.width + 48.0f, armouryDoor.height + 48.0f };
        if (CheckCollisionRecs(player.box, zone) &&
            player.hasKeycard && IsKeyPressed(KEY_F)) {
            armouryOpen = true;
            DisableWall(armouryDoorWall);
            SpawnParticles(RectCenter(armouryDoor), 18,
                           (Color){ 230, 210, 140, 255 }, 150.0f, 0.5f, 3.0f);
            AddShake(6.0f);
            ShowMessage("armoury unlocked");
        }
    }

    /* the exit */
    if (CheckCollisionRecs(player.box, exitDoor)) {
        if (player.hasMap) {
            int bonus = 0;
            if (game.levelTime < BONUS_FAST_TIME)  bonus += BONUS_FAST_COINS;
            if (player.infection < BONUS_CLEAN_INFECTION) bonus += BONUS_CLEAN_COINS;
            AddCoins(bonus);
            SaveLoadout();
            game.scene = SCENE_RESULTS;
            return;
        } else if (game.msgTimer <= 0.0f) {
            ShowMessage("not without the map");
        }
    }

    if (player.health <= 0) PlayerDied(SCENE_LEVEL1);
}

/* draw */
static Rectangle ObjectiveTarget(void)
{
    int i;
    if (!player.hasKeycard) {
        for (i = 0; i < pickupCount; i++)
            if (pickups[i].type == PICK_KEYCARD && !pickups[i].taken) return pickups[i].box;
    }
    if (!armouryOpen) return armouryDoor;
    if (!player.hasWeapon[WEAP_SHOTGUN]) {
        for (i = 0; i < pickupCount; i++)
            if (pickups[i].type == PICK_SHOTGUN && !pickups[i].taken) return pickups[i].box;
    }
    if (!player.hasMap && mapPickupIndex >= 0) return pickups[mapPickupIndex].box;
    return exitDoor;
}

/* The light only reaches about 9 tiles. Without this you spend the level
   walking into dark corners hunting for the next thing. */
static void DrawObjectivePointer(void)
{
    Rectangle t = ObjectiveTarget();
    Vector2 tc = RectCenter(t);
    Vector2 pc = RectCenter(player.box);
    float d = Dist(pc, tc);
    float a, sx, sy, fade;
    Vector2 v1, v2, v3;
    const char *lbl;

    if (d < 140.0f) return;

    a  = atan2f(tc.y - pc.y, tc.x - pc.x);
    sx = SCREEN_W * 0.5f + cosf(a) * 210.0f;
    sy = SCREEN_H * 0.5f + sinf(a) * 210.0f;
    fade = (d - 140.0f) / 300.0f;
    if (fade > 1.0f) fade = 1.0f;
    fade *= 0.75f;

    v1.x = sx + cosf(a) * 14.0f;        v1.y = sy + sinf(a) * 14.0f;
    v2.x = sx + cosf(a + 2.5f) * 11.0f; v2.y = sy + sinf(a + 2.5f) * 11.0f;
    v3.x = sx + cosf(a - 2.5f) * 11.0f; v3.y = sy + sinf(a - 2.5f) * 11.0f;
    DrawTriangle(v1, v2, v3, (Color){ 240, 196, 62, (unsigned char)(255.0f * fade) });

    lbl = TextFormat("%dm", (int)(d / 64.0f));
    DrawText(lbl, (int)(sx - MeasureText(lbl, 12) * 0.5f), (int)(sy + 20), 12,
             (Color){ 240, 196, 62, (unsigned char)(200.0f * fade) });
}

static const char *CurrentObjective(void)
{
    if (player.hasMap)                    return "OBJECTIVE: reach the exit";
    if (roomCleared)                      return "OBJECTIVE: take the map";
    if (!player.hasKeycard)               return "OBJECTIVE: find a way through the base";
    if (!armouryOpen)                     return "OBJECTIVE: open the armoury  [F]";
    if (!player.hasWeapon[WEAP_SHOTGUN])  return "OBJECTIVE: take the shotgun";
    return "OBJECTIVE: the map is east  -  it is guarded";
}

void Level1_Draw(void)
{
    BeginMode2D(cam);
        DrawFloor(MAP_COLS, MAP_ROWS);
        DrawWalls(walls, wallCount);

        if (!armouryOpen) {
            DrawRectangleRec(armouryDoor,
                player.hasKeycard ? (Color){ 90, 150, 100, 255 }
                                  : (Color){ 130, 60, 60, 255 });
            DrawRectangleLinesEx(armouryDoor, 3.0f, (Color){ 200, 190, 160, 255 });
            DrawText(player.hasKeycard ? "ARMOURY  [F]" : "ARMOURY - LOCKED",
                     (int)armouryDoor.x - 10, (int)armouryDoor.y - 24, 18,
                     (Color){ 225, 210, 180, 255 });
        }

        /* the gate into the mess hall */
        if (!gateCrossed) {
            DrawRectangleLinesEx(messGate, 2.0f, (Color){ 150, 120, 90, 160 });
            DrawText("GATE", (int)messGate.x - 4, (int)messGate.y - 22, 14,
                     (Color){ 180, 150, 110, 200 });
        }

        DrawRectangleRec(exitDoor,
            player.hasMap ? (Color){ 90, 200, 120, 255 } : (Color){ 120, 60, 60, 255 });
        DrawRectangleLinesEx(exitDoor, 3.0f, (Color){ 220, 220, 220, 255 });
        DrawText("EXIT", (int)exitDoor.x - 4, (int)exitDoor.y - 24, 18,
                 (Color){ 210, 220, 215, 255 });

        /* the map, ringed so you can see it from across the room */
        if (mapPickupIndex >= 0 && !pickups[mapPickupIndex].taken) {
            Vector2 mc = RectCenter(pickups[mapPickupIndex].box);
            float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 2.2f);
            Color ring = roomCleared
                ? (Color){ 120, 220, 150, (unsigned char)(90 + 110 * pulse) }
                : (Color){ 200, 80, 80,  (unsigned char)(60 + 80 * pulse) };
            DrawCircleLines((int)mc.x, (int)mc.y, 22.0f + pulse * 6.0f, ring);
            DrawText(roomCleared ? "MAP" : "GUARDED",
                     (int)mc.x - (roomCleared ? 14 : 30), (int)mc.y - 40, 15,
                     roomCleared ? (Color){ 140, 230, 165, 255 }
                                 : (Color){ 200, 138, 138, 255 });
        }

        DrawPickups(pickups, pickupCount, lastDt);
        DrawZombies(zombies, zombieCount);
        DrawBullets(bullets, (Color){ 255, 224, 140, 255 });
        DrawParticles();
        DrawPlayer();
    EndMode2D();

    DrawDarkness(cam, RectCenter(player.box), LIGHT_RADIUS, DARKNESS_ALPHA);
    DrawInfectionOverlay();
    DrawVignette(70);
    DrawFloatingText(cam);
    DrawFlash();

    DrawObjectivePointer();
    DrawHUD(CurrentObjective());

    /* how many soldiers are still between you and the map */
    if (gateCrossed && !roomCleared && SoldiersLeft() > 0) {
        const char *t = TextFormat("SOLDIERS  %d / 4", SoldiersLeft());
        int w = MeasureText(t, 24);
        DrawRectangle(SCREEN_W / 2 - w / 2 - 16, SCREEN_H - 62, w + 32, 36,
                      (Color){ 0, 0, 0, 170 });
        DrawText(t, SCREEN_W / 2 - w / 2, SCREEN_H - 54, 24,
                 (Color){ 240, 190, 120, 255 });
    }

    if (player.injectTimer > 0.0f)
        DrawHoldBar(player.injectTimer / INJECT_HOLD_TIME, "INJECTING - DON'T MOVE");
    else if (player.injectors > 0 && player.infection > 12.0f && game.msgTimer <= 0.0f)
        DrawPrompt("HOLD [E] TO INJECT");

    (void)armouryFound;
}