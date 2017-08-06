#pragma once

typedef struct
{
    unsigned char name[1024]; /*Item name*/
    unsigned char type;       /*Item type:
                                *1-object
                                *2-source
                                *3-surface
                                */
    void *pv;                 /*our path to the core*/
} script_item_data;

unsigned char *script_source_name(void *sc);
unsigned char *script_param_retrieve(script_item_data *sid,unsigned char *res,unsigned char *def,unsigned char xflags);
double script_object_param(script_item_data *sid,unsigned char id);
void script_set_param(script_item_data *sid,unsigned char *p,unsigned char *v);
void script_send_command(void *pv,unsigned char *cmd);
unsigned char *script_xpand(void *pv,unsigned char *s);
void *script_source_param(script_item_data *sid,unsigned char id);
unsigned char *script_variable(void *pv,unsigned char *s);
double script_math(unsigned char *frml);
