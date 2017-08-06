#pragma once

unsigned char* enumerator_next_value(void* ed);
unsigned char* enumerator_first_value(void* r, int rt, void** ed);
void enumerator_finish(void** ed);

#define ENUMERATOR_OBJECT 0x1
#define ENUMERATOR_SOURCE 0x0
