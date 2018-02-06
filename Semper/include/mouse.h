#pragma once
#include <stddef.h>
#include <crosswin/crosswin.h>
#define MOUSE_SURFACE 0x1
#define MOUSE_OBJECT 0x2
#define MOUSE_LEFT 0x1
#define MOUSE_MIDDLE 0x3
#define MOUSE_RIGHT 0x2
#define MOUSE_PRESSED 255
#define MOUSE_RELEASED 0

typedef struct _mouse_actions
{
    /*old states*/
    mouse_button_state ombs;
    mouse_button       omb;
    mouse_hover_state omhs;
    // Mouse down
    unsigned char* lcd;
    unsigned char* mcd;
    unsigned char* rcd;

    // Mouse up
    unsigned char* lcu;
    unsigned char* mcu;
    unsigned char* rcu;

    // Mouse double click
    unsigned char* lcdd;
    unsigned char* mcdd;
    unsigned char* rcdd;

    unsigned char* scrl_up;
    unsigned char* scrl_down;

    unsigned char* ha;
    unsigned char* nha;
} mouse_actions;

int mouse_set_actions(void *pv,unsigned char tgt);
void mouse_destroy_actions( mouse_actions* ma);
int mouse_handle_button(void* pv, unsigned char mode, mouse_status* ms);
