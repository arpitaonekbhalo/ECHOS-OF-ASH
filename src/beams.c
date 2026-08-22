/* beams.c  -  the level 3 security grid.
  Interface: beams.h */
#include "game.h"
#include "beams.h"
#include <math.h>
#include <string.h>

enum { BEAM_SWEEP, BEAM_ROTATE, BEAM_BLINK };

#define MAX_BEAMS 12

typedef struct Beam {
    int   kind;
    /* sweep: the bar runs from spanA to spanB on one axis, and its
       position slides between 'from' and 'to' on the other */
    float spanA, spanB, from, to, speed, t;
    /* rotate */
    float cx, cy, len, angle, spin;
    /* blink */
    float at, period, duty, phase;
    bool  inRoom;      /* true = one of the two in Voss's room */
} Beam;

static Beam beams[MAX_BEAMS];
static int  beamCount;
static bool gridLive;

static void AddSweep(float spanA, float spanB, float from, float to, float speed)
{
    Beam *b;
    if (beamCount >= MAX_BEAMS) return;
    b = &beams[beamCount++];
    memset(b, 0, sizeof(Beam));
    b->kind = BEAM_SWEEP;
    b->spanA = spanA; b->spanB = spanB;
    b->from = from;   b->to = to;
    b->speed = speed; b->t = 0.0f;
}

static void AddRotate(float cx, float cy, float len, float spin, float start, bool inRoom)
{
    Beam *b;
    if (beamCount >= MAX_BEAMS) return;
    b = &beams[beamCount++];
    memset(b, 0, sizeof(Beam));
    b->kind = BEAM_ROTATE;
    b->cx = cx; b->cy = cy; b->len = len;
    b->spin = spin; b->angle = start; b->inRoom = inRoom;
}

static void AddBlink(float spanA, float spanB, float at,
                     float period, float duty, float phase)
{
    Beam *b;
    if (beamCount >= MAX_BEAMS) return;
    b = &beams[beamCount++];
    memset(b, 0, sizeof(Beam));
    b->kind = BEAM_BLINK;
    b->spanA = spanA; b->spanB = spanB; b->at = at;
    b->period = period; b->duty = duty; b->phase = phase;
}

void BuildGrid(void)
{
    beamCount = 0;

    /* section 1: two sliding bars, offset so you weave between them */
    AddSweep(210.0f, 470.0f, 420.0f, 720.0f, 1.50f);
    AddSweep(500.0f, 790.0f, 700.0f, 420.0f, 1.20f);

    /* section 2: one rotating arm in the middle of the room */
    AddRotate(896.0f, 496.0f, 250.0f, 1.15f, 0.0f, false);

    /* section 3: three gates pulsing on staggered phases */
    AddBlink(210.0f, 790.0f, 1168.0f, 2.6f, 0.55f, 0.0f);
    AddBlink(210.0f, 790.0f, 1264.0f, 2.6f, 0.55f, 0.9f);
    AddBlink(210.0f, 790.0f, 1360.0f, 2.6f, 0.55f, 1.8f);

    /* Voss's room: two slow arms. Deliberately gentler than the
       antechamber, because there is a fight happening in here too. */
    AddRotate(1730.0f, 520.0f, 230.0f,  0.55f, 0.0f, true);
    AddRotate(1980.0f, 660.0f, 230.0f, -0.45f, 1.8f, true);

    gridLive = true;
}

/* where a beam's two ends are right now */
static void BeamLine(const Beam *b, Vector2 *p1, Vector2 *p2)
{
    if (b->kind == BEAM_SWEEP) {
        /* 0.5 - 0.5*cos gives a smooth ease-out at each end of the slide */
        float k = 0.5f - 0.5f * cosf(b->t);
        float x = b->from + (b->to - b->from) * k;
        p1->x = x; p1->y = b->spanA;
        p2->x = x; p2->y = b->spanB;
    } else if (b->kind == BEAM_ROTATE) {
        p1->x = b->cx; p1->y = b->cy;
        p2->x = b->cx + cosf(b->angle) * b->len;
        p2->y = b->cy + sinf(b->angle) * b->len;
    } else {
        p1->x = b->spanA; p1->y = b->at;
        p2->x = b->spanB; p2->y = b->at;
    }
}

static bool BeamOn(const Beam *b)
{
    float cycle;
    if (b->kind != BEAM_BLINK) return true;
    cycle = fmodf((float)GetTime() + b->phase, b->period) / b->period;
    return cycle < b->duty;
}

void UpdateBeams(float dt)
{
    int i;
    for (i = 0; i < beamCount; i++) {
        if (beams[i].kind == BEAM_SWEEP)  beams[i].t     += dt * beams[i].speed;
        if (beams[i].kind == BEAM_ROTATE) beams[i].angle += dt * beams[i].spin;
    }
}

/* Sampled line-versus-rectangle. We walk along the beam in 6-unit steps
   and ask if each point is inside the player's box. The player is 26
   units wide, so nothing can slip between two samples. */
static bool BeamTouches(const Beam *b)
{
    Vector2 p1, p2;
    float len;
    int steps, i;

    if (!BeamOn(b)) return false;
    BeamLine(b, &p1, &p2);
    len = Dist(p1, p2);
    steps = (int)(len / BEAM_SAMPLE_STEP);
    if (steps < 2) steps = 2;

    for (i = 0; i <= steps; i++) {
        float t = (float)i / (float)steps;
        Vector2 p;
        p.x = p1.x + (p2.x - p1.x) * t;
        p.y = p1.y + (p2.y - p1.y) * t;
        if (CheckCollisionPointRec(p, player.box)) return true;
    }
    return false;
}

/* ---- the small public surface the level talks to ---- */
int  BeamCount(void)              { return beamCount; }
bool BeamInVossRoom(int i)        { return beams[i].inRoom; }
void SetGridLive(bool live)       { gridLive = live; }
bool GridIsLive(void)             { return gridLive; }
bool BeamHitsPlayer(int i)        { return BeamTouches(&beams[i]); }

void DrawBeams(void)
{
    int i;
    for (i = 0; i < beamCount; i++) {
        Vector2 p1, p2;
        BeamLine(&beams[i], &p1, &p2);

        if (!gridLive) {                     /* dead rails */
            DrawLineEx(p1, p2, 2.0f, (Color){ 70, 70, 80, 90 });
            continue;
        }
        if (!BeamOn(&beams[i])) {            /* about to come back on */
            DrawLineEx(p1, p2, 2.0f, (Color){ 220, 90, 90, 56 });
            continue;
        }
        {
            float pulse = 0.75f + 0.25f * sinf((float)GetTime() * 11.0f);
            DrawLineEx(p1, p2, 11.0f,
                       (Color){ 255, 70, 60, (unsigned char)(56 * pulse) });
            DrawLineEx(p1, p2, 3.0f,
                       (Color){ 255, 150, 120, (unsigned char)(230 * pulse) });
            DrawCircleV(p1, 4.0f, (Color){ 255, 200, 170, 230 });
        }
    }
}