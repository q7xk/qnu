#ifndef QNU_PART_H
#define QNU_PART_H
#include <qnu/types.h>
typedef struct { u64 first_lba, sectors; u32 type; bool gpt; } partition_t;
int part_scan_mbr(const char *blk, partition_t *parts, int max);
int part_scan_gpt(const char *blk, partition_t *parts, int max);
#endif
