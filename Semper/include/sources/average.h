#pragma once

#include <stddef.h>
#include <sources/source.h>

typedef struct _avg_val
{
    double value;
    list_entry current;
} avg_val;

typedef struct _average
{
    size_t count; // current number of elements in list
    size_t target; // the target of elements that have to be in the list
    list_entry values;
    double total;
} average;

double average_update(average** avg, size_t avg_count, double value);
void average_destroy(average** a);
