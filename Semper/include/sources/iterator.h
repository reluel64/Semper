#include <stdio.h>
typedef struct _iterator_inf
{
    long start_value;
    long end_value;
    long step;
    long current_value;
    unsigned char stop;
    size_t curent_count;
    size_t rep_count;

} iterator_info;

void iterator_init(void** spv, void* ip);
void iterator_reset(void* spv, void* ip);
double iterator_update(void* spv);
void iterator_destroy(void** spv);
void iterator_command(void* spv, unsigned char* command);
