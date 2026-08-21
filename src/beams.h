/* beams.h  -  the level 3 security grid.

   A beam is two points and a timer. Three kinds:
     sweep   a bar sliding back and forth  -> go around it
     rotate  an arm turning about a point  -> cross when it points away
     blink   a fixed gate pulsing on/off   -> wait for your window

   You cannot shoot them. That is the point: Voss does not fight,
   his machines do.*/
#ifndef BEAMS_H
#define BEAMS_H

#include "game.h"

void  BuildGrid(void);              /* set up all 8 beams          */
void  UpdateBeams(float dt);        /* advance every beam          */
void  DrawBeams(void);              /* draw them                   */
bool  BeamHitsPlayer(int index);    /* is beam[i] touching you?    */
int   BeamCount(void);              /* how many there are          */
bool  BeamInVossRoom(int index);    /* room beams vs corridor ones */
void  SetGridLive(bool live);       /* ARC dying kills the grid    */
bool  GridIsLive(void);

#endif /* BEAMS_H */