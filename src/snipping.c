#include "snipping.h"
#include "types.h"
#include <stdlib.h>
#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>

static unsigned int spr_idx = 0;
static ALLEGRO_COLOR transparent_color;
static ALLEGRO_BITMAP *spr_buffer;

#ifdef DEBUG_VIS

extern ALLEGRO_BITMAP *main_buffer;
extern void flip();

#endif

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


// Compare two pixel values.
// TODO: shouldn't need this once we are using PCX data directly.
static int color_match(ALLEGRO_COLOR a, ALLEGRO_COLOR b)
{
	unsigned char r1, g1, b1;
	unsigned char r2, g2, b2;

	al_unmap_rgb(a, &r1, &g1, &b1);
	al_unmap_rgb(b, &r2, &g2, &b2);

	return (r1 == r2 && g1 == g2 && b1 == b2);
}

// Check if a region is empty in the image.
// TODO: Supply image, don't use a global
static int area_is_empty(int x, int y, int x2, int y2)
{
	x = MAX(x, 0);
	y = MAX(y, 0);
	x2 = MIN(x2, al_get_bitmap_width(spr_buffer));
	y2 = MIN(y2, al_get_bitmap_height(spr_buffer));
	al_lock_bitmap(spr_buffer, al_get_bitmap_format(spr_buffer), ALLEGRO_LOCK_READONLY);
	for (int i = y; i < y2; i++)
	{
		for (int j = x; j < x2; j++)
		{
			if (!color_match(al_get_pixel(spr_buffer, j, i), transparent_color))
			{
				return 0;
			}
		}
	}
	al_unlock_bitmap(spr_buffer);
	return 1;
}

// Creates a sprite entry as specified, and then removes the region from the
// tested bitmap.
// TODO: Parameterize bitmap
void snip_sprite(sprite_t *sprites, int x, int y, int w, int h)
{
	if (spr_idx >= MAX_SPR)
	{
		//printf("Warning: Overrunning max sprite count of %d.\n", spr_idx);
	}
	else
	{
		//printf("Spr $%X: (%d, %d) --> (%d, %d)\n", spr_idx, x, y, w, h);
		sprites[spr_idx].x = x;
		sprites[spr_idx].y = y;
		sprites[spr_idx].w = w;
		sprites[spr_idx].h = h;
	}

	spr_idx++;
#ifdef DEBUG_VIS
	al_draw_filled_rectangle(x, y, x + w, y + h, al_map_rgb(0,0,0));
	for (int i = 0; i < 2; i++)
	{
		flip();
	}
#endif

	al_set_target_bitmap(spr_buffer);
	al_draw_filled_rectangle(x, y, x + w, y + h, transparent_color);
#ifdef DEBUG_VIS
	al_set_target_bitmap(main_buffer);

	al_draw_bitmap(spr_buffer, 0, 0, 0);
	flip();
#endif
}

// The main algorithm which pulls sprites out.
// TODO: Parameterize bitmap, etc.
static void claim(sprite_t *sprites)
{
	unsigned int orig_x, orig_y;
	unsigned int w, h;
	unsigned int img_w, img_h;
	img_w = al_get_bitmap_width(spr_buffer);
	img_h = al_get_bitmap_height(spr_buffer);
	// First, find topmost line to have data taken from it
	//printf("Finding top line\n");
	for (orig_y = 0; orig_y < img_h; orig_y += 1)
	{
		if (!area_is_empty(0, orig_y, img_w, orig_y+1))
		{
			//printf("Spr $%X: Top at %d\n", spr_idx, orig_y);
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
			if (!area_is_empty(orig_x, orig_y, orig_x+1, orig_y+h))
			{
				//printf("Spr $%X: Left at %d\n", spr_idx, orig_x);
				break;
			}
			orig_x++;
		}
		while (w > 8)
		{
			// Shrink width as much as possible
			if (area_is_empty(orig_x+w-1, orig_y, orig_x+w, orig_y+h))
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
		if (!area_is_empty(orig_x, ycopy+24, orig_x+w, ycopy+32))
		{
			goto chk_top;
		}
		h -= 8;
		if (!area_is_empty(orig_x, ycopy+16, orig_x+w, ycopy+24))
		{
			goto chk_top;
		}
		h -= 8;
		if (!area_is_empty(orig_x, ycopy+8, orig_x+w, ycopy+16))
		{
			goto chk_top;
		}
		h -= 8;

chk_top:
		// Above
		if (h <= 8 || !area_is_empty(orig_x, ycopy, orig_x+w, ycopy+8))
		{
			goto do_snip;
		}
		ycopy += 8;
		if (h <= 16 || !area_is_empty(orig_x, ycopy, orig_x+w, ycopy+8))
		{
			goto do_snip;
		}
		ycopy += 8;
		if (h <= 24 || !area_is_empty(orig_x, ycopy, orig_x+w, ycopy+8))
		{
			goto do_snip;
		}
		ycopy += 8;

do_snip:
		h -= ycopy - orig_y;

		if (!area_is_empty(orig_x, ycopy, orig_x+w, orig_y+h))
		{
			snip_sprite(sprites, orig_x, ycopy, w, h);
		}
		orig_x += w;
	}
	while (!area_is_empty(orig_x, orig_y, img_w, orig_y + h));
}

// Top-level to call the algorithm until we're done
void place_sprites(const char *fname, sprite_t *sprites)
{
	al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
	spr_buffer = al_load_bitmap(fname);
	if (!spr_buffer)
	{
		fprintf(stderr, "Couldn't load sprite `%s`.\n", fname);
		return;
	}
	spr_idx = 0;

	// Get transparent color from top-left of the image
	transparent_color = al_get_pixel(spr_buffer, 0, 0);
	while (!area_is_empty(0, 0, al_get_bitmap_width(spr_buffer), al_get_bitmap_height(spr_buffer)))
	{
		claim(sprites);
	}

	while (spr_idx < MAX_SPR)
	{
		sprites[spr_idx].w = 0;
		sprites[spr_idx].h = 0;
		spr_idx++;
	}

	al_destroy_bitmap(spr_buffer);
}
