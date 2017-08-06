void surfaces_collector_init(void** spv, void* ip);
unsigned char* surfaces_collector_string(void* spv);
void surfaces_collector_destroy(void** spv);
void surfaces_collector_reset(void* spv, void* ip);
double surfaces_collector_update(void* spv);
void surfaces_collector_command(void* spv, unsigned char* command);
