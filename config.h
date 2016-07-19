 /*
 *   /\___/\
 *  ( o   o )  Made by cat...
 *  (  =^=  )
 *  (        )            ... for cat!
 *  (         )
 *  (          ))))))________________ Cute And Tiny Window Manager
 *  ______________________________________________________________________________
 *
 *  Copyright (c) 2010, Julien Rinaldini, julien.rinaldini@heig-vd.ch
 *  Copyright (c) 2016, Antoine Balestrat, antoine.balestrat<at>polytechnique.edu
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef CONFIG_H
#define CONFIG_H

// Mod (Mod1 == alt) and master size
#define MOD             XCB_MOD_MASK_1
#define MASTER_SIZE     0.6

// Colors
#define FOCUS           "#D64937"
#define UNFOCUS         "#000000"

const char* dmenucmd[] = {"dmenu_run",NULL};
const char* urxvtcmd[] = {"urxvt",NULL};
const char* killit[] =   {"pkill", "catwm-xcb", NULL};
const char* lockcmd[]  = {"slock",NULL};
const char* next[]     = {"ncmpcpp","next",NULL};
const char* prev[]     = {"ncmpcpp","prev",NULL};
const char* toggle[]   = {"ncmpcpp","toggle",NULL };
const char* voldown[]  = {"amixer","set","PCM","5\%-",NULL};
const char* volup[]    = {"amixer","set","PCM","5\%+",NULL};

// Avoid multiple paste
#define DESKTOPCHANGE(K,N) \
    {  MOD,             K,                          change_desktop, {.i = N}}, \
    {  MOD|ShiftMask,   K,                          client_to_desktop, {.i = N}},

// Shortcuts
static struct key keys[] = {
    // MOD              KEY                         FUNCTION        ARGS
    {  MOD,             XK_h,                       decrease,       {NULL}},
    {  MOD,             XK_l,                       increase,       {NULL}},
    {  MOD,             XK_x,                       kill_client,    {NULL}},
    {  MOD,             XK_j,                       prev_win,       {NULL}},
    {  MOD,             XK_Tab,                     next_win,       {NULL}},
    {  MOD,             XK_k,                       spawn,          {.com = killit}},
    {  MOD|ShiftMask,   XK_j,                       move_up,        {NULL}},
    {  MOD|ShiftMask,   XK_k,                       move_down,      {NULL}},
    {  MOD,             XK_Return,                  swap_master,    {NULL}},
    {  MOD,             XK_space,                   switch_mode,    {NULL}},
    {  MOD,             XK_c,                       spawn,          {.com = lockcmd}},
    {  0,               XF86XK_AudioNext,           spawn,          {.com = next}},
    {  0,               XF86XK_AudioPrev,           spawn,          {.com = prev}},
    {  0,               XF86XK_AudioPlay,           spawn,          {.com = toggle}},
    {  0,               XF86XK_AudioLowerVolume,    spawn,          {.com = voldown}},
    {  0,               XF86XK_AudioRaiseVolume,    spawn,          {.com = volup}},
    {  MOD,             XK_p,                       spawn,          {.com = dmenucmd}},
    {  MOD,             XK_u,                       spawn,          {.com = urxvtcmd}},
    {  MOD,             XK_Right,                   next_desktop,   {NULL}},
    {  MOD,             XK_Left,                    prev_desktop,   {NULL}},
       DESKTOPCHANGE(   XK_0,                                       0)
       DESKTOPCHANGE(   XK_1,                                       1)
       DESKTOPCHANGE(   XK_2,                                       2)
       DESKTOPCHANGE(   XK_3,                                       3)
       DESKTOPCHANGE(   XK_4,                                       4)
       DESKTOPCHANGE(   XK_5,                                       5)
       DESKTOPCHANGE(   XK_6,                                       6)
       DESKTOPCHANGE(   XK_7,                                       7)
       DESKTOPCHANGE(   XK_8,                                       8)
       DESKTOPCHANGE(   XK_9,                                       9)
    {  MOD,             XK_q,                       quit,           {NULL}}
};

#endif

