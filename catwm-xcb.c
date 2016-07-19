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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <stdarg.h>

#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <X11/X.h>
#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>

#define TABLENGTH(X)    (sizeof(X)/sizeof(*X))

typedef union
{
    const char **com;
    const int i;
} Arg;

struct key
{
    unsigned int mod;
    xcb_keysym_t keysym;
    void (*function)(const Arg arg);
    const Arg arg;
};

typedef struct client client;
struct client
{
    client *next;
    client *prev;

    xcb_window_t window;
};

typedef struct desktop desktop;
struct desktop
{
    int master_size;
    int mode;
    client *head;
    client *current;
};

// Functions
static void add_window(xcb_window_t w);
static void change_desktop(const Arg arg);
static void client_to_desktop(const Arg arg);
static void configurenotify(xcb_configure_notify_event_t *e);
static void configurerequest(xcb_configure_request_event_t *e);
static void decrease();
static void destroynotify(xcb_destroy_notify_event_t *e);
static void die(const char *format, ...);
static unsigned long get_color(const char* color);
static void grabkeys();
static void increase();
static void keypress(xcb_key_press_event_t *e);
static void kill_client();
static void maprequest(xcb_map_request_event_t *e);
static void move_down();
static void move_up();
static void next_desktop();
static void next_win();
static void prev_desktop();
static void prev_win();
static void quit();
static void remove_window(xcb_window_t w);
static void save_desktop(int i);
static void select_desktop(int i);
//static void send_kill_signal(xcb_window_t w);
static void setup();
static void sigchld(int unused);
static void spawn(const Arg arg);
static void start();
//static void swap();
static void swap_master();
static void switch_mode();
static void tile();
static void update_current();

// Include configuration file (need struct key)
#include "config.h"

// Variable
static xcb_connection_t *connection;
static int bool_quit;
static int current_desktop;
static int master_size;
static int mode;
static int sh;
static int sw;
static int screenNum;
static unsigned int win_focus;
static unsigned int win_unfocus;
static client *head;
static client *current;

xcb_key_symbols_t *keysyms;

// Desktop array
static desktop desktops[10];
xcb_screen_t *screen;

xcb_keysym_t keycode_to_keysym(xcb_keycode_t keycode)
{
    return xcb_key_symbols_get_keysym(keysyms, keycode, 0);
}

void add_window(xcb_window_t w)
{
    client *c,*t;

    if(!(c = (client *)calloc(1,sizeof(client))))
        die("calloc failed !");

    if(head == NULL)
    {
        c->next = NULL;
        c->prev = NULL;
        c->window = w;
        head = c;
    }
    else
    {
        for(t=head; t->next; t=t->next);

        c->next = NULL;
        c->prev = t;
        c->window = w;

        t->next = c;
    }

    current = c;
}

void change_desktop(const Arg arg)
{
    client *c;

    if(arg.i == current_desktop)
        return;

    // Unmap all window
    if(head != NULL)
        for(c=head; c; c=c->next)
            xcb_unmap_window(connection, c->window);

    // Save current "properties"
    save_desktop(current_desktop);

    // Take "properties" from the new desktop
    select_desktop(arg.i);

    // Map all windows
    if(head != NULL)
        for(c=head; c; c=c->next)
            xcb_map_window(connection, c->window);

    tile();
    update_current();
}

void client_to_desktop(const Arg arg)
{
    client *tmp = current;
    int tmp2 = current_desktop;

    if(arg.i == current_desktop || current == NULL)
        return;

    // Add client to desktop
    select_desktop(arg.i);
    add_window(tmp->window);
    save_desktop(arg.i);

    // Remove client from current desktop
    select_desktop(tmp2);
    remove_window(current->window);

    tile();
    update_current();
}

void configurenotify(xcb_configure_notify_event_t *e)
{
    // Do nothing for the moment
}

void configurerequest(xcb_configure_request_event_t *e)
{
    const uint32_t values[] = {e->x, e->y, e->width, e->height, e->border_width, e->sibling, e->stack_mode};
    xcb_configure_window(connection, e->window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_BORDER_WIDTH | XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE, values);
}

void decrease()
{
    if(master_size > 50)
    {
        master_size -= 10;
        tile();
    }
}

