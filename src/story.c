/* 
   Text types out line by line. NEXT / START button bottom-right.
   Pressing any key or clicking skips the typing and shows the
   whole card instantly - ALWAYS let people skip.

 */

#include "game.h"
#include <string.h>

#define MAX_STORY_LINES 12

static const char *lines[MAX_STORY_LINES];
static int   lineCount;
static char  buttonLabel[24];
static Scene nextScene;

static float typeTimer;      /* seconds since this card started */
static int   revealedLines;  /* how many lines are fully shown  */
static int   revealedChars;  /* chars shown on the current line */
static bool  finished;

#define CHARS_PER_SEC 42.0f
#define LINE_PAUSE     0.35f

void Story_Show(const char **newLines, int newCount,const char *label, Scene next)
{
    int i;
    if (newCount > MAX_STORY_LINES) newCount = MAX_STORY_LINES;
    for (i = 0; i < newCount; i++) lines[i] = newLines[i];
    lineCount = newCount;

    strncpy(buttonLabel, label, sizeof(buttonLabel) - 1);
    buttonLabel[sizeof(buttonLabel) - 1] = '\0';

    nextScene = next;
    typeTimer = 0.0f;
    revealedLines = 0;
    revealedChars = 0;
    finished = false;

    game.scene = SCENE_STORY;
}

// CARD 1 : before level 1 //
 
void Story_ShowIntro(void)
{
    static const char *intro[] = 
    {
        "It took nine days.",
        "",
        "Not months. Not years. Nine days.",
        "",
        "By the time command gave the order to seal the gates,",
        "the men giving it had already turned.",
        "",
        "You woke up in a supply room in Sector 7",
        "because nobody came back to check it.",
        "",
        "You are the last thing breathing in this base."
    };
    Story_Show(intro, 11, "START", SCENE_LEVEL1);
}

// CARD 2 : after level 1, before level 2 //


void Story_ShowCard2(void)
{
    static const char *card2[] = 
    {
        "The map isn't a map of the base.",
        "",
        "It's a route. Forty kilometres north,",
        "under the salt flats.",
        "",
        "BLACKWELL RESEARCH STATION.",
        "Sub-Level 4. Sealed from the inside.",
        "",
        "Somebody down there knew this was coming",
        "before it happened."
    };
    Story_Show(card2, 10, "NEXT", SCENE_LEVEL2);
}

//  CARD 3 : after level 2, before level 3 //

void Story_ShowCard3(void)
{
    static const char *card3[] = 
    {
        "The lift only goes one way now.",
        "",
        "Sub-Level 4.",
        "",
        "He is still down there.",
        "He has been watching you the whole way.",
        "",
        "There is a grid across his door",
        "and something behind it that used to be a man."
    };
    Story_Show(card3, 9, "NEXT", SCENE_LEVEL3);
}

static void RevealEverything(void)
{
    revealedLines = lineCount;
    revealedChars = 0;
    finished = true;
}

void Story_Update(float dt)
{
    int len;

    if (!finished) 
    {
        /* skip the animation on any input */

        if (GetKeyPressed() != 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            RevealEverything();
            return;
        }

        typeTimer += dt;
        if (revealedLines >= lineCount) { finished = true; return; }

        len = (int)strlen(lines[revealedLines]);
        if (len == 0) 
        
        {   /* blank line = a beat */
        
            if (typeTimer > LINE_PAUSE) 
            { revealedLines++; typeTimer = 0.0f; }
            return;
        }

        revealedChars = (int)(typeTimer * CHARS_PER_SEC);
        if (revealedChars >= len) 
        {
            if (typeTimer > (float)len / CHARS_PER_SEC + LINE_PAUSE) 
            {
                revealedLines++;
                revealedChars = 0;
                typeTimer = 0.0f;
                if (revealedLines >= lineCount) finished = true;
            }
        }
    } 
    
    else 

    {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
            GoToScene(nextScene);
    }
}

void Story_Draw(void)
{
    int i, y = 150;
    Rectangle btn = { SCREEN_W - 230.0f, SCREEN_H - 110.0f, 180.0f, 54.0f };

    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, (Color){ 8, 8, 10, 255 });

    /* thin frame so it reads as a deliberate screen, not a bug */

    DrawRectangleLines(60, 70, SCREEN_W - 120, SCREEN_H - 190, (Color){ 44, 44, 52, 255 });

    for (i = 0; i < lineCount && i <= revealedLines; i++) 
    {
        const char *text = lines[i];
        if (i == revealedLines && !finished) 
        {
            /* partially typed line + a blinking cursor */

            char buf[256];
            int n = revealedChars;
            int len = (int)strlen(text);
            if (n > len) n = len;
            if (n > (int)sizeof(buf) - 2) n = (int)sizeof(buf) - 2;
            memcpy(buf, text, n);
            buf[n] = '\0';
            DrawText(buf, 110, y, 26, (Color){ 214, 214, 222, 255 });
            if (((int)(GetTime() * 3.0)) % 2 == 0)
                DrawText("_", 110 + MeasureText(buf, 26) + 3, y, 26,(Color){ 214, 214, 222, 255 });
        }
        
        else 
        {
            DrawText(text, 110, y, 26, (Color){ 214, 214, 222, 255 });
        }
        y += 36;
    }

    if (finished) 
    {
        if (ButtonUI(btn, buttonLabel)) GoToScene(nextScene);
        DrawText("[ENTER]", (int)btn.x + 50, (int)btn.y + 60, 16, (Color){ 110, 110, 120, 255 });
    } 
    else 
    {
        DrawText("click or press any key to skip", 110, SCREEN_H - 96, 16,(Color){ 90, 90, 100, 255 });
    }
}