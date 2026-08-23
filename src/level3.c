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
/*  update  */
void Level3_Update(float dt)
{
    int i;
    Rectangle zone;

    lastDt = dt;
    game.levelTime += dt;
    game.level = 3;
    game.retryScene = SCENE_LEVEL3;

    if (shopFlash > 0.0f) shopFlash -= dt;

    /* the shop swallows everything else while it is up */
    if (shopOpen) {
        UpdateShop();
        UpdateCameraFollow(&cam, dt, 0.0f);
        return;
    }

    AimAtMouse(cam);
    UpdatePlayerMovement(walls, wallCount, dt);
    UpdateCameraFollow(&cam, dt, CAMERA_LEAN);
    UpdateInfection(0.0f, dt);
    UpdateInjector(dt);

    if (IsKeyPressed(KEY_ONE))   SwitchWeapon(WEAP_KNIFE);
    if (IsKeyPressed(KEY_TWO))   SwitchWeapon(WEAP_PISTOL);
    if (IsKeyPressed(KEY_THREE)) SwitchWeapon(WEAP_SHOTGUN);

    /*  the supply cache  */
    zone = (Rectangle){ supplyCache.x - 28.0f, supplyCache.y - 28.0f,
                        supplyCache.width + 56.0f, supplyCache.height + 56.0f };
    if (CheckCollisionRecs(player.box, zone)) {
        if (IsKeyPressed(KEY_F)) shopOpen = true;
        else if (game.msgTimer <= 0.0f) ShowMessage("supply cache  [F]  -  spend your coins");
    }

    /*  the grid (lives in beams.c)  */
    UpdateBeams(dt);
    if (GridIsLive()) {
        for (i = 0; i < BeamCount(); i++) {
            /* the antechamber goes quiet behind you; his room comes alive */
            if (BeamInVossRoom(i) && !gridDoorOpen) continue;
            if (!BeamInVossRoom(i) && gridDoorOpen) continue;
            if (BeamHitsPlayer(i) && player.hurtTimer <= 0.0f)
                DamagePlayer(hasPlating ? BEAM_DAMAGE_PLATED : BEAM_DAMAGE, 0,
                             RectCenter(player.box));
        }
    }

    /*  shooting  */
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && player.injectTimer <= 0.0f && !dialogueOn)
        FireWeapon(bullets, NULL, 0);

    UpdateBullets(bullets, walls, wallCount, dt);
    UpdateBullets(foeShots, walls, wallCount, dt);

    /* their shots hitting you */
    for (i = 0; i < MAX_BULLETS; i++) {
        if (!foeShots[i].active) continue;
        if (CheckCollisionCircleRec(foeShots[i].pos, 4.0f, player.box)) {
            foeShots[i].active = false;
            if (player.hurtTimer <= 0.0f)
                DamagePlayer(foeShots[i].damage, INFECT_STRONG, foeShots[i].pos);
        }
    }

    /* your shots hitting ARC */
    if (arcAlive) {
        for (i = 0; i < MAX_BULLETS; i++) {
            if (!bullets[i].active) continue;
            if (CheckCollisionCircleRec(bullets[i].pos, 4.0f, arcBox)) {
                Vector2 d = Norm(bullets[i].vel);
                bullets[i].active = false;
                DamageArc(bullets[i].damage, d);
                if (!arcAlive) break;
            }
        }
        if (arcAlive && player.attackTimer > 0.14f &&
            CheckCollisionRecs(MeleeHitbox(), arcBox))
            DamageArc(30, player.aim);
    }

    /* Voss introduces the specimen the moment you step in, so ARC is not
       a stranger who appears out of nowhere */
    if (gridDoorOpen && !vossGreeted && arcAlive &&
        RectCenter(player.box).x > 23.0f * TILE) {
        vossGreeted = true;
        ShowMessage("VOSS: Specimen nine. He was a corporal, once.");
        FlashScreen((Color){ 120, 180, 220, 46 }, 0.5f);
    }

    UpdateArc(dt);

    /*  Voss walks out  */
    if (vossWalking && !vossArrived) {
        Vector2 vc = { vossBox.x, vossBox.y };
        float dx = vossTarget.x - vc.x, dy = vossTarget.y - vc.y;
        float dd = sqrtf(dx * dx + dy * dy);
        if (dd < 6.0f) { vossArrived = true; vossWalking = false; }
        else {
            vossBox.x += (dx / dd) * VOSS_WALK_SPEED * dt;
            vossBox.y += (dy / dd) * VOSS_WALK_SPEED * dt;
        }
    }

    /* He starts talking on his own. You never have to walk up to him. */
    if (vossArrived && !dialogueOn && !vossSpoken && !tubePlaced) {
        vossWait += dt;
        if (vossWait > 1.0f) { vossSpoken = true; dialogueOn = true; dialogueLine = 0; }
    }

    if (dialogueOn) {
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
            dialogueLine++;
            if (dialogueLine >= VOSS_LINE_COUNT) {
                dialogueOn = false;
                dialogueLine = 0;
                vossArmed = true; vossFire = 1.0f;   /* he draws */
                ShowMessage("he has a sidearm");
            }
        }
    }

    /*  he fights, badly, and dies to one hit  */
    if (vossArmed && !vossDead) {
        Vector2 vc = RectCenter(vossBox);
        Vector2 pc = RectCenter(player.box);
        float d = Dist(vc, pc);
        bool killed = false;

        if (d < VOSS_BACKAWAY_RANGE) {                    /* backing away */
            Vector2 away = Norm((Vector2){ vc.x - pc.x, vc.y - pc.y });
            MoveBox(&vossBox, away.x * VOSS_BACKAWAY_SPEED * dt,
                              away.y * VOSS_BACKAWAY_SPEED * dt, walls, wallCount);
        }
        vossFire -= dt;
        if (vossFire <= 0.0f && d < 620.0f) {
            float a = atan2f(pc.y - vc.y, pc.x - vc.x)
                    + (float)GetRandomValue(-22, 22) / 100.0f;   /* a wild shot */
            Vector2 dir; dir.x = cosf(a); dir.y = sinf(a);
            vossFire = VOSS_FIRE_RATE;
            SpawnBullet(foeShots, vc, dir, VOSS_SHOT_SPEED, VOSS_SHOT_DAMAGE);
            SpawnParticles(vc, 3, (Color){ 255, 220, 140, 255 }, 80.0f, 0.12f, 2.5f);
        }

        for (i = 0; i < MAX_BULLETS; i++) {
            if (!bullets[i].active) continue;
            if (CheckCollisionCircleRec(bullets[i].pos, 4.0f, vossBox)) {
                bullets[i].active = false; killed = true; break;
            }
        }
        if (!killed && player.attackTimer > 0.14f &&
            CheckCollisionRecs(MeleeHitbox(), vossBox)) killed = true;

        if (killed) {
            vossDead = true;
            SpawnBlood(RectCenter(vossBox), (Vector2){ 0.0f, 1.0f }, 28);
            FlashScreen((Color){ 150, 20, 20, 140 }, 0.8f);
            AddShake(16.0f);
            AddHitstop(0.28f);
            AddCoins(COIN_VOSS_KILL);
            ShowMessage("the lock is open");
        }
    }

    /*  pickups  */
    for (i = 0; i < pickupCount; i++) {
        if (pickups[i].taken) continue;
        if (!CheckCollisionRecs(player.box, pickups[i].box)) continue;
        pickups[i].taken = true;
        switch (pickups[i].type) {
            case PICK_COIN: AddCoins(pickups[i].amount); break;
            case PICK_AMMO:
                player.ammo[WEAP_PISTOL] += pickups[i].amount;
                if (player.hasWeapon[WEAP_SHOTGUN]) player.ammo[WEAP_SHOTGUN] += 3;
                ShowMessage("resupplied");
                break;
            case PICK_INJECTOR:
                player.injectors++;
                ShowMessage("+1 injector");
                break;
            default: break;
        }
    }

    /*  the release panel opens his door  */
    if (!gridDoorOpen) {
        zone = (Rectangle){ releasePanel.x - 28.0f, releasePanel.y - 28.0f,
                            releasePanel.width + 56.0f, releasePanel.height + 56.0f };
        if (CheckCollisionRecs(player.box, zone)) {
            if (IsKeyPressed(KEY_F)) {
                gridDoorOpen = true;
                DisableWall(gridDoorWall);
                AddCoins(COIN_LOCK_OPENED);
                FlashScreen((Color){ 200, 220, 150, 115 }, 0.5f);
                AddShake(10.0f);
                ShowMessage("DOOR RELEASED  -  he is through there");
            } else if (game.msgTimer <= 0.0f) {
                ShowMessage("release panel  [F]");
            }
        }
    }

    /*  the machine: the whole reason for all of this  */
    if (!arcAlive && vossDead && !dialogueOn && !tubePlaced) {
        zone = (Rectangle){ machine.x - 28.0f, machine.y - 28.0f,
                            machine.width + 56.0f, machine.height + 56.0f };
        if (CheckCollisionRecs(player.box, zone)) {
            if (IsKeyPressed(KEY_F)) {
                tubePlaced = true;
                AddCoins(COIN_MACHINE);
                FlashScreen((Color){ 255, 255, 255, 240 }, 1.6f);
                AddShake(22.0f);
                ShowMessage("THE MACHINE IS RUNNING");
            } else if (game.msgTimer <= 0.0f) {
                ShowMessage("place the tube  [F]");
            }
        }
    }

    if (tubePlaced) {
        endTimer += dt;
        if (endTimer > 2.2f) { SaveLoadout(); game.scene = SCENE_RESULTS; return; }
    }

    if (player.health <= 0) PlayerDied(SCENE_LEVEL3);
}
/*  draw  */
static void DrawArc(void)
{
    Vector2 ac = RectCenter(arcBox);
    Color body;

    if (!arcAlive) {
        DrawRectangleRec(arcBox, (Color){ 52, 26, 28, 150 });
        return;
    }

    if (arcState == ARC_PULSEWIND)
        DrawCircleLines((int)ac.x, (int)ac.y, arcRing, (Color){ 255, 200, 90, 190 });
    if (arcState == ARC_PULSE)
        DrawCircleLines((int)ac.x, (int)ac.y, arcPulse, (Color){ 150, 220, 255, 240 });

    /* He is a zombie underneath: same green body, same red eyes as every
       other infected, just carrying a charge. */
    body = (Color){ 92, 122, 68, 255 };
    if (arcState == ARC_CHASE)     body = (Color){ 116, 156, 74, 255 };
    if (arcState == ARC_BOLTWIND)  body = (Color){ 245, 200, 74, 255 };
    if (arcState == ARC_PULSEWIND) body = (Color){ 245, 160, 60, 255 };
    if (arcState == ARC_DRAINED)   body = (Color){ 85, 96, 72, 255 };
    if (arcHitFlash > 0.0f)        body = (Color){ 255, 245, 245, 255 };

    DrawEllipse((int)ac.x, (int)(arcBox.y + arcBox.height - 2),
                arcBox.width * 0.5f, 6.0f, (Color){ 0, 0, 0, 90 });
    DrawRectangleRec(arcBox, body);
    DrawRectangleLinesEx(arcBox, 2.0f, (Color){ 24, 38, 20, 255 });
    DrawCircle((int)(arcBox.x + 9), (int)(arcBox.y + 11), 3.0f, (Color){ 228, 62, 62, 255 });
    DrawCircle((int)(arcBox.x + arcBox.width - 9), (int)(arcBox.y + 11), 3.0f,
               (Color){ 228, 62, 62, 255 });
    /* the electrode rig Voss bolted onto him */
    DrawRectangle((int)(arcBox.x + 2), (int)(arcBox.y + arcBox.height * 0.42f),
                  (int)(arcBox.width - 4), 5, (Color){ 90, 200, 240, 216 });

    if (arcState == ARC_DRAINED)
        DrawText("!", (int)ac.x - 4, (int)arcBox.y - 22, 22, (Color){ 185, 216, 245, 255 });
}

