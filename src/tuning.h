* ============================================================
   tuning.h  -  EVERY NUMBER THAT CHANGES HOW THE GAME PLAYS.

   This file exists for one reason: when somebody says "make the
   zombies faster" or "what happens if the shotgun is weaker",
   you change ONE line here instead of hunting through 4,000
   lines of code.

   Nothing in here is logic. It is all just numbers with names.
   Change one, rebuild, play. That is the whole workflow.

   Owner: Person B, but anyone may change a number here. Say so
   in your commit message so the others know the balance moved.
   ============================================================ */
#ifndef TUNING_H
#define TUNING_H

/* ============================================================
   1. THE PLAYER
   ============================================================ */
#define PLAYER_SPEED            220.0f   /* world units per second      */
#define PLAYER_SPRINT_MULT      1.5f     /* how much faster Shift is    */
#define PLAYER_MAX_HEALTH       100
#define PLAYER_SIZE             26.0f    /* hitbox width and height     */
#define PLAYER_IFRAMES          0.75f    /* seconds of invincibility
                                            after taking a hit          */
#define PLAYER_KNOCKBACK        320.0f   /* how hard a hit shoves you   */
/* ============================================================
   2. THE INFECTION METER    (the game's signature system)

   It starts at 0 and only rises when something hits you.
   At 100 you turn, whatever your health says.
   ============================================================ */
#define INFECT_WEAK             2        /* per hit: shambler, runner,
                                            crawler                     */
#define INFECT_STRONG           5        /* per hit: soldier, ARC       */
#define INJECT_HOLD_TIME        1.5f     /* seconds of holding E,
                                            standing perfectly still    */
#define INJECT_CLEARS           25.0f    /* infection removed per use   */

/* what infection does to you, and when */
#define INFECT_VISUAL_START     15.0f    /* colour starts draining      */
#define INFECT_HEAVY_START      35.0f    /* slowdown + screen pulse     */
#define INFECT_SLOW_AMOUNT      0.20f    /* 20% slower at maximum       */

/* ============================================================
   3. WEAPONS
   ============================================================ */
   #define KNIFE_DAMAGE            34
#define KNIFE_COOLDOWN          0.42f
#define KNIFE_REACH             48.0f    /* size of the swing hitbox    */

#define PISTOL_DAMAGE           26
#define PISTOL_COOLDOWN         0.26f    /* seconds between shots       */
#define PISTOL_SPEED            780.0f
#define PISTOL_START_AMMO       24

#define SHOTGUN_PELLETS         7        /* 7 x 15 = 105 point blank    */
#define SHOTGUN_PELLET_DAMAGE   15
#define SHOTGUN_COOLDOWN        0.80f
#define SHOTGUN_SPREAD          0.16f    /* radians of scatter          */
#define SHOTGUN_START_AMMO      10

#define STUN_DAMAGE_BONUS_NUM   3        /* a stunned enemy takes       */
#define STUN_DAMAGE_BONUS_DEN   2        /* 3/2 = 150% damage           */
