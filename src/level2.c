/* 
   

   Solve a riddle -> the vault opens -> take the antidote tube.
   Walk to the exit -> it is locked -> solve a second, different
   riddle -> out.
 */


#include "game.h"
#include <math.h>
#include <string.h>

#define MAP_COLS 28
#define MAP_ROWS 14

static const char *MAP[MAP_ROWS] = {
"############################",
"############################",
"#................#........##",
"#...C......C.....#........##",
"#................V....B...##",
"#........C.......#........##",
"#..............C.#........##",
"#.P..............###########",
"#................#........##",
"#............C...#........##",
"#..C..C..........X........##",
"#................#........##",
"############################",
"############################",
};



#define MAX_RIDDLE_LINES 8

typedef struct Riddle 
{
    const char *title;
    const char *note;
    const char *lines[MAX_RIDDLE_LINES];
    int         lineCount;
    const char *answer;
    const char *hint;
} Riddle;

static const Riddle VAULT_RIDDLE = 
{
    "VAULT 4-B  -  SEALED",
    "EACH RIDDLE IS ONE DIGIT. IN ORDER.",
    {
        "1.  I AM ODD. TAKE AWAY A LETTER",
        "    AND I BECOME EVEN.",
        "",
        "2.  LAY ME ON MY SIDE AND I AM FOREVER.",
        "",
        "3.  MY DIVISORS ARE ONE, TWO AND THREE.",
        "    ADD THEM AND YOU HAVE ME."
    },
    7,
    "786",
    "HINT - the first digit is 7."
};

static const Riddle EXIT_RIDDLE = 
{
    "EXIT  -  SEALED",
    "ONE RIDDLE. THE WHOLE CODE.",
    {
        "ALL THREE OF MY DIGITS ARE ODD.",
        "",
        "MY FIRST IS MY LAST PLUS FOUR.",
        "",
        "MY MIDDLE IS THE SMALLEST ODD NUMBER.",
        "",
        "MY DIGITS TOTAL ELEVEN."
    },
    7,
    "713",
    "HINT - the middle digit is the easy one. Start there."
};


enum { LOCK_VAULT, LOCK_EXIT };

static Rectangle walls[MAX_WALLS];      static int wallCount;
static Pickup    pickups[MAX_PICKUPS];  static int pickupCount;
static Camera2D  cam;

static Rectangle vaultDoor, exitLock;
static int       vaultDoorWall, exitLockWall;
static bool      vaultOpen, exitOpen, hasTube;

static bool      keypadOpen;
static int       lockTarget;
static char      keypadEntry[4];
static int       keypadLen;
static float     keypadFlash;
static int       wrongVault, wrongExit;
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

static const Riddle *CurrentRiddle(void)
{
    return (lockTarget == LOCK_VAULT) ? &VAULT_RIDDLE : &EXIT_RIDDLE;
}

static int WrongCount(void)
{
    return (lockTarget == LOCK_VAULT) ? wrongVault : wrongExit;
}

void Level2_Init(void)
{
    int r, c;
    float px = 150.0f, py = 460.0f;

    wallCount = 0; pickupCount = 0;
    vaultOpen = false; exitOpen = false; hasTube = false;
    keypadOpen = false; keypadLen = 0; keypadFlash = 0.0f;
    lockTarget = LOCK_VAULT;
    wrongVault = 0; wrongExit = 0;
    keypadEntry[0] = '\0';
    lastDt = 0.0f;

    game.level = 2;
    game.kills = 0; game.hits = 0; game.peakInfection = 0.0f;

    FxReset();
    wallCount = BuildWallsFromMap(MAP, MAP_ROWS, walls, MAX_WALLS);

    for (r = 0; r < MAP_ROWS; r++) 
    {
        for (c = 0; c < MAP_COLS; c++) 
        {
            float x = c * TILE, y = r * TILE;
            switch (MAP[r][c]) {
                case 'P': px = x + 19.0f; py = y + 19.0f; break;
                case 'C': AddPickup(x, y, PICK_COIN, COIN_PICKUP_VALUE); break;
                case 'B': AddPickup(x, y, PICK_TUBE, 1);  break;
                case 'V':
                    vaultDoor.x = x; vaultDoor.y = y;
                    vaultDoor.width = TILE; vaultDoor.height = TILE;
                    break;
                case 'X':
                    exitLock.x = x; exitLock.y = y;
                    exitLock.width = TILE; exitLock.height = TILE;
                    break;
                default: break;
            }
        }
    }

    
    vaultDoorWall = AddDynamicWall(vaultDoor);
    exitLockWall  = AddDynamicWall(exitLock);

    
    ResetPlayer(px, py);
    RestoreLoadout();
    PlacePlayer(px, py);

    cam = MakeCamera(1.0f);
    ShowMessage("BLACKWELL  -  find the vault");
}


static void OpenKeypad(int which)
{
    keypadOpen = true;
    lockTarget = which;
    keypadLen = 0;
    keypadEntry[0] = '\0';
}

