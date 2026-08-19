/* 

   Firing, bullet travel, bullet-vs-zombie, and the melee arc.
   Weapon feel lives here: cooldowns, spread, damage, knockback.
   All the numbers come from tuning.h.
 */

#include "game.h"
#include <math.h>


const char *WeaponName(int w)
{
    switch (w)
     {
        case WEAP_KNIFE:   return "KNIFE";
        case WEAP_PISTOL:  return "PISTOL";
        case WEAP_SHOTGUN: return "SHOTGUN";
        default:           return "?";
    }
}

void SwitchWeapon(int w)
{
    if (w < 0 || w >= WEAP_COUNT) return;
    if (!player.hasWeapon[w]) return;
    player.weapon = w;
    player.fireCooldown = 0.15f;
}

void CycleWeapon(void)
{
    int i, w = player.weapon;
    for (i = 0; i < WEAP_COUNT; i++) 
    {
        w = (w + 1) % WEAP_COUNT;
        if (player.hasWeapon[w]) { SwitchWeapon(w); return; }
    }
}

/* returns true if something actually fired */


bool FireWeapon(Bullet *bullets, Zombie *z, int zCount)
{
    Vector2 c = RectCenter(player.box);

    if (player.fireCooldown > 0.0f) return false;

    switch (player.weapon)
     {
        case WEAP_KNIFE:
            player.attackTimer = 0.18f;
            player.fireCooldown = KNIFE_COOLDOWN;
            MeleeHitZombies(z, zCount, KNIFE_DAMAGE);
            return true;

        case WEAP_PISTOL:
            if (player.ammo[WEAP_PISTOL] <= 0) 
            {
                ShowMessage("click - out of pistol ammo");
                player.fireCooldown = 0.4f;
                return false;
            }
            player.ammo[WEAP_PISTOL]--;
            player.fireCooldown = PISTOL_COOLDOWN;
            SpawnBullet(bullets, c, player.aim, PISTOL_SPEED, PISTOL_DAMAGE);
            SpawnParticles((Vector2){ c.x + player.aim.x * 24.0f, c.y + player.aim.y * 24.0f },4, (Color){ 255, 220, 140, 255 }, 90.0f, 0.12f, 3.0f);
            AddShake(2.5f);
            return true;

        case WEAP_SHOTGUN:
         {
            int i;
            float base;
            if (player.ammo[WEAP_SHOTGUN] <= 0)
             {
                ShowMessage("click - out of shells");
                player.fireCooldown = 0.4f;
                return false;
            }
            player.ammo[WEAP_SHOTGUN]--;
            player.fireCooldown = SHOTGUN_COOLDOWN;
            base = atan2f(player.aim.y, player.aim.x);
            for (i = 0; i < SHOTGUN_PELLETS; i++) 
            {
                float a = base + (float)GetRandomValue(-16, 16) / 100.0f;  /* SHOTGUN_SPREAD */
                Vector2 d; d.x = cosf(a); d.y = sinf(a);
                SpawnBullet(bullets, c, d,680.0f + (float)GetRandomValue(-60, 60), SHOTGUN_PELLET_DAMAGE);
            }
            SpawnParticles((Vector2){ c.x + player.aim.x * 26.0f, c.y + player.aim.y * 26.0f },12, (Color){ 255, 210, 130, 255 }, 190.0f, 0.18f, 4.0f);
            AddShake(8.0f);
            AddHitstop(0.04f);
            player.knockback.x = -player.aim.x * 150.0f;
            player.knockback.y = -player.aim.y * 150.0f;
            return true;
        }
        default: return false;
    }
}

/*  bullets  */


void SpawnBullet(Bullet *list, Vector2 pos, Vector2 dir, float speed, int dmg)
{
    int i;
    for (i = 0; i < MAX_BULLETS; i++) 
    {
        if (!list[i].active)
         {
            list[i].active = true;
            list[i].pos = pos;
            list[i].vel.x = dir.x * speed;
            list[i].vel.y = dir.y * speed;
            list[i].life = 1.4f;
            list[i].damage = dmg;
            return;
        }
    }
}

