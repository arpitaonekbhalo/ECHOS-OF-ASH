/* level3.c  

   The shape of the level:
     1. Arrive. A SUPPLY CACHE sits right there - spend the coins
        you collected in levels 1 and 2. This is what they were for.
     2. Cross the SECURITY GRID (see beams.c). Three sections,
        three kinds of laser. You cannot shoot them. Reach the
        release panel.
     3. Dr. Voss watches from a sealed booth. He sends SPECIMEN 09
        ("ARC") - an infected with an electrode rig bolted on.
     4. Kill ARC and the grid dies with him. The glass drops and
        Voss walks out.
     5. He explains himself, and tells you the machine only
        releases when his heart stops. Then he draws a sidearm.
     6. One shot ends him. Put the tube in the machine. Done.
   THE MAP BELOW IS THE LEVEL:
       #  wall            .  floor
       P  player start    Y  supply cache
       R  release panel   D  door into Voss's room
       E  ARC spawn       =  the glass
       V  Dr. Voss        M  the dispersal machine
       C  coin            A  ammo            I  injector

   Every row must be exactly 38 characters, and there must be 16. */
#include "game.h"
#include <math.h>
#include <string.h>
#include "beams.h"

#define MAP_COLS 38
#define MAP_ROWS 16

static const char *MAP[MAP_ROWS] = {
"######################################",
"######################################",
"#######################........#######",
"######................#......A.#######",
"######...C.........C..#..C.....=.....#",
"######................#........=.....#",
"#.....................#........=.V.M.#",
"#.P..................RD........=.....#",
"#...Y.................#....E...=...C.#",
"#.....................#........=.....#",
"######................#........#######",
"######....C...I.....A.#...C....#######",
"######................#.I......#######",
"#######################........#######",
"######################################",
"######################################",
};

/* SPECIMEN 09, "ARC"

   Not a separate character - an infected like every other one in
   this game, with an electrode rig bolted to his chest.

   Same grammar as the zombies: he telegraphs, he commits, and
   when he misses he is helpless.
     BOLT    winds up, then fires a three-shot fan
     PULSE   a ring closes in on him, then a shockwave goes out.
             Be outside it when it lands.
     BLINK   teleports, so you cannot pin him in a corner
     DRAINED the punish window after a pulse: +50% damage taken */
enum { ARC_CHASE, ARC_BOLTWIND, ARC_PULSEWIND, ARC_PULSE, ARC_DRAINED, ARC_BLINK };

#define ARC_MAX_HP       ARC_HEALTH        /* see tuning.h */
#define ARC_PULSE_RADIUS ARC_PULSE_RANGE

static Rectangle arcBox;
static int       arcHP;
static int       arcState;
static float     arcTimer, arcHitFlash, arcRing, arcPulse;
static bool      arcAlive;
static Vector2   arcKnock;

/* DR. VOSS
   He starts sealed in the booth. When ARC dies the glass drops
   and he walks out to meet you. He talks, then he draws.
   One hit of anything ends him. */
static Rectangle vossBox;
static Vector2   vossTarget;
static bool      vossWalking, vossArrived, vossSpoken, vossArmed, vossDead;
static float     vossWait, vossFire;

static const char *VOSS_SPEAKER[] = {
    "YOU","VOSS","YOU","VOSS","VOSS","VOSS","VOSS","VOSS","YOU","VOSS",
    "VOSS","VOSS","VOSS","YOU","VOSS","YOU","YOU","YOU","YOU","VOSS",
    "VOSS","VOSS","VOSS","VOSS","YOU","VOSS","VOSS",""
};
static const char *VOSS_LINE[] = {
    "Why?",
    "You want a reason. Everyone wants a reason.",
    "I want yours.",
    "I sent them models for twenty-two years.",
    "Water tables. Crop yields. The year it stops working.",
    "They read the summary page.",
    "Then they cut my funding and gave my lab",
    "to a man who made fertiliser.",
    "So this was about your funding.",
    "This was about being right in a room",
    "where nobody was listening.",
    "I asked for twenty-two years.",
    "One morning I stopped asking.",
    "I understand you.",
    "No. You are here to shoot me.",
    "My unit had forty-one people in it.",
    "Not one of them sat on your funding board.",
    "You were right about the world.",
    "You were wrong about what to do with it.",
    "...",
    "Then you should know one more thing.",
    "The dispersal unit reads my lifesign.",
    "While my heart is beating it will not release.",
    "I built it that way so nobody could take it from me.",
    "You made yourself the lock.",
    "I made myself necessary. It is the same thing.",
    "Go on. That is what you walked all this way for.",
    "HE IS REACHING FOR SOMETHING."
};
#define VOSS_LINE_COUNT 28