static void SubmitCode(void)
{
    const Riddle *rd = CurrentRiddle();

    if (strcmp(keypadEntry, rd->answer) != 0)
     {
        keypadFlash = 0.9f;
        keypadLen = 0;
        keypadEntry[0] = '\0';
        if (lockTarget == LOCK_VAULT) wrongVault++; else wrongExit++;
        AddShake(3.0f);
        return;                     
    }

    keypadOpen = false;
    keypadLen = 0;
    keypadEntry[0] = '\0';

    if (lockTarget == LOCK_VAULT) 
    {
        vaultOpen = true;
        DisableWall(vaultDoorWall);
        AddCoins(COIN_LOCK_OPENED);
        ShowMessage("VAULT OPEN  -  take the tube");
    } 
    else 

    {
        exitOpen = true;
        DisableWall(exitLockWall);
        AddCoins(COIN_LOCK_OPENED);
        ShowMessage("EXIT OPEN  -  go");
    }
    FlashScreen((Color){ 200, 220, 150, 115 }, 0.5f);
    AddShake(10.0f);
    SpawnParticles(RectCenter(player.box), 20,(Color){ 200, 230, 160, 255 }, 150.0f, 0.5f, 3.0f);
}

static void UpdateKeypad(void)
{
    int ch;

    
    while ((ch = GetCharPressed()) != 0)
    {
        if (ch >= '0' && ch <= '9' && keypadLen < 3) 
        {
            keypadEntry[keypadLen++] = (char)ch;
            keypadEntry[keypadLen] = '\0';
        }
    }

    if (IsKeyPressed(KEY_BACKSPACE) && keypadLen > 0) 
    {
        keypadLen--;
        keypadEntry[keypadLen] = '\0';
    }
    if (IsKeyPressed(KEY_ESCAPE)) 
    {
        keypadOpen = false;
        keypadLen = 0;
        keypadEntry[0] = '\0';
    }
    if (IsKeyPressed(KEY_ENTER) && keypadLen == 3) SubmitCode();
}