static void DrawVoss(void)
{
    Vector2 vc = RectCenter(vossBox);

    if (vossDead) {
        DrawRectangle((int)vossBox.x - 6, (int)(vossBox.y + vossBox.height - 16),
                      (int)vossBox.width + 12, 16, (Color){ 60, 30, 32, 190 });
        DrawEllipse((int)vc.x, (int)(vossBox.y + vossBox.height - 6), 34.0f, 12.0f,
                    (Color){ 120, 25, 28, 128 });
        return;
    }

    DrawEllipse((int)vc.x, (int)(vossBox.y + vossBox.height - 2),
                vossBox.width * 0.5f, 6.0f, (Color){ 0, 0, 0, 90 });
    /* a lab coat over dark clothes: he reads as a scientist, not a soldier */
    DrawRectangle((int)vossBox.x, (int)(vossBox.y + 10),
                  (int)vossBox.width, (int)(vossBox.height - 10),
                  (Color){ 220, 214, 196, 255 });
    DrawRectangle((int)(vossBox.x + vossBox.width * 0.5f - 4), (int)(vossBox.y + 14),
                  8, (int)(vossBox.height - 14), (Color){ 74, 70, 64, 255 });
    DrawRectangle((int)(vossBox.x + 5), (int)vossBox.y,
                  (int)(vossBox.width - 10), 13, (Color){ 200, 168, 140, 255 });
    DrawRectangleLinesEx((Rectangle){ vossBox.x, vossBox.y + 10,
                                      vossBox.width, vossBox.height - 10 },
                         2.0f, (Color){ 240, 234, 218, 255 });

    if (vossArmed) {
        Vector2 pc = RectCenter(player.box);
        float a = atan2f(pc.y - vc.y, pc.x - vc.x);
        DrawLineEx(vc, (Vector2){ vc.x + cosf(a) * 20.0f, vc.y + sinf(a) * 20.0f },
                   4.0f, (Color){ 225, 225, 232, 255 });
    }

    DrawText("DR. VOSS", (int)vc.x - 32, (int)vossBox.y - 12, 14,
             (Color){ 240, 234, 218, 255 });
}

