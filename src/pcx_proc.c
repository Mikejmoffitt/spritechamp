#include "pcx_proc.h"
#include "types.h"
#include <stdlib.h>

// Cut an 8x8 cell out of PCX data, throw it at a file
void pcx_make_tile(pcx_t *pcx, unsigned int x, unsigned int y, FILE *f)
{
	uint8_t tdata[32];
	for (unsigned int i = 0; i < 8; i++)
	{
		unsigned int y_idx = (i + y) * pcx->w;
		for (unsigned int j = 0; j < 8; j++)
		{
			unsigned int t_idx = (4 * i) + (j / 2);
			unsigned int x_idx = x + j;
			if ((y+i < pcx->h) && (x+j < pcx->w))
			{
				// Upper nybble
				if (j % 2 == 0)
				{
					tdata[t_idx] = (pcx->data[y_idx + x_idx] & 0x0F) << 4;
				}
				// Lower nybble
				else
				{
					tdata[t_idx] |= pcx->data[y_idx + x_idx] & 0x0F;
				}
			}
			else
			{
				// Upper nybble
				if (j % 2 == 0)
				{
					tdata[t_idx] = 0;
				}
				// Lower nybble
				else
				{
					tdata[t_idx] &= 0xF0;
				}
			}
		}
	}
	fwrite((void *)tdata, 32, 1, f);
}

// Take requisite tiles from a sprite and dump tiledata
void pcx_dump_sprite_tiles(pcx_t *pcx, sprite_t *spr, FILE *f)
{
	unsigned int spr_count = 0;
	//printf("Tile [%d, %d : %d x %d]:\n", spr->x, spr->y, spr->w, spr->h);
	for (int x = 0; x < spr->w; x += 8)
	{
		for (int y = 0; y < spr->h; y += 8)
		{
			if (spr->w == 0 || spr->h == 0)
			{
				//printf(" unused (#%d)\n", spr_count);
			}
			else
			{
				//printf(" %02d, %02d (#%d)\n", x, y, spr_count);
				pcx_make_tile(pcx, spr->x+x, spr->y+y, f);
			}
			spr_count++;
		}
	}
}

// Dump all sprite tiledata.
void pcx_dump_tiledata(pcx_t *pcx_data, sprite_t *sprites, FILE *f)
{
	for (unsigned int i = 0; i < MAX_SPR; i++)
	{
		if (sprites[i].w == 0 || sprites[i].h == 0)
		{
			return;
		}
		pcx_dump_sprite_tiles(pcx_data, &sprites[i], f);
	}
}

void pcx_destroy(pcx_t *pcx)
{
	if (pcx->data)
	{
		free(pcx->data);
	}
	if (pcx->pal)
	{
		free(pcx->pal);
	}
}
