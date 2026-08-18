#include "game.h"
#include <math.h>

int SpawnZombie(Zombie *list, int *count, float x, float y, int type)
{
    Zombie *z;
    if (*count >= MAX_ZOMBIES) return -1;
    z = &list[*count];

    z->box.x = x; z->box.y = y;
    z->box.width = 28.0f; z->box.height = 28.0f;
    z->type = type;
    z->alive = true;
    z->dormant = false;
    z->state = Z_IDLE;
    z->stateTimer = (float)GetRandomValue(50, 250) / 100.0f;
    z->hitTimer = 0.0f;
    z->cooldown = 0.0f;
    z->knockback.x = 0.0f; z->knockback.y = 0.0f;
    z->wanderDir.x = 0.0f; z->wanderDir.y = 1.0f;
    z->lungeDir.x = 0.0f;  z->lungeDir.y = 1.0f;

    switch (type) {
        case ZT_RUNNER:
            z->speed = RUNNER_SPEED;   z->maxHealth = RUNNER_HEALTH;
            z->damage = RUNNER_DAMAGE; z->infect = INFECT_WEAK;
            z->box.width = 24.0f; z->box.height = 24.0f;
            break;
        case ZT_CRAWLER:
            z->speed = CRAWLER_SPEED;   z->maxHealth = CRAWLER_HEALTH;
            z->damage = CRAWLER_DAMAGE; z->infect = INFECT_WEAK;
            z->box.width = 32.0f; z->box.height = 20.0f;
            break;
        case ZT_SOLDIER:
            /* 130 HP = 5 pistol shots, or 2 shotgun blasts up close.
               That gap is the whole reason the armoury exists. */
            z->speed = SOLDIER_SPEED;   z->maxHealth = SOLDIER_HEALTH;
            z->damage = SOLDIER_DAMAGE; z->infect = INFECT_STRONG;
            z->box.width = 32.0f; z->box.height = 32.0f;
            break;
        default:
            z->speed = SHAMBLER_SPEED;   z->maxHealth = SHAMBLER_HEALTH;
            z->damage = SHAMBLER_DAMAGE; z->infect = INFECT_WEAK;
            break;
    }
    z->health = z->maxHealth;

    (*count)++;
    return *count - 1;
}

void UpdateZombies(Zombie *z, int count, Rectangle *walls, int wallCount,
                   float dt, float aggroRange)
{
    int i;
    Vector2 pc = RectCenter(player.box);

    for (i = 0; i < count; i++) {
        Zombie *m = &z[i];
        Vector2 zc, dir;
        float d;

        if (!m->alive) continue;
        if (m->dormant) continue;      /* asleep until something wakes them */

        if (m->hitTimer > 0.0f) m->hitTimer -= dt;
        if (m->cooldown > 0.0f) m->cooldown -= dt;
        m->stateTimer -= dt;

        m->knockback.x = Approach(m->knockback.x, 0.0f, 800.0f * dt);
        m->knockback.y = Approach(m->knockback.y, 0.0f, 800.0f * dt);
        if (m->knockback.x != 0.0f || m->knockback.y != 0.0f)
            MoveBox(&m->box, m->knockback.x * dt, m->knockback.y * dt,
                    walls, wallCount);

        zc = RectCenter(m->box);
        d = Dist(zc, pc);
        dir = Norm((Vector2){ pc.x - zc.x, pc.y - zc.y });

        switch (m->state) {
            case Z_IDLE:
                if (d < aggroRange) m->state = Z_CHASE;
                else if (m->stateTimer <= 0.0f) {
                    float a = (float)GetRandomValue(0, 628) / 100.0f;
                    m->wanderDir.x = cosf(a); m->wanderDir.y = sinf(a);
                    m->state = Z_WANDER;
                    m->stateTimer = (float)GetRandomValue(100, 260) / 100.0f;
                }
                break;

            case Z_WANDER:
                MoveBox(&m->box, m->wanderDir.x * m->speed * 0.3f * dt,
                                 m->wanderDir.y * m->speed * 0.3f * dt,
                        walls, wallCount);
                if (d < aggroRange) m->state = Z_CHASE;
                else if (m->stateTimer <= 0.0f) {
                    m->state = Z_IDLE;
                    m->stateTimer = (float)GetRandomValue(60, 200) / 100.0f;
                }
                break;

            case Z_CHASE:
                MoveBox(&m->box, dir.x * m->speed * dt, dir.y * m->speed * dt,
                        walls, wallCount);
                if (m->type == ZT_CRAWLER) {
                    if (CheckCollisionRecs(m->box, player.box) && m->cooldown <= 0.0f) {
                        DamagePlayer(m->damage, m->infect, zc);
                        m->cooldown = 1.1f;
                    }
                } else if (d < ZOMBIE_LUNGE_RANGE && m->cooldown <= 0.0f) {
                    m->state = Z_WINDUP;
                    m->stateTimer = (m->type == ZT_RUNNER) ? ZOMBIE_RUNNER_WINDUP
                                                          : ZOMBIE_WINDUP_TIME;
                    m->lungeDir = dir;
                }
                if (d > aggroRange * 2.4f) { m->state = Z_WANDER; m->stateTimer = 1.5f; }
                break;

            case Z_WINDUP:
                m->lungeDir = dir;
                if (m->stateTimer <= 0.0f) { m->state = Z_LUNGE; m->stateTimer = ZOMBIE_LUNGE_TIME; }
                break;

            case Z_LUNGE:
                MoveBox(&m->box,
                        m->lungeDir.x * m->speed * ZOMBIE_LUNGE_SPEED * dt,
                        m->lungeDir.y * m->speed * ZOMBIE_LUNGE_SPEED * dt,
                        walls, wallCount);
                if (CheckCollisionRecs(m->box, player.box)) {
                    DamagePlayer(m->damage, m->infect, zc);
                    m->state = Z_CHASE;
                    m->cooldown = 1.5f;
                } else if (m->stateTimer <= 0.0f) {
                    m->state = Z_STUN;
                    m->stateTimer = ZOMBIE_STUN_TIME;
                    m->cooldown = 1.4f;
                }
                break;

            case Z_STUN:
                if (m->stateTimer <= 0.0f) m->state = Z_CHASE;
                break;

            default: break;
        }
    }
}