static void DrawShop(void)
{
    int w = 560, h = 330;
    int x = SCREEN_W / 2 - w / 2, y = SCREEN_H / 2 - h / 2;
    int iy, i;

    DrawRectangle(x, y, w, h, (Color){ 6, 6, 9, 245 });
    DrawRectangleLinesEx((Rectangle){ (float)x, (float)y, (float)w, (float)h }, 2.0f,
        shopFlash > 0.0f ? (Color){ 228, 72, 72, 255 } : (Color){ 200, 168, 96, 255 });

    DrawText("SUPPLY CACHE", x + 28, y + 22, 20, (Color){ 240, 196, 62, 255 });
    DrawText(TextFormat("COINS  %d", game.coins), x + w - 160, y + 24, 15,
             (Color){ 230, 225, 212, 255 });
    DrawRectangle(x + 28, y + 54, w - 56, 1, (Color){ 46, 42, 34, 255 });
    DrawText("everything you picked up in the base and the station",
             x + 28, y + 66, 13, (Color){ 138, 130, 112, 255 });

    iy = y + 104;
    for (i = 0; i < SHOP_COUNT; i++) {
        bool owned  = (strcmp(SHOP[i].name, "PLATING") == 0 && hasPlating);
        bool afford = (game.coins >= SHOP[i].cost) && !owned;
        DrawText(TextFormat("[%d]  %s", i + 1, SHOP[i].name), x + 36, iy, 19,
                 afford ? (Color){ 230, 225, 212, 255 } : (Color){ 86, 80, 96, 255 });
        DrawText(SHOP[i].note, x + 210, iy + 3, 14,
                 afford ? (Color){ 138, 132, 150, 255 } : (Color){ 74, 70, 84, 255 });
        DrawText(owned ? "FITTED" : TextFormat("%d", SHOP[i].cost), x + w - 96, iy, 19,
                 owned  ? (Color){ 111, 191, 154, 255 }
                        : afford ? (Color){ 240, 196, 62, 255 }
                                 : (Color){ 106, 90, 58, 255 });
        iy += 46;
    }

    if (shopFlash > 0.0f)
        DrawText("not enough coins", x + 28, y + h - 34, 13, (Color){ 228, 72, 72, 255 });
    else
        DrawText("[1-4] buy      [F] or [ESC] close", x + 28, y + h - 34, 13,
                 (Color){ 110, 100, 128, 255 });
}

