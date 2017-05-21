#include <stdlib.h>
#include <stdio.h>
#include "snipping.h"
#include "types.h"
#include "pcx_proc.h"
#include "pcx.h"

/*


Sprite origin point for a 256x128 image:

  (0, 0)
       _______________
      |               |
      |               |
      |               |
      |   (128, 128)  |
      |_______X_______|

                   (256, 128)

Sprites making one metasprite are represented with their positions relative to
the origin point. A sprite in the very top-left would be described as being at
(-128, -128).

If the image dimensions change, it should not be a problem.

A full character image could be 256x256 with this system, but everything below
the image's 128 height (character 0) would be below the character.

*/

static inline uint8_t get_pixel(pcx_t *pcx, unsigned int x, unsigned int y)
{
	return pcx->data[(y * pcx->w) + x];
}

static void draw_rect(pcx_t *pcx, unsigned int x, unsigned int y,
                      unsigned int x2, unsigned int y2, uint8_t val)
{
	for (unsigned int i = y; i < y2; i++)
	{
		for (unsigned int j = x; j < x2; j++)
		{
			pcx->data[(i * pcx->w) + j] = val;
		}
	}
}

// Check if a region is empty in the image.
static int area_is_empty(pcx_t *pcx, int x, int y, int x2, int y2)
{
	x = MAX(x, 0);
	y = MAX(y, 0);
	x2 = MIN(x2, pcx->w);
	y2 = MIN(y2, pcx->h);
	for (int i = y; i < y2; i++)
	{
		for (int j = x; j < x2; j++)
		{
			if (get_pixel(pcx, j, i) != 0)
			{
				return 0;
			}
		}
	}
	return 1;
}

// Creates a sprite entry as specified, and then removes the region from the
// tested bitmap.
void snip_sprite(pcx_t *pcx, unsigned int *spr_idx, sprite_t *sprites, int x, int y)
{
	unsigned int w = pcx->w;
	unsigned int h = pcx->h;
	if (*spr_idx >= MAX_SPR)
	{
		//printf("Warning: Overrunning max sprite count of %d.\n", *spr_idx);
	}
	else
	{
		//printf("Spr $%X: (%d, %d) --> (%d, %d)\n", *spr_idx, x, y, w, h);
		sprites[*spr_idx].x = x;
		sprites[*spr_idx].y = y;
		sprites[*spr_idx].w = w;
		sprites[*spr_idx].h = h;
	}

	*spr_idx = *spr_idx + 1;

	draw_rect(pcx, x, y, x + w, y + h, 0);
}

// The main algorithm which pulls sprites out.
static void claim(pcx_t *pcx, unsigned int *spr_idx, sprite_t *sprites)
{
	unsigned int orig_x, orig_y;
	unsigned int w, h;
	unsigned int img_w, img_h;
	img_w = pcx->w;
	img_h = pcx->h;
	// First, find topmost line to have data taken from it
	//printf("Finding top line\n");
	for (orig_y = 0; orig_y < img_h; orig_y += 1)
	{
		if (!area_is_empty(pcx, 0, orig_y, img_w, orig_y+1))
		{
			//printf("Spr $%X: Top at %d\n", *spr_idx, orig_y);
			break;
		}
	}

	orig_x = 0;
	// Now that the left is found, eat away the row
	do
	{
		h = 32;
		w = 32;
		//printf("Finding left side\n");
		while (orig_x < img_w)
		{
			if (!area_is_empty(pcx, orig_x, orig_y, orig_x+1, orig_y+h))
			{
				//printf("Spr $%X: Left at %d\n", *spr_idx, orig_x);
				break;
			}
			orig_x++;
		}
		while (w > 8)
		{
			// Shrink width as much as possible
			if (area_is_empty(pcx, orig_x+w-1, orig_y, orig_x+w, orig_y+h))
			{
				w -= 1;
			}
			else
			{
				break;
			}
		}
		w = (w + 7) & 0xFFF8;
		// Now try to close in on Y
		unsigned int ycopy = orig_y;

		// Below
		if (!area_is_empty(pcx, orig_x, ycopy+24, orig_x+w, ycopy+32))
		{
			goto chk_top;
		}
		h -= 8;
		if (!area_is_empty(pcx, orig_x, ycopy+16, orig_x+w, ycopy+24))
		{
			goto chk_top;
		}
		h -= 8;
		if (!area_is_empty(pcx, orig_x, ycopy+8, orig_x+w, ycopy+16))
		{
			goto chk_top;
		}
		h -= 8;

chk_top:
		// Above
		if (h <= 8 || !area_is_empty(pcx, orig_x, ycopy, orig_x+w, ycopy+8))
		{
			goto do_snip;
		}
		ycopy += 8;
		if (h <= 16 || !area_is_empty(pcx, orig_x, ycopy, orig_x+w, ycopy+8))
		{
			goto do_snip;
		}
		ycopy += 8;
		if (h <= 24 || !area_is_empty(pcx, orig_x, ycopy, orig_x+w, ycopy+8))
		{
			goto do_snip;
		}
		ycopy += 8;

do_snip:
		h -= ycopy - orig_y;

		if (!area_is_empty(pcx, orig_x, ycopy, orig_x+w, orig_y+h))
		{
			snip_sprite(pcx, spr_idx, sprites, orig_x, ycopy);
		}
		orig_x += w;
	}
	while (!area_is_empty(pcx, orig_x, orig_y, img_w, orig_y + h));
}

// Top-level to call the algorithm until we're done
void place_sprites(const char *fname, sprite_t *sprites)
{
	pcx_t pcx;
	unsigned int spr_idx = 0;

	// keep our own copy of the PCX here since we're going to mutatee it
	pcx_new(&pcx, fname);

	// TODO: This loop shouldn't be needed unless something is wrong with claim
	while (!area_is_empty(&pcx, 0, 0, pcx.w, pcx.h))
	{
		claim(&pcx, &spr_idx, sprites);
	}

	// Mark unused sprites
	while (spr_idx < MAX_SPR)
	{
		sprites[spr_idx].w = 0;
		sprites[spr_idx].h = 0;
		spr_idx++;
	}

	pcx_destroy(&pcx);
}