static bool  dialogueOn;
static int   dialogueLine;

/* THE SUPPLY CACHE - what all those coins were for */
typedef struct ShopItem {
    const char *name;
    int         cost;
    const char *note;
} ShopItem;

static const ShopItem SHOP[] = {
    { "MEDKIT",   SHOP_MEDKIT_COST,   "+40 health" },
    { "INJECTOR", SHOP_INJECTOR_COST, "+1 injector" },
    { "AMMO",     SHOP_AMMO_COST,     "+16 rounds, +6 shells" },
    { "PLATING",  SHOP_PLATING_COST,  "the grid burns you half as hard" },
};
#define SHOP_COUNT 4

static bool  shopOpen, hasPlating;
static float shopFlash;

/*  level state  */
static Rectangle walls[MAX_WALLS];      static int wallCount;
static Pickup    pickups[MAX_PICKUPS];  static int pickupCount;
static Bullet    bullets[MAX_BULLETS];  /* yours */
static Bullet    foeShots[MAX_BULLETS]; /* ARC's and Voss's */
static Camera2D  cam;

static Rectangle supplyCache, releasePanel, gridDoor, machine;
static int       gridDoorWall;
static bool      gridDoorOpen, vossGreeted, tubePlaced;
static float     endTimer, lastDt;

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

/*  setup  */
void Level3_Init(void)
{
    int r, c, i;
    float px = 150.0f, py = 460.0f;

    wallCount = 0; pickupCount = 0;
    gridDoorOpen = false; vossGreeted = false; tubePlaced = false;
    endTimer = 0.0f; lastDt = 0.0f;
    shopOpen = false; hasPlating = false; shopFlash = 0.0f;
    dialogueOn = false; dialogueLine = 0;

    arcAlive = false; arcHP = ARC_MAX_HP; arcState = ARC_CHASE;
    arcTimer = 1.0f; arcHitFlash = 0.0f; arcRing = 0.0f; arcPulse = 0.0f;
    arcKnock.x = 0.0f; arcKnock.y = 0.0f;

    vossWalking = false; vossArrived = false; vossSpoken = false;
    vossArmed = false; vossDead = false; vossWait = 0.0f; vossFire = 0.0f;

    game.level = 3;
    game.kills = 0; game.hits = 0; game.peakInfection = 0.0f;

    for (i = 0; i < MAX_BULLETS; i++) { bullets[i].active = false; foeShots[i].active = false; }
    FxReset();
    BuildGrid();

    wallCount = BuildWallsFromMap(MAP, MAP_ROWS, walls, MAX_WALLS);

    for (r = 0; r < MAP_ROWS; r++) {
        for (c = 0; c < MAP_COLS; c++) {
            float x = c * TILE, y = r * TILE;
            switch (MAP[r][c]) {
                case 'P': px = x + 19.0f; py = y + 19.0f; break;
                case 'C': AddPickup(x, y, PICK_COIN, COIN_PICKUP_VALUE); break;
                case 'A': AddPickup(x, y, PICK_AMMO, 10);    break;
                case 'I': AddPickup(x, y, PICK_INJECTOR, 1); break;

                case 'Y': supplyCache  = (Rectangle){ x, y, TILE, TILE }; break;
                case 'R': releasePanel = (Rectangle){ x, y, TILE, TILE }; break;
                case 'M': machine      = (Rectangle){ x, y, TILE, TILE }; break;

                case 'D':
                    gridDoor = (Rectangle){ x, y, TILE, TILE };
                    break;

                case 'E':
                    arcBox = (Rectangle){ x + 15.0f, y + 13.0f, 34.0f, 38.0f };
                    arcAlive = true;
                    break;

                case 'V':
                    vossBox = (Rectangle){ x + 18.0f, y + 14.0f, 28.0f, 38.0f };
                    break;

                case '=':
                    AddDynamicWall((Rectangle){ x, y, TILE, TILE });
                    break;

                default: break;
            }
        }
    }

    gridDoorWall = AddDynamicWall(gridDoor);

    /* he walks out to here when the glass drops */
    vossTarget.x = 29.0f * TILE;
    vossTarget.y = 6.0f * TILE + 14.0f;

    ResetPlayer(px, py);
    RestoreLoadout();
    PlacePlayer(px, py);

    cam = MakeCamera(1.0f);
    ShowMessage("the grid is live  -  you cannot shoot it, cross it");
}

