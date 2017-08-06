/*Credit goes to https://github.com/tcr*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

static uint32_t uppercase_delta(uint32_t point)
{
    switch(point)
    {
#include <3rdparty/upper.h>
    default:
        return 0;
    }
}

uint32_t uppercase(uint32_t point)
{
    return point - uppercase_delta(point);
}

static uint32_t lowercase_delta(uint32_t point)
{
    switch(point)
    {
#include <3rdparty/lower.h>
    default:
        return 0;
    }
}

uint32_t lowercase(uint32_t point)
{
    return point - lowercase_delta(point);
}
