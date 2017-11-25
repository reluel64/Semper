#pragma once
#include <stddef.h>
typedef  int(*math_parser_callback)(unsigned char *str,size_t len,double *val,void *pv) ;
int math_parser(unsigned char *formula,double *res,math_parser_callback mpc,void *pv);