/*  the shop  */
static void BuyItem(int index)
{
    const ShopItem *it;
    if (index < 0 || index >= SHOP_COUNT) return;
    it = &SHOP[index];

    if (game.coins < it->cost) { shopFlash = 0.7f; return; }
    if (strcmp(it->name, "PLATING") == 0 && hasPlating) { shopFlash = 0.7f; return; }

    game.coins -= it->cost;

    if (strcmp(it->name, "MEDKIT") == 0) {
        player.health += SHOP_MEDKIT_HEAL;
        if (player.health > player.maxHealth) player.health = player.maxHealth;
        ShowMessage("+40 health");
    } else if (strcmp(it->name, "INJECTOR") == 0) {
        player.injectors++;
        ShowMessage("+1 injector");
    } else if (strcmp(it->name, "AMMO") == 0) {
        player.ammo[WEAP_PISTOL] += 16;
        if (player.hasWeapon[WEAP_SHOTGUN]) player.ammo[WEAP_SHOTGUN] += 6;
        ShowMessage("resupplied");
    } else {
        hasPlating = true;
        ShowMessage("plating fitted  -  the grid hurts less");
    }
}

static void UpdateShop(void)
{
    if (IsKeyPressed(KEY_ONE))   BuyItem(0);
    if (IsKeyPressed(KEY_TWO))   BuyItem(1);
    if (IsKeyPressed(KEY_THREE)) BuyItem(2);
    if (IsKeyPressed(KEY_FOUR))  BuyItem(3);
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_F)) shopOpen = false;
}
/* ARC  */
static void ArcFireBolt(void)
{
    Vector2 ac = RectCenter(arcBox);
    Vector2 pc = RectCenter(player.box);
    float base = atan2f(pc.y - ac.y, pc.x - ac.x);
    int k;
    for (k = -1; k <= 1; k++) {
        float a = base + (float)k * 0.16f;
        Vector2 d; d.x = cosf(a); d.y = sinf(a);
        SpawnBullet(foeShots, ac, d, ARC_BOLT_SPEED, ARC_BOLT_DAMAGE);
    }
}