void UpdateBullets(Bullet *list, Rectangle *walls, int wallCount, float dt)
{
    int i, w;
    for (i = 0; i < MAX_BULLETS; i++) 
    {
        if (!list[i].active) continue;
        list[i].pos.x += list[i].vel.x * dt;
        list[i].pos.y += list[i].vel.y * dt;
        list[i].life -= dt;
        if (list[i].life <= 0.0f) { list[i].active = false; continue; }
        for (w = 0; w < wallCount; w++) {
            if (CheckCollisionCircleRec(list[i].pos, 3.0f, walls[w])) {
                list[i].active = false;
                SpawnParticles(list[i].pos, 3, (Color){ 220, 200, 140, 255 },110.0f, 0.2f, 2.0f);
                break;
            }
        }
    }
}

void DrawBullets(Bullet *list, Color c)
{
    int i;
    for (i = 0; i < MAX_BULLETS; i++)
     {
        if (!list[i].active) continue;
        DrawLineEx(list[i].pos,(Vector2){ list[i].pos.x - list[i].vel.x * 0.018f,list[i].pos.y - list[i].vel.y * 0.018f },3.0f, c);
    }
}

int BulletsHitZombies(Bullet *list, Zombie *z, int count)
{
    int i, j, kills = 0;
    for (i = 0; i < MAX_BULLETS; i++) {
        if (!list[i].active) continue;
        for (j = 0; j < count; j++) {
            if (!z[j].alive) continue;
            if (CheckCollisionCircleRec(list[i].pos, 3.0f, z[j].box)) {
                Vector2 d = Norm(list[i].vel);
                int dmg = list[i].damage;
                /* punish window: a stunned enemy takes 150% */
                if (z[j].state == Z_STUN)
                    dmg = (dmg * STUN_DAMAGE_BONUS_NUM) / STUN_DAMAGE_BONUS_DEN;

                z[j].health -= dmg;
                z[j].hitTimer = 0.12f;
                z[j].knockback.x = d.x * 190.0f;
                z[j].knockback.y = d.y * 190.0f;
                if (z[j].state == Z_IDLE || z[j].state == Z_WANDER)
                    z[j].state = Z_CHASE;
                list[i].active = false;

                SpawnBlood(RectCenter(z[j].box), d, 7);
                AddHitstop(HITSTOP_ON_HIT);
                AddShake(2.5f);

                if (z[j].health <= 0) {
                    z[j].alive = false;
                    kills++;
                    SpawnBlood(RectCenter(z[j].box), d, 22);
                    AddShake(SHAKE_ON_KILL);
                    AddHitstop(HITSTOP_ON_KILL);
                }
                break;
            }
        }
    }
    return kills;
}

/* melee  */


Rectangle MeleeHitbox(void)
{
    Vector2 c = RectCenter(player.box);
    Rectangle r;
    r.width = KNIFE_REACH; r.height = KNIFE_REACH;
    r.x = c.x + player.aim.x * 32.0f - r.width * 0.5f;
    r.y = c.y + player.aim.y * 32.0f - r.height * 0.5f;
    return r;
}

int MeleeHitZombies(Zombie *z, int count, int damage)
{
    Rectangle hit = MeleeHitbox();
    int i, kills = 0, connected = 0;

    for (i = 0; i < count; i++) {
        if (!z[i].alive) continue;
        if (!CheckCollisionRecs(hit, z[i].box)) continue;
        {
            int dmg = damage;
            if (z[i].state == Z_STUN)
                dmg = (dmg * STUN_DAMAGE_BONUS_NUM) / STUN_DAMAGE_BONUS_DEN;

            z[i].health -= dmg;
            z[i].hitTimer = 0.12f;
            z[i].knockback.x = player.aim.x * 300.0f;
            z[i].knockback.y = player.aim.y * 300.0f;
            if (z[i].state == Z_IDLE || z[i].state == Z_WANDER) z[i].state = Z_CHASE;
            connected = 1;

            SpawnBlood(RectCenter(z[i].box), player.aim, 9);
            if (z[i].health <= 0) {
                z[i].alive = false;
                kills++;
                SpawnBlood(RectCenter(z[i].box), player.aim, 24);
            }
        }
    }

    if (connected)
     {
        AddHitstop(kills > 0 ? 0.09f : 0.05f);
        AddShake(kills > 0 ? 8.0f : 4.0f);
    }
    return kills;
}