static void DrawDialogue(void)
{
    int bh = 118, by = SCREEN_H - bh - 24;
    const char *who  = VOSS_SPEAKER[dialogueLine];
    const char *line = VOSS_LINE[dialogueLine];
    const char *hint;

    DrawRectangle(60, by, SCREEN_W - 120, bh, (Color){ 6, 6, 9, 237 });
    DrawRectangleLinesEx((Rectangle){ 60.0f, (float)by,
                                      (float)(SCREEN_W - 120), (float)bh },
                         2.0f, (Color){ 94, 84, 104, 255 });

    if (who[0] != '\0')
        DrawText(who, 88, by + 20, 14,
                 (strcmp(who, "YOU") == 0) ? (Color){ 140, 200, 230, 255 }
                                           : (Color){ 216, 180, 120, 255 });
    DrawText(line, 88, by + 50, 24, (Color){ 230, 225, 212, 255 });

    hint = TextFormat("[SPACE] continue   %d / %d", dialogueLine + 1, VOSS_LINE_COUNT);
    DrawText(hint, SCREEN_W - 88 - MeasureText(hint, 12), by + bh - 24, 12,
             (Color){ 110, 100, 120, 255 });
}

static Rectangle ObjectiveTarget(void)
{
    if (!gridDoorOpen) return releasePanel;
    if (arcAlive)      return arcBox;
    if (!vossDead)     return vossBox;
    return machine;
}

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
    if (!gridDoorOpen) return "OBJECTIVE: cross the grid, reach the release";
    if (arcAlive)      return "OBJECTIVE: something is in the way";
    if (dialogueOn)    return "";
    if (!vossDead)     return "OBJECTIVE: he is the lock";
    if (!tubePlaced)   return "OBJECTIVE: place the tube";
    return "";
}