static void UpdateArc(float dt)
{
    Vector2 ac, pc, dir;
    float d;

    if (!arcAlive) return;

    ac = RectCenter(arcBox);
    pc = RectCenter(player.box);
    d  = Dist(ac, pc);
    dir = Norm((Vector2){ pc.x - ac.x, pc.y - ac.y });

    if (arcHitFlash > 0.0f) arcHitFlash -= dt;
    arcTimer -= dt;

    arcKnock.x = Approach(arcKnock.x, 0.0f, 700.0f * dt);
    arcKnock.y = Approach(arcKnock.y, 0.0f, 700.0f * dt);
    if (arcKnock.x != 0.0f || arcKnock.y != 0.0f)
        MoveBox(&arcBox, arcKnock.x * dt, arcKnock.y * dt, walls, wallCount);

    switch (arcState) {
        case ARC_CHASE:
            MoveBox(&arcBox, dir.x * ARC_SPEED * dt, dir.y * ARC_SPEED * dt, walls, wallCount);
            if (arcTimer <= 0.0f) {
                int roll = GetRandomValue(0, 99);
                if (d < 200.0f && roll < 55) {
                    arcState = ARC_PULSEWIND; arcTimer = ARC_PULSE_WINDUP; arcRing = ARC_PULSE_RADIUS;
                } else if (roll < 80) {
                    arcState = ARC_BOLTWIND; arcTimer = ARC_BOLT_WINDUP;
                } else {
                    arcState = ARC_BLINK; arcTimer = 0.35f;
                }
            }
            break;

        case ARC_BOLTWIND:
            if (arcTimer <= 0.0f) {
                ArcFireBolt();
                arcState = ARC_CHASE; arcTimer = 1.1f;
            }
            break;

        case ARC_PULSEWIND:
            arcRing = ARC_PULSE_RADIUS * (arcTimer / ARC_PULSE_WINDUP);   /* ring closes in */
            if (arcTimer <= 0.0f) { arcState = ARC_PULSE; arcTimer = 0.35f; arcPulse = 0.0f; }
            break;

        case ARC_PULSE:
            arcPulse = ARC_PULSE_RADIUS * (1.0f - arcTimer / 0.35f);
            if (fabsf(d - arcPulse) < 26.0f && player.hurtTimer <= 0.0f)
                DamagePlayer(ARC_PULSE_DAMAGE, INFECT_STRONG, ac);
            if (arcTimer <= 0.0f) { arcState = ARC_DRAINED; arcTimer = ARC_DRAINED_TIME; AddShake(6.0f); }
            break;

        case ARC_DRAINED:                       /* the punish window */
            if (arcTimer <= 0.0f) { arcState = ARC_CHASE; arcTimer = 0.8f; }
            break;

        case ARC_BLINK:
            if (arcTimer <= 0.0f) {
                int tries;
                SpawnParticles(ac, 16, (Color){ 140, 216, 255, 255 }, 200.0f, 0.4f, 3.0f);
                for (tries = 0; tries < 30; tries++) {
                    float ang = (float)GetRandomValue(0, 628) / 100.0f;
                    float rr  = 190.0f + (float)GetRandomValue(0, 160);
                    float nx = pc.x + cosf(ang) * rr;
                    float ny = pc.y + sinf(ang) * rr;
                    Rectangle test = { nx - arcBox.width * 0.5f, ny - arcBox.height * 0.5f,
                                       arcBox.width, arcBox.height };
                    int w; bool blocked = false;
                    if (nx < 23.0f * TILE + 40.0f || nx > 36.0f * TILE) continue;
                    if (ny < 2.0f * TILE + 40.0f  || ny > 13.0f * TILE) continue;
                    for (w = 0; w < wallCount; w++)
                        if (CheckCollisionRecs(test, walls[w])) { blocked = true; break; }
                    if (!blocked) { arcBox.x = test.x; arcBox.y = test.y; break; }
                }
                SpawnParticles(RectCenter(arcBox), 16,
                               (Color){ 140, 216, 255, 255 }, 200.0f, 0.4f, 3.0f);
                arcState = ARC_CHASE; arcTimer = 0.7f;
            }
            break;

        default: break;
    }

    if (CheckCollisionRecs(arcBox, player.box) && player.hurtTimer <= 0.0f)
        DamagePlayer(ARC_TOUCH_DAMAGE, INFECT_STRONG, ac);
}

static void DamageArc(int dmg, Vector2 dir)
{
    int actual = (arcState == ARC_DRAINED)
               ? (dmg * STUN_DAMAGE_BONUS_NUM) / STUN_DAMAGE_BONUS_DEN : dmg;

    arcHP -= actual;
    arcHitFlash = 0.12f;
    arcKnock.x = dir.x * 140.0f;
    arcKnock.y = dir.y * 140.0f;

    SpawnBlood(RectCenter(arcBox), dir, 6);
    SpawnParticles(RectCenter(arcBox), 4, (Color){ 156, 224, 255, 255 }, 130.0f, 0.25f, 3.0f);
    AddHitstop(0.03f);
    AddShake(2.5f);

    if (arcHP <= 0 && arcAlive) {
        int i;
        arcAlive = false;
        SetGridLive(false);               /* he was powering the grid */
        game.kills++;
        AddCoins(COIN_ARC_KILL);
        SpawnParticles(RectCenter(arcBox), 40,
                       (Color){ 140, 216, 255, 255 }, 260.0f, 0.8f, 4.0f);
        FlashScreen((Color){ 150, 220, 255, 128 }, 0.7f);
        AddShake(18.0f);
        AddHitstop(0.2f);
        ShowMessage("SPECIMEN 09 IS DOWN  -  the glass is dropping");

        /* the glass drops: shove every '=' wall off the map */
        for (i = 0; i < wallCount; i++) {
            if (walls[i].width == TILE && walls[i].height == TILE &&
                walls[i].x >= 31.0f * TILE && walls[i].x <= 31.5f * TILE) {
                walls[i].x = -99999.0f; walls[i].y = -99999.0f;
            }
        }
        vossWalking = true;
    }
}