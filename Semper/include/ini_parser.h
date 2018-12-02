#pragma once
#include <stddef.h>
#define INI_PARSER_MAX_LINE 65535

typedef int (*ini_handler)(unsigned char* sn, unsigned char* kn, unsigned char* kv, unsigned char* com, void* pv);
typedef char* (*ini_reader)(unsigned char* buf, size_t buf_sz, void* pv);
#define ini_handler(func) (func)(unsigned char* sn, unsigned char* kn, unsigned char* kv, unsigned char* com, void* pv)
int ini_parser_parse_stream(ini_reader ir, void* data, ini_handler ih, void* pv);
int ini_parser_parse_file(unsigned char* fn, ini_handler ih, void* pv);
