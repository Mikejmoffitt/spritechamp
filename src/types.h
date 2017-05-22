#ifndef TYPES_H
#define TYPES_H
#include <stdint.h>
#include "pcx.h"

#define MIN(x, y) (x < y ? x : y)
#define MAX(x, y) (x > y ? x : y)
#define MAX_SPR 24
typedef PcxFile pcx_t;

#define PAK_SIZE_INVAL 0xFF

typedef struct sprite_t
{
	// Region to snip from
	unsigned int x, y;
	unsigned int w, h;
} sprite_t;

typedef struct pak_entry_t
{
	uint8_t data_idx[4]; // Index within pak for data
	uint8_t numtiles[2]; // Tile count (size / 32)
	int8_t x[MAX_SPR]; // Relative to (SPR_W/2)-1
	int8_t y[MAX_SPR]; // Relative to SPR_H-1
	uint8_t size[MAX_SPR]; // 4-bit format native to MD
} pak_entry_t;


#endif
