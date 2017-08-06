#pragma once

void timed_action_init(void**spv,void *ip);
void timed_action_reset(void*spv,void *ip);
void timed_action_destroy(void**spv);
void timed_action_command(void *spv,unsigned char *command);