void Level3_Draw(void)
{
    BeginMode2D(cam);
        DrawFloor(MAP_COLS, MAP_ROWS);
        DrawWalls(walls, wallCount);

        /* supply cache */
        DrawRectangle((int)supplyCache.x + 8, (int)supplyCache.y + 8,
                      (int)TILE - 16, (int)TILE - 16, (Color){ 62, 58, 46, 255 });
        DrawRectangleLinesEx((Rectangle){ supplyCache.x + 8, supplyCache.y + 8,
                                          TILE - 16, TILE - 16 },
                             2.0f, (Color){ 200, 168, 96, 255 });
        DrawText("SUPPLY [F]", (int)supplyCache.x - 14, (int)supplyCache.y - 20, 13,
                 (Color){ 240, 196, 62, 255 });

        /* release panel */
        DrawRectangle((int)releasePanel.x + 10, (int)releasePanel.y + 10,
                      (int)TILE - 20, (int)TILE - 20,
                      gridDoorOpen ? (Color){ 62, 106, 78, 255 }
                                   : (Color){ 46, 68, 80, 255 });
        DrawRectangleLinesEx((Rectangle){ releasePanel.x + 10, releasePanel.y + 10,
                                          TILE - 20, TILE - 20 }, 2.0f,
                             gridDoorOpen ? (Color){ 140, 224, 168, 255 }
                                          : (Color){ 111, 168, 196, 255 });
        DrawText(gridDoorOpen ? "RELEASED" : "RELEASE [F]",
                 (int)releasePanel.x - 16, (int)releasePanel.y - 20, 13,
                 (Color){ 143, 208, 230, 255 });

        if (!gridDoorOpen) {
            DrawRectangleRec(gridDoor, (Color){ 74, 62, 82, 255 });
            DrawRectangleLinesEx(gridDoor, 3.0f, (Color){ 180, 156, 196, 255 });
        }

        /* the machine */
        {
            bool ready = (!arcAlive && vossDead && !dialogueOn && !tubePlaced);
            DrawRectangle((int)machine.x + 6, (int)machine.y + 6,
                          (int)TILE - 12, (int)TILE - 12,
                          tubePlaced ? (Color){ 124, 230, 192, 255 }
                                     : ready ? (Color){ 62, 106, 94, 255 }
                                             : (Color){ 58, 58, 68, 255 });
            DrawRectangleLinesEx((Rectangle){ machine.x + 6, machine.y + 6,
                                              TILE - 12, TILE - 12 }, 3.0f,
                                 tubePlaced ? (Color){ 216, 255, 242, 255 }
                                            : ready ? (Color){ 140, 224, 200, 255 }
                                                    : (Color){ 94, 94, 106, 255 });
            if (ready) {
                float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 2.2f);
                DrawCircleLines((int)RectCenter(machine).x, (int)RectCenter(machine).y,
                                30.0f + pulse * 8.0f,
                                (Color){ 140, 224, 200,
                                         (unsigned char)(76 + 114 * pulse) });
            }
            DrawText(tubePlaced ? "RUNNING" : "DISPERSAL",
                     (int)machine.x - 10, (int)machine.y - 20, 13,
                     (Color){ 156, 224, 200, 255 });
        }

        DrawPickups(pickups, pickupCount, lastDt);
        DrawBeams();
        DrawArc();
        DrawVoss();
        DrawBullets(bullets,  (Color){ 255, 224, 140, 255 });
        DrawBullets(foeShots, (Color){ 150, 220, 255, 255 });
        DrawParticles();
        DrawPlayer();
    EndMode2D();

    DrawDarkness(cam, RectCenter(player.box), LIGHT_RADIUS, DARKNESS_ALPHA);
    DrawInfectionOverlay();
    DrawVignette(70);
    DrawFloatingText(cam);
    DrawFlash();

    if (!shopOpen && !dialogueOn) DrawObjectivePointer();
    DrawHUD(CurrentObjective());

    /* ARC's health bar */
    if (arcAlive) {
        float pct = (float)arcHP / (float)ARC_MAX_HP;
        int bw = 560;
        DrawRectangle(SCREEN_W / 2 - bw / 2 - 4, SCREEN_H - 72, bw + 8, 32,
                      (Color){ 0, 0, 0, 184 });
        DrawRectangle(SCREEN_W / 2 - bw / 2, SCREEN_H - 68, bw, 24,
                      (Color){ 34, 50, 62, 255 });
        DrawRectangle(SCREEN_W / 2 - bw / 2, SCREEN_H - 68, (int)(bw * pct), 24,
                      (Color){ 79, 168, 216, 255 });
        DrawRectangleLines(SCREEN_W / 2 - bw / 2, SCREEN_H - 68, bw, 24,
                           (Color){ 207, 235, 255, 255 });
        DrawText("SPECIMEN 09  -  \"ARC\"", SCREEN_W / 2 - bw / 2, SCREEN_H - 92, 17,
                 (Color){ 220, 242, 255, 255 });
    }

    if (player.injectTimer > 0.0f)
        DrawHoldBar(player.injectTimer / INJECT_HOLD_TIME, "INJECTING - DON'T MOVE");

    if (dialogueOn) DrawDialogue();
    if (shopOpen)   DrawShop();

    /* the white-out at the very end */
    if (tubePlaced) {
        float t = endTimer / 1.8f;
        if (t > 1.0f) t = 1.0f;
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H,
                      (Color){ 255, 255, 255, (unsigned char)(255.0f * t) });
    }
}