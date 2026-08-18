#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include <stdbool.h>
#include "tuning.h"

#define SCREEN_W 1280
#define SCREEN_H 720

#define TILE          64.0f
#define MAX_WALLS     300
#define MAX_ZOMBIES   40
#define MAX_BULLETS   120
#define MAX_PICKUPS   64
#define MAX_PARTICLES 300

typedef enum Scene {
    SCENE_MENU,
    SCENE_STORY,     
    SCENE_LEVEL1,
    SCENE_LEVEL2,
    SCENE_LEVEL3,
    SCENE_RESULTS,
    SCENE_GAMEOVER,
    SCENE_VICTORY
} Scene;

typedef enum PickupType {
    PICK_COIN,
    PICK_AMMO,
    PICK_INJECTOR,
    PICK_KEYCARD,
    PICK_SHOTGUN,
    PICK_MAP,
    PICK_TUBE
} PickupType;

typedef enum WeaponType {
    WEAP_KNIFE,
    WEAP_PISTOL,
    WEAP_SHOTGUN,
    WEAP_COUNT
} WeaponType;

typedef enum ZState {
    Z_IDLE,  
    Z_WANDER,   
    Z_CHASE,     
    Z_WINDUP,    /* frozen, about to lunge - DODGE NOW */
    Z_LUNGE,     /* committed, straight line */
    Z_STUN       /* whiffed - takes 50% extra damage */
} ZState;

typedef enum ZType {
    ZT_SHAMBLER,   /* corridor tutorial enemy - dies to the pistol */
    ZT_RUNNER,
    ZT_CRAWLER,
    ZT_SOLDIER     /* armoured base guard - wants the shotgun     */
} ZType;

typedef struct Player {
    Rectangle box;
    Vector2   aim;
    Vector2   knockback;
    float     speed;
    int       health;
    int       maxHealth;

    float     infection;          /* 0..100 - the signature system */
    int       injectors;
    float     injectTimer;       

    int       weapon;
    bool      hasWeapon[WEAP_COUNT];
    int       ammo[WEAP_COUNT];

    float     fireCooldown;
    float     attackTimer;
    float     hurtTimer;
    bool      sprinting;

    bool      hasKeycard;
    bool      hasMap;
} Player;

typedef struct Zombie {
    Rectangle box;
    Vector2   knockback;
    Vector2   wanderDir;
    Vector2   lungeDir;
    float     speed;
    int       health;
    int       maxHealth;
    int       damage;
    int       infect;       
    int       type;
    int       state;
    float     stateTimer;
    float     hitTimer;
    float     cooldown;
    bool      alive;
    bool      dormant;   /* asleep: ignores you completely until woken */
} Zombie;

typedef struct Bullet {
    Vector2 pos, vel;
    float   life;
    int     damage;
    bool    active;
} Bullet;

typedef struct Pickup {
    Rectangle box;
    int       type;
    int       amount;
    bool      taken;
    float     bob;
} Pickup;

typedef struct Particle {
    Vector2 pos, vel;
    float   life, maxLife, size, drag;
    Color   color;
    bool    active;
} Particle;

typedef struct GameData {
    Scene scene;
    Scene retryScene;
    int   level;             
    int   coins;
    int   deaths;
    float levelTime;
    char  msg[128];
    float msgTimer;
    bool  quitRequested;

    /* per-level stats */
    int   kills;
    int   hits;
    float peakInfection;

    /* the loadout you walked out of the last level with. */
    bool  hasCarry;
    int   carryHealth;
    float carryInfection;
    int   carryInjectors;
    int   carryPistol;
    int   carryShells;
    bool  carryShotgun;
} GameData;

extern GameData game;
extern Player   player;

/* fx.c */
void  FxReset(void);
float FxTick(float rawDt);                 /* returns 0 during hitstop */
void  AddShake(float power);
void  AddHitstop(float seconds);
void  FlashScreen(Color c, float duration);
void  ApplyShake(Camera2D *cam);
void  SpawnParticles(Vector2 pos, int count, Color c,
                     float speed, float life, float size);