void DrawZombies(Zombie *z, int count)
{
    int i;
    for (i = 0; i < count; i++) {
        Color body;
        Vector2 c;

        if (!z[i].alive) {
            DrawRectangleRec(z[i].box, (Color){ 52, 26, 28, 150 });
            continue;
        }

        c = RectCenter(z[i].box);

        switch (z[i].state) {
            case Z_WINDUP: body = (Color){ 245, 170, 60, 255 };  break;  /* DODGE  */
            case Z_LUNGE:  body = (Color){ 240, 90, 70, 255 };   break;
            case Z_STUN:   body = (Color){ 135, 135, 180, 255 }; break;  /* PUNISH */
            case Z_CHASE:  body = (Color){ 116, 156, 74, 255 };  break;
            default:       body = (Color){ 74, 94, 58, 255 };    break;
        }
        if (z[i].type == ZT_SOLDIER && z[i].state == Z_CHASE)
            body = (Color){ 110, 116, 88, 255 };
        if (z[i].hitTimer > 0.0f) body = (Color){ 255, 245, 245, 255 };

        DrawEllipse((int)c.x, (int)(z[i].box.y + z[i].box.height - 2),
                    z[i].box.width * 0.52f, 6.0f, (Color){ 0, 0, 0, 90 });
        DrawRectangleRec(z[i].box, body);
        DrawRectangleLinesEx(z[i].box, 2.0f, (Color){ 24, 38, 20, 255 });
        /* this one is tougher */
        if (z[i].type == ZT_SOLDIER && z[i].hitTimer <= 0.0f)
            DrawRectangle((int)(z[i].box.x + 3),
                          (int)(z[i].box.y + z[i].box.height * 0.42f),
                          (int)(z[i].box.width - 6),
                          (int)(z[i].box.height * 0.30f),
                          (Color){ 28, 30, 24, 190 });
        DrawCircle((int)(z[i].box.x + 8), (int)(z[i].box.y + 9), 2.5f,
                   (Color){ 228, 62, 62, 255 });
        DrawCircle((int)(z[i].box.x + z[i].box.width - 8), (int)(z[i].box.y + 9), 2.5f,
                   (Color){ 228, 62, 62, 255 });

        if (z[i].state == Z_WINDUP)
            DrawLineEx(c, (Vector2){ c.x + z[i].lungeDir.x * 150.0f,
                                     c.y + z[i].lungeDir.y * 150.0f },
                       2.0f, (Color){ 245, 170, 60, 140 });
        if (z[i].state == Z_STUN)
            DrawText("!", (int)c.x - 3, (int)z[i].box.y - 22, 22,
                     (Color){ 185, 185, 245, 255 });

        if (z[i].health < z[i].maxHealth) {
            float w = z[i].box.width;
            DrawRectangle((int)z[i].box.x, (int)z[i].box.y - 8, (int)w, 3,
                          (Color){ 0, 0, 0, 160 });
            DrawRectangle((int)z[i].box.x, (int)z[i].box.y - 8,
                          (int)(w * z[i].health / z[i].maxHealth), 3,
                          (Color){ 212, 70, 70, 255 });
        }
    }
}