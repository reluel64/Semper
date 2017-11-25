#pragma once

typedef enum
{
    catalog,
    tooltip
} surface_builtin_type;


int surface_builtin_destroy(void **pv);
int surface_builtin_init(void *holder,surface_builtin_type tp);