void  SpawnBlood(Vector2 pos, Vector2 dir, int count);
void  SpawnDust(Vector2 pos);
void  DrawParticles(void);
void  DrawVignette(unsigned char strength);
void  DrawFlash(void);
void  PushFloatingText(Vector2 worldPos, const char *text, Color c);
void  DrawFloatingText(Camera2D cam);

/* player.c */
Vector2  RectCenter(Rectangle r);
Vector2  Norm(Vector2 v);
float    Dist(Vector2 a, Vector2 b);
float    Approach(float value, float target, float rate);
int      BuildWallsFromMap(const char **rows, int rowCount,
                           Rectangle *walls, int maxWalls);

void     ResetPlayer(float x, float y);
void     PlacePlayer(float x, float y);     
void     SaveLoadout(void);
void     RestoreLoadout(void);

void     MoveBox(Rectangle *box, float dx, float dy, Rectangle *walls, int wallCount);
void     UpdatePlayerMovement(Rectangle *walls, int wallCount, float dt);
void     AimAtMouse(Camera2D cam);
void     DamagePlayer(int amount, int infectAmount, Vector2 fromPos);
void     AddCoins(int amount);
void     PlayerDied(Scene retryLevel);

void     AddInfection(float amount);
void     UpdateInfection(float ambientPerSecond, float dt);
void     UpdateInjector(float dt);

Camera2D MakeCamera(float zoom);
void     UpdateCameraFollow(Camera2D *cam, float dt, float leanAmount);

/* weapons.c*/
void     SwitchWeapon(int w);
void     CycleWeapon(void);
bool     FireWeapon(Bullet *bullets, Zombie *z, int zCount);
const char *WeaponName(int w);

void     SpawnBullet(Bullet *list, Vector2 pos, Vector2 dir, float speed, int dmg);
void     UpdateBullets(Bullet *list, Rectangle *walls, int wallCount, float dt);
void     DrawBullets(Bullet *list, Color c);
int      BulletsHitZombies(Bullet *list, Zombie *z, int count);

Rectangle MeleeHitbox(void);
int      MeleeHitZombies(Zombie *z, int count, int damage);

/*  enemy.c */
int      SpawnZombie(Zombie *list, int *count, float x, float y, int type);
void     UpdateZombies(Zombie *z, int count, Rectangle *walls, int wallCount,
                       float dt, float aggroRange);
void     DrawZombies(Zombie *z, int count);

/*  render.c  */
void     DrawFloor(int cols, int rows);
void     DrawWalls(Rectangle *walls, int count);
void     DrawPlayer(void);
void     DrawPickups(Pickup *p, int count, float dt);
void     DrawDarkness(Camera2D cam, Vector2 worldLight, float radius, unsigned char alpha);
void     DrawInfectionOverlay(void);
void     DrawHUD(const char *objective);
void     DrawMessage(void);
void     ShowMessage(const char *text);
void     DrawPrompt(const char *text);
void     DrawHoldBar(float progress, const char *label);
bool     ButtonUI(Rectangle r, const char *label);

/*  story.c */
void Story_Show(const char **lines, int lineCount,
                const char *buttonLabel, Scene nextScene);
void Story_Update(float dt);
void Story_Draw(void);
void Story_ShowIntro(void);          
void Story_ShowCard2(void);        
void Story_ShowCard3(void);         

void GoToScene(Scene s);

void Menu_Update(float dt);     void Menu_Draw(void);
void Results_Update(float dt);  void Results_Draw(void);
void GameOver_Update(float dt); void GameOver_Draw(void);
void Victory_Update(float dt);  void Victory_Draw(void);

void Level1_Init(void); void Level1_Update(float dt); void Level1_Draw(void);
void Level2_Init(void); void Level2_Update(float dt); void Level2_Draw(void);
void Level3_Init(void); void Level3_Update(float dt); void Level3_Draw(void);

#endif /* GAME_H */