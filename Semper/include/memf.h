#pragma once
typedef struct _memf memf;
size_t mwrite(const void *buf, size_t size, size_t count, memf *mf);
size_t mread(void *buf, size_t size, size_t count, memf *mf);
size_t mseek(memf *mf, long off, int origin);
memf *mopen(size_t init);
int mprintf(memf *mf, const char *format, ...);
long int mtell(memf *mf);
void mclose(memf **mf);
char *mgets(char *str, int num, memf *mf);
