#ifndef PCX_PROC_H
#define PCX_PROC_H
#include <stdio.h>
#include "types.h"

// Cut an 8x8 cell out of PCX data, throw it at a file
void pcx_make_tile(pcx_t *pcx, unsigned int x, unsigned int y, FILE *f);
// Take requisite tiles from a sprite and dump tiledata
void pcx_dump_sprite_tiles(pcx_t *pcx_data, sprite_t *spr, FILE *f);
void pcx_dump_tiledata(pcx_t *pcx_data, sprite_t *sprites, FILE *f);
void pcx_destroy(pcx_t *pcx);

#endif