void destroynotify(xcb_destroy_notify_event_t *e)
{
    int i = 0;
    client *c = NULL;

    // Uber (and ugly) hack
    for(c=head; c; c=c->next)
        if(e->window == c->window)
            i++;
    // End of the hack

    if(i == 0)
        return;

    remove_window(e->window);
    tile();
    update_current();
}

void die(const char *format, ...)
{
    va_list vargs;

    va_start(vargs, format);
    fprintf(stderr, "catwm-xcb: ");
    vfprintf(stderr, format, vargs);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

// Thanks monsterwm
static unsigned int get_colorpixel(const char *hex)
{
    char strgroups[3][3]  = {{hex[1], hex[2], '\0'}, {hex[3], hex[4], '\0'}, {hex[5], hex[6], '\0'}};
    unsigned int rgb16[3] = {(strtol(strgroups[0], NULL, 16)), (strtol(strgroups[1], NULL, 16)), (strtol(strgroups[2], NULL, 16))};
    return (rgb16[0] << 16) + (rgb16[1] << 8) + rgb16[2];
}

// ras
unsigned long get_color(const char* color)
{
    xcb_colormap_t map = screen->default_colormap;
    xcb_alloc_color_reply_t *c;
    unsigned long pixel, rgb = get_colorpixel(color);

    c = xcb_alloc_color_reply(connection, xcb_alloc_color(connection, map, (rgb >> 16) * 257, (rgb >> 8 & 255) * 257, (rgb & 256)* 257), NULL);
    if (!c)
        die("cannot allocate color '%s'", color);

    pixel = c->pixel;
    free(c);
    return pixel;
}

void grabkeys()
{
    int i;
    xcb_keycode_t *code = NULL;

    xcb_ungrab_key(connection, XCB_GRAB_ANY, screen->root, XCB_MOD_MASK_ANY);

    // For every shortcuts
    for(i=0; i<TABLENGTH(keys); ++i)
        if((code = xcb_key_symbols_get_keycode(keysyms, keys[i].keysym)))
            xcb_grab_key(connection, 1, screen->root, keys[i].mod, *code, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
}

void increase()
{
    if(master_size < sw-50)
    {
        master_size += 10;
        tile();
    }
}

void keypress(xcb_key_press_event_t *e)
{
    int i;
    xcb_keysym_t keysym = keycode_to_keysym(e->detail);

    for(i=0; i<TABLENGTH(keys); ++i)
        if(keys[i].keysym == keysym && keys[i].mod == e->state)
            keys[i].function(keys[i].arg);
}

xcb_atom_t get_intern_atom(const char *name)
{
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(connection, xcb_intern_atom(connection, 0, strlen(name), name), NULL);

    if(!reply)
        die("couldn't retrieve atom %s", name);

    xcb_atom_t ret = reply->atom;

    free(reply);
    return ret;
}

// WARNING: this is NOT ICCCM-compliant !
void kill_client()
{
    if(current != NULL)
        xcb_kill_client(connection, current->window);
 }

void maprequest(xcb_map_request_event_t *e)
{
    // In case the client has already made a map request but yields another one later on.
    client *c;
    for(c=head; c; c=c->next)
        if(e->window == c->window)
        {
            xcb_map_window(connection, e->window);
            return;
        }

    // Otherwise, it is the first time we hear about it.
    add_window(e->window);
    xcb_map_window(connection, e->window);
    tile();
    update_current();
}

void move_down()
{
    xcb_window_t tmp;
    if(current == NULL || current->next == NULL || current->window == head->window || current->prev == NULL)
    {
        return;
    }
    tmp = current->window;
    current->window = current->next->window;
    current->next->window = tmp;
    //keep the moved window activated
    next_win();
    tile();
    update_current();
}

void move_up()
{
    xcb_window_t tmp;

    if(current == NULL || current->prev == head || current->window == head->window)
        return;

    tmp = current->window;
    current->window = current->prev->window;
    current->prev->window = tmp;
    prev_win();
    tile();
    update_current();
}

void next_desktop()
{
    int tmp = current_desktop;
    if(tmp== 9)
        tmp = 0;
    else
        tmp++;

    Arg a = {.i = tmp};
    change_desktop(a);
}

void next_win()
{
    client *c;

    if(current != NULL && head != NULL)
    {
        if(current->next == NULL)
            c = head;
        else
            c = current->next;

        current = c;
        update_current();
    }
}

void prev_desktop()
{
    int tmp = current_desktop;
    if(tmp == 0)
        tmp = 9;
    else
        tmp--;

    Arg a = {.i = tmp};
    change_desktop(a);
}

void prev_win()
{
    client *c;

    if(current != NULL && head != NULL)
    {
        if(current->prev == NULL)
            for(c=head; c->next; c=c->next);
        else
            c = current->prev;

        current = c;
        update_current();
    }
}

// TODO: implement
void quit()
{
    /*Window root_return, parent;
    Window *children;
    int i;
    unsigned int nchildren;
    XEvent ev;


    if(bool_quit == 1) {
        XUngrabKey(dis, AnyKey, AnyModifier, root);
        XDestroySubwindows(dis, root);
        fprintf(stdout, "catwm: Thanks for using!\n");
        XCloseDisplay(dis);
        die("forced shutdown");
    }

    bool_quit = 1;
    XQueryTree(dis, root, &root_return, &parent, &children, &nchildren);
    for(i = 0; i < nchildren; i++) {
        send_kill_signal(children[i]);
    }
    //keep alive until all windows are killed
    while(nchildren > 0) {
        XQueryTree(dis, root, &root_return, &parent, &children, &nchildren);
        XNextEvent(dis,&ev);
        if(events[ev.type])
            events[ev.type](&ev);
    }

    XUngrabKey(dis,AnyKey,AnyModifier,root);
    fprintf(stdout,"catwm: Thanks for using!\n");*/
}

void remove_window(xcb_window_t w)
{
    client *c;

    // CHANGE THIS UGLY CODE
    for(c=head; c; c=c->next)
    {

        if(c->window == w)
        {
            if(c->prev == NULL && c->next == NULL)
            {
                free(head);
                head = NULL;
                current = NULL;
                return;
            }

            if(c->prev == NULL)
            {
                head = c->next;
                c->next->prev = NULL;
                current = c->next;
            }
            else if(c->next == NULL)
            {
                c->prev->next = NULL;
                current = c->prev;
            }
            else
            {
                c->prev->next = c->next;
                c->next->prev = c->prev;
                current = c->prev;
            }

            free(c);
            return;
        }
    }
}

void save_desktop(int i)
{
    desktops[i].master_size = master_size;
    desktops[i].mode = mode;
    desktops[i].head = head;
    desktops[i].current = current;
}

void select_desktop(int i)
{
    head = desktops[i].head;
    current = desktops[i].current;
    master_size = desktops[i].master_size;
    mode = desktops[i].mode;
    current_desktop = i;
}

void sigchld(int unused)
{
    // Again, thx to dwm ;)
    if(signal(SIGCHLD, sigchld) == SIG_ERR)
        die("Can't install SIGCHLD handler");
    while(0 < waitpid(-1, NULL, WNOHANG));
}

void spawn(const Arg arg)
{
    if(fork() == 0)
    {
        if(fork() == 0)
        {
            if(connection)
                close(xcb_get_file_descriptor(connection));

            setsid();
            execvp((char*)arg.com[0],(char**)arg.com);
        }
        exit(0);
    }
}

void start()
{
    xcb_generic_event_t *ge = NULL;

    // Main loop, just dispatch events ;)
    while(!bool_quit)
    {
    	xcb_flush(connection);

    	if((ge = xcb_wait_for_event(connection)))
    	{
	        switch(ge->response_type & ~0x80)
	        {
	            case XCB_KEY_PRESS:
	                keypress((xcb_key_press_event_t*)ge);
	                break;

	            case XCB_MAP_REQUEST:
	                puts("maprequest");
	                maprequest((xcb_map_request_event_t*)ge);
	                break;

	            case XCB_DESTROY_NOTIFY:
	                puts("destroynotify");
	                destroynotify((xcb_destroy_notify_event_t*)ge);
	                break;

	            case XCB_CONFIGURE_NOTIFY:
	                puts("configurenotify");
	                configurenotify((xcb_configure_notify_event_t*)ge);
	                break;

	            case XCB_CONFIGURE_REQUEST:
	                puts("configurerequest");
	                configurerequest((xcb_configure_request_event_t*)ge);
	                break;

	            default:
	                break;
	        }

	        free(ge);
	    }
    }
}

void swap_master()
{
    xcb_window_t tmp;

    if(head != NULL && current != NULL && current != head && mode == 0)
    {
        tmp = head->window;
        head->window = current->window;
        current->window = tmp;
        current = head;

        tile();
        update_current();
    }
}

void switch_mode()
{
    mode = (int)(mode == 0);
    tile();
    update_current();
}

void move_window(xcb_window_t window, int x, int y, int w, int h)
{
    const unsigned int values[4] = {x,y,w,h};
    xcb_configure_window(connection, window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
}

void tile()
{
    client *c;
    int n = 0;
    int y = 0;

    // If only one window
    if(head && !head->next)
        move_window(head->window, 0, 0, sw-2, sh-2);
    else if(head)
    {
        switch(mode)
        {
	        case 0:
	            // Master window
	            move_window(head->window, 0, 0, master_size-2, sh-2);

	            // Stack
	            for(c = head->next; c; c = c->next)
	            	++n;

	            for(c = head->next; c; c = c->next)
	            {
	                move_window(c->window, master_size, y, sw-master_size-2, (sh/n)-2);
	                y += sh/n;
	            }
	            break;

	        case 1:
	            for(c = head; c; c = c->next)
	                move_window(c->window, 0, 0, sw, sh);
	            break;

	        default:
	            break;
        }
    }
}

void update_current()
{
    client *c;
    uint32_t values[1] = {1};

    for(c = head; c; c = c->next)
    {
        if(current == c)
        {
            // Adjust border width and border color
            xcb_configure_window(connection, c->window, XCB_CONFIG_WINDOW_BORDER_WIDTH, values);
            xcb_change_window_attributes(connection, c->window, XCB_CW_BORDER_PIXEL, &win_focus);

            // Give focus
            xcb_set_input_focus(connection, XCB_INPUT_FOCUS_PARENT, c->window, XCB_CURRENT_TIME);

            // Place above
            values[0] = XCB_STACK_MODE_ABOVE;
            xcb_configure_window(connection, c->window, XCB_CONFIG_WINDOW_STACK_MODE, values);
        }

        else
        {
            xcb_configure_window(connection, c->window, XCB_CONFIG_WINDOW_BORDER_WIDTH, values);
            xcb_change_window_attributes(connection, c->window, XCB_CW_BORDER_PIXEL, &win_unfocus);
        }
    }

    xcb_flush(connection);
}

// Grab events on the root window. If we can't, then another WM is already listening !
static void register_events(void)
{
    unsigned int values[1] = {XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_BUTTON_PRESS};
    xcb_generic_error_t *error = xcb_request_check(connection, xcb_change_window_attributes_checked(connection, screen->root, XCB_CW_EVENT_MASK, values));
    
    xcb_flush(connection);

    if(error)
    	die("another WM is already running !");

    free(error);
}

void setup()
{
    int i;

    // Install a signal
    sigchld(0);

    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(connection));

    for(i = 0; i < screenNum; ++i)
        xcb_screen_next(&iter);

    screen = iter.data;

    register_events();

    // Screen width and height
    sw = screen->width_in_pixels;
    sh = screen->height_in_pixels;

    // Colors
    win_focus = get_color(FOCUS);
    win_unfocus = get_color(UNFOCUS);

    if(!(keysyms = xcb_key_symbols_alloc(connection)))
        die("couldn't allocate keysyms !");

    // Shortcuts
    grabkeys();

    // Vertical stack
    mode = 0;

    bool_quit = 0;

    head = current = NULL;

    // Master size
    master_size = sw*MASTER_SIZE;

    // Set up all desktop
    for(i=0; i < TABLENGTH(desktops); ++i)
    {
        desktops[i].master_size = master_size;
        desktops[i].mode = mode;
        desktops[i].head = head;
        desktops[i].current = current;
    }

    // Select first desktop by default
    const Arg arg = {.i = 1};
    current_desktop = arg.i;
    change_desktop(arg);
}

int main(int argc, char **argv)
{
    // Connect to the X server through XCB
    if(!(connection = xcb_connect(NULL, &screenNum)))
        die("Cannot open display!");

    // Setup env
    setup();

    // Start WM
    start();

    // Disconnect
    xcb_disconnect(connection);

    return 0;
}