void Level2_Update(float dt)
{
    int i;
    Rectangle nearVault, nearExit;

    lastDt = dt;
    game.levelTime += dt;
    game.level = 2;
    game.retryScene = SCENE_LEVEL2;

    if (keypadFlash > 0.0f) keypadFlash -= dt;

    
    if (keypadOpen)
    {
        UpdateKeypad();
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

    for (i = 0; i < pickupCount; i++) 
    {
        if (pickups[i].taken) continue;
        if (!CheckCollisionRecs(player.box, pickups[i].box)) continue;
        pickups[i].taken = true;
        if (pickups[i].type == PICK_TUBE)
         {
            hasTube = true;
            AddCoins(COIN_TUBE_TAKEN);
            FlashScreen((Color){ 160, 255, 220, 140 }, 0.7f);
            ShowMessage("ANTIDOTE TUBE SECURED  -  find the exit");
        } else {
            AddCoins(pickups[i].amount);
        }
    }

    nearVault = (Rectangle){ vaultDoor.x - 30.0f, vaultDoor.y - 30.0f,vaultDoor.width + 60.0f, vaultDoor.height + 60.0f };
    nearExit  = (Rectangle){ exitLock.x - 30.0f, exitLock.y - 30.0f,exitLock.width + 60.0f, exitLock.height + 60.0f };

    if (!vaultOpen && CheckCollisionRecs(player.box, nearVault)) 
    {
        if (IsKeyPressed(KEY_F)) OpenKeypad(LOCK_VAULT);
        else if (game.msgTimer <= 0.0f) ShowMessage("vault lock  [F]");
    } else if (!exitOpen && CheckCollisionRecs(player.box, nearExit))
     {
        if (!hasTube) 
        {
            if (game.msgTimer <= 0.0f) ShowMessage("not without the tube");
        } else if (IsKeyPressed(KEY_F))
        {
            OpenKeypad(LOCK_EXIT);
        }
    }

    if (exitOpen && hasTube && CheckCollisionRecs(player.box, exitLock))
    {
        AddCoins(COIN_LEVEL_FINISHED);
        SaveLoadout();
        game.scene = SCENE_RESULTS;
        return;
    }

    if (player.health <= 0) PlayerDied(SCENE_LEVEL2);
}


static Rectangle ObjectiveTarget(void)
{
    int i;
    if (!vaultOpen) return vaultDoor;
    if (!hasTube) {
        for (i = 0; i < pickupCount; i++)
            if (pickups[i].type == PICK_TUBE && !pickups[i].taken)
                return pickups[i].box;
    }
    return exitLock;
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
    DrawText(lbl, (int)(sx - MeasureText(lbl, 12) * 0.5f), (int)(sy + 20), 12,(Color){ 240, 196, 62, (unsigned char)(200.0f * fade) });
}

static void DrawKeypad(void)
{
    const Riddle *rd = CurrentRiddle();
    bool showHint = WrongCount() >= 3;
    int w = 660;
    int h = 210 + rd->lineCount * 26 + (showHint ? 30 : 0);
    int x = SCREEN_W / 2 - w / 2;
    int y = SCREEN_H / 2 - h / 2;
    int ly, i;
    Color border = (keypadFlash > 0.0f) ? (Color){ 228, 72, 72, 255 }: (Color){ 180, 156, 196, 255 };

    DrawRectangle(x, y, w, h, (Color){ 6, 6, 9, 245 });
    DrawRectangleLinesEx((Rectangle){ (float)x, (float)y, (float)w, (float)h },2.0f, border);

    DrawText(rd->title, x + 28, y + 20, 18, (Color){ 200, 180, 212, 255 });
    DrawRectangle(x + 28, y + 48, w - 56, 1, (Color){ 46, 42, 54, 255 });
    DrawText(rd->note, x + 28, y + 58, 13, (Color){ 124, 118, 144, 255 });

    ly = y + 86;
    for (i = 0; i < rd->lineCount; i++)
    {
        if (rd->lines[i][0] == '\0') { ly += 12; continue; }
        DrawText(rd->lines[i], x + 34, ly, 17, (Color){ 230, 225, 212, 255 });
        ly += 26;
    }

    if (showHint) 
    {
        DrawText(rd->hint, x + 34, ly + 8, 14, (Color){ 216, 168, 96, 255 });
        ly += 30;
    }

    for (i = 0; i < 3; i++) 
    {
        char buf[2];
        Color col;
        buf[0] = (i < keypadLen) ? keypadEntry[i] : '_';
        buf[1] = '\0';
        if (keypadFlash > 0.0f)  col = (Color){ 228, 72, 72, 255 };
        else if (i < keypadLen)  col = (Color){ 240, 196, 62, 255 };
        else                     col = (Color){ 74, 74, 84, 255 };
        DrawText(buf, x + w / 2 - 62 + i * 50, y + h - 94, 42, col);
    }

    if (keypadFlash > 0.0f) 
    {
        const char *t = "WRONG  -  try again, it costs you nothing";
        DrawText(t, x + w / 2 - MeasureText(t, 13) / 2, y + h - 32, 13,(Color){ 228, 72, 72, 255 });
    }
     else 
    {
        const char *t = "[0-9] type     [ENTER] submit     [ESC] back";
        DrawText(t, x + w / 2 - MeasureText(t, 13) / 2, y + h - 32, 13, (Color){ 110, 100, 128, 255 });
    }
}

void Level2_Draw(void)
{
    BeginMode2D(cam);
        DrawFloor(MAP_COLS, MAP_ROWS);
        DrawWalls(walls, wallCount);

        if (!vaultOpen) {
            DrawRectangleRec(vaultDoor, (Color){ 74, 62, 82, 255 });
            DrawRectangleLinesEx(vaultDoor, 3.0f, (Color){ 180, 156, 196, 255 });
            DrawText("VAULT [F]", (int)vaultDoor.x - 10, (int)vaultDoor.y - 22, 14,(Color){ 216, 196, 228, 255 });
        }

        if (!exitOpen) 
        {
            DrawRectangleRec(exitLock,
                hasTube ? (Color){ 90, 74, 50, 255 } : (Color){ 74, 50, 50, 255 });
            DrawRectangleLinesEx(exitLock, 3.0f,
                hasTube ? (Color){ 216, 180, 120, 255 } : (Color){ 160, 112, 112, 255 });
            DrawText(hasTube ? "EXIT [F]" : "EXIT - LOCKED",(int)exitLock.x - 22, (int)exitLock.y - 22, 14,hasTube ? (Color){ 232, 200, 144, 255 }: (Color){ 192, 144, 144, 255 });
        } 
        
        else 
        
        {
            DrawRectangleRec(exitLock, (Color){ 90, 200, 120, 255 });
            DrawText("EXIT", (int)exitLock.x + 6, (int)exitLock.y - 22, 16, (Color){ 210, 220, 215, 255 });
        }

        DrawPickups(pickups, pickupCount, lastDt);
        DrawParticles();
        DrawPlayer();
    EndMode2D();

    DrawDarkness(cam, RectCenter(player.box), LIGHT_RADIUS, DARKNESS_ALPHA);
    DrawInfectionOverlay();
    DrawVignette(70);
    DrawFloatingText(cam);
    DrawFlash();

    if (!keypadOpen) DrawObjectivePointer();

    DrawHUD(!vaultOpen ? "OBJECTIVE: unlock the vault" : (!hasTube  ? "OBJECTIVE: take the tube": (!exitOpen ? "OBJECTIVE: unlock the exit" : "OBJECTIVE: get out")));

    if (player.injectTimer > 0.0f)
        DrawHoldBar(player.injectTimer / INJECT_HOLD_TIME, "INJECTING - DON'T MOVE");

    if (keypadOpen) DrawKeypad();
}