#include "zombie.h"
#include <math.h>

void Zombie_Spawn(Zombie z[], int *count, Vector3 pos, float speed)
{
    if (*count >= MAX_ZOMBIES) return;
    z[*count].position    = pos;
    z[*count].radius      = 0.45f;
    z[*count].height      = 1.8f;
    z[*count].speed       = speed;
    z[*count].alive       = true;
    z[*count].health      = 30;
    z[*count].hitCooldown = 0.0f;
    (*count)++;
}

void Zombie_UpdateAll(Zombie z[], int count, Player *player, Vector3 roomMin, Vector3 roomMax, float dt)
{
    for (int i = 0; i < count; i++) {
        if (!z[i].alive) continue;
        if (z[i].hitCooldown > 0.0f) z[i].hitCooldown -= dt;

        float dx = player->position.x - z[i].position.x;
        float dz = player->position.z - z[i].position.z;
        float dist = sqrtf(dx * dx + dz * dz);

        if (dist > 0.05f) {
            dx /= dist;
            dz /= dist;
            z[i].position.x += dx * z[i].speed * dt;
            z[i].position.z += dz * z[i].speed * dt;
        }

        if (z[i].position.x - z[i].radius < roomMin.x) z[i].position.x = roomMin.x + z[i].radius;
        if (z[i].position.z - z[i].radius < roomMin.z) z[i].position.z = roomMin.z + z[i].radius;
        if (z[i].position.x + z[i].radius > roomMax.x) z[i].position.x = roomMax.x - z[i].radius;
        if (z[i].position.z + z[i].radius > roomMax.z) z[i].position.z = roomMax.z - z[i].radius;

        if (dist < (player->radius + z[i].radius) && z[i].hitCooldown <= 0.0f) {
            player->health -= 10;
            z[i].hitCooldown = 1.0f;
        }
    }
}

void Zombie_DrawAll(Zombie z[], int count)
{
    for (int i = 0; i < count; i++) {
        if (!z[i].alive) continue;
        Vector3 bottom = { z[i].position.x, z[i].position.y, z[i].position.z };
        Vector3 top    = { z[i].position.x, z[i].position.y + z[i].height - z[i].radius * 2, z[i].position.z };
        DrawCapsule(bottom, top, z[i].radius, 12, 8, MAROON);
        DrawCapsuleWires(bottom, top, z[i].radius, 12, 8, BLACK);
    }
}