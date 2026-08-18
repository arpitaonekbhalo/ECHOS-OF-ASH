/* 
   tuning.h  -  EVERY NUMBER THAT CHANGES HOW THE GAME PLAYS.

   This file exists for one reason: when somebody says "make the
   zombies faster" or "what happens if the shotgun is weaker",
   you change ONE line here instead of hunting through 4,000
   lines of code.
 */

#ifndef TUNING_H
#define TUNING_H

  // 1. THE PLAYER//

#define PLAYER_SPEED            220.0f   /* world units per second */
#define PLAYER_SPRINT_MULT      1.5f     /* how much faster Shift is */
#define PLAYER_MAX_HEALTH       100
#define PLAYER_SIZE             26.0f    /* hitbox width and height */
#define PLAYER_IFRAMES          0.75f    /* seconds of invincibility after taking a hit  */
#define PLAYER_KNOCKBACK        320.0f   /* how hard a hit shoves you   */

/* 
   2. THE INFECTION METER 

   It starts at 0 and only rises when something hits you.
   At 100 you turn, whatever your health says.  */

#define INFECT_WEAK             2        /* per hit: shambler, runner,crawler */
#define INFECT_STRONG           5        /* per hit: soldier, ARC */
#define INJECT_HOLD_TIME        1.5f     /* seconds of holding E, standing perfectly still */
#define INJECT_CLEARS           25.0f    /* infection removed per use   */

/* what infection does to you, and when */
#define INFECT_VISUAL_START     15.0f    /* colour starts draining      */
#define INFECT_HEAVY_START      35.0f    /* slowdown + screen pulse     */
#define INFECT_SLOW_AMOUNT      0.20f    /* 20% slower at maximum       */

  // 3. WEAPONS //


#define KNIFE_DAMAGE            34
#define KNIFE_COOLDOWN          0.42f
#define KNIFE_REACH             48.0f    /* size of the swing hitbox */

#define PISTOL_DAMAGE           26
#define PISTOL_COOLDOWN         0.26f    /* seconds between shots */
#define PISTOL_SPEED            780.0f
#define PISTOL_START_AMMO       24

#define SHOTGUN_PELLETS         7        /* 7 x 15 = 105 point blank */
#define SHOTGUN_PELLET_DAMAGE   15
#define SHOTGUN_COOLDOWN        0.80f
#define SHOTGUN_SPREAD          0.16f    /* radians of scatter */
#define SHOTGUN_START_AMMO      10

#define STUN_DAMAGE_BONUS_NUM   3        /* a stunned enemy takes  */
#define STUN_DAMAGE_BONUS_DEN   2        /* 3/2 = 150% damage */

/* 
   4. ZOMBIES

   Change any of these and the difficulty of level 1 moves.
   The soldier's 130 health is the number that makes the shotgun
   worth finding: 5 pistol shots each, vs 2 shotgun blasts.
   */


#define SHAMBLER_SPEED          86.0f
#define SHAMBLER_HEALTH         60
#define SHAMBLER_DAMAGE         12

#define RUNNER_SPEED            138.0f
#define RUNNER_HEALTH           40
#define RUNNER_DAMAGE           9

#define CRAWLER_SPEED           58.0f
#define CRAWLER_HEALTH          35
#define CRAWLER_DAMAGE          8

#define SOLDIER_SPEED           78.0f
#define SOLDIER_HEALTH          130
#define SOLDIER_DAMAGE          16

/* how the AI behaves */
#define ZOMBIE_AGGRO_RANGE      330.0f   /* how far they notice you  */
#define ZOMBIE_LUNGE_RANGE      96.0f    /* how close before they wind up */
#define ZOMBIE_WINDUP_TIME      0.45f    /* the DODGE NOW window  */
#define ZOMBIE_RUNNER_WINDUP    0.32f    /* runners telegraph faster */
#define ZOMBIE_LUNGE_TIME       0.30f
#define ZOMBIE_LUNGE_SPEED      4.4f     /* multiplier on normal speed  */
#define ZOMBIE_STUN_TIME        0.75f    /* your punish window          */


  // 5. ARC  -  SPECIMEN 09    (the level 3 boss)//


#define ARC_HEALTH              360      /* 4 point-blank shotgun blasts */
#define ARC_SPEED               96.0f
#define ARC_TOUCH_DAMAGE        10
#define ARC_BOLT_DAMAGE         11
#define ARC_BOLT_SPEED          430.0f
#define ARC_PULSE_DAMAGE        14
#define ARC_PULSE_RANGE         230.0f   /* how far the shockwave goes  */
#define ARC_BOLT_WINDUP         0.6f
#define ARC_PULSE_WINDUP        0.9f
#define ARC_DRAINED_TIME        1.2f     /* punish window after a pulse */

/*
   6. DR. VOSS
   He is not a fighter. One hit of anything ends him.
 */

#define VOSS_WALK_SPEED         70.0f    /* how fast he comes to meet you */
#define VOSS_BACKAWAY_SPEED     62.0f
#define VOSS_BACKAWAY_RANGE     170.0f   /* he retreats inside this  */
#define VOSS_SHOT_DAMAGE        8
#define VOSS_SHOT_SPEED         370.0f
#define VOSS_FIRE_RATE          1.5f     /* seconds between his shots */


  // 7. THE LASER GRID  (level 3)//

#define BEAM_DAMAGE             12       /* without plating    */
#define BEAM_DAMAGE_PLATED      6        /* with plating bought   */
#define BEAM_SAMPLE_STEP        6.0f     /* collision precision in units */


  // 8. COINS  -  earning and spending //


#define COIN_PICKUP_VALUE       15
#define COIN_PER_KILL           10
#define COIN_ARC_KILL           80
#define COIN_VOSS_KILL          100
#define COIN_SHOTGUN_FOUND      30
#define COIN_ROOM_CLEARED       40
#define COIN_MAP_TAKEN          25
#define COIN_LOCK_OPENED        60
#define COIN_TUBE_TAKEN         120
#define COIN_LEVEL_FINISHED     100
#define COIN_MACHINE            200
#define COIN_DEATH_PENALTY      25

#define BONUS_FAST_TIME         180.0f   /* finish under this for*/
#define BONUS_FAST_COINS        60
#define BONUS_CLEAN_INFECTION   20.0f    /* stay under this for*/
#define BONUS_CLEAN_COINS       50

/* the supply cache in level 3 */


#define SHOP_MEDKIT_COST        30
#define SHOP_MEDKIT_HEAL        40
#define SHOP_INJECTOR_COST      35
#define SHOP_AMMO_COST          25
#define SHOP_PLATING_COST       60


  // 9. LOOK AND FEEL//

#define LIGHT_RADIUS            560.0f   /* how far you can see  */
#define DARKNESS_ALPHA          205      /* 0 = daylight, 255 = pitch   */
#define CAMERA_LEAN             55.0f    /* how far the view leads your aim */
#define CAMERA_FOLLOW_SPEED     7.0f

#define HITSTOP_ON_HIT          0.03f    /* the freeze that sells a hit  */
#define HITSTOP_ON_KILL         0.07f
#define SHAKE_ON_HURT           9.0f
#define SHAKE_ON_KILL           6.0f

/* 
   10. RETRY SAFETY NET
   Floors so a retry is never unwinnable.  */


#define RETRY_MIN_HEALTH        45
#define RETRY_MIN_PISTOL_AMMO   18
#define RETRY_MIN_SHELLS        6

#endif /* TUNING_H */