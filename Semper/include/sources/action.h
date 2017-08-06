#pragma once
#include <stdio.h>
#include <sources/source.h>
typedef enum _action_type { unset, below, equal, above, condition, match } action_type;

typedef struct _action
{
    size_t index;
    unsigned char done;
    /////////////ACTION STUFF/////////////////
    double val;
    unsigned char* actstr;
    //////////CONDITIONAL STUFF//////////////
    unsigned char* cond_e;
    unsigned char* cond_t;
    unsigned char* cond_f;
    /////////////////////////////////////////
    list_entry current;
} action;

typedef struct _source_action
{

    list_entry blw;
    list_entry equ;
    list_entry abv;
    list_entry cond;
    list_entry match;

} source_action;

void action_init(source* s);
void action_destroy(source* s);
void action_reset(source* s);
void action_execute(source* s);
