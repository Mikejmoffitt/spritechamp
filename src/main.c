#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <pthread.h>
#include "pcx.h"

typedef PcxFile pcx_t;

#define SPR_W 256
#define SPR_H 128
#define MAX_SPR 32

#define MIN(x, y) (x < y ? x : y)
#define MAX(x, y) (x > y ? x : y)



typedef struct sprite_t
{
	// Region to snip from
	unsigned int x, y;
	unsigned int w, h;
} sprite_t;

sprite_t sprites[MAX_SPR];
unsigned int spr_idx = 0;
pcx_t pcx_data;

ALLEGRO_DISPLAY *display;
ALLEGRO_BITMAP *main_buffer;
ALLEGRO_BITMAP *spr_buffer;
const char *spr_fname;

ALLEGRO_COLOR transparent_color;

int color_match(ALLEGRO_COLOR a, ALLEGRO_COLOR b)
{
	unsigned char r1, g1, b1;
	unsigned char r2, g2, b2;

	al_unmap_rgb(a, &r1, &g1, &b1);
	al_unmap_rgb(b, &r2, &g2, &b2);

	return (r1 == r2 && g1 == g2 && b1 == b2);
}

void flip(void)
{
	al_set_target_backbuffer(display);
	al_draw_bitmap(main_buffer, 0, 0, 0);
	al_flip_display();
	al_set_target_bitmap(main_buffer);
}

int area_is_empty(int x, int y, int x2, int y2)
{
	x = MAX(x, 0);
	y = MAX(y, 0);
	x2 = MIN(x2, SPR_W);
	y2 = MIN(y2, SPR_H);
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

int init(void)
{
	if (!al_init())
	{
		fprintf(stderr, "Couldn't initialize Allegro 5.\n");
		return 0;
	}
	if (!al_init_image_addon())
	{
		fprintf(stderr, "Couldn't load image addon.\n");
		return 0;
	}
	if (!al_init_primitives_addon())
	{
		fprintf(stderr, "Couldn't initialize primitives addon.\n");
		return 0;
	}
	
	al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
	display = al_create_display(SPR_W, SPR_H);
	if (!display)
	{
		fprintf(stderr, "Couldn't create display.\n");
		return 0;
	}
	main_buffer = al_create_bitmap(SPR_W, SPR_H);
	if (!main_buffer)
	{
		fprintf(stderr, "Couldn't create main buffer,.\n");
		al_destroy_display(display);
		return 0;
	}
	
	al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
	spr_buffer = al_load_bitmap(spr_fname);
	if (!spr_buffer)
	{
		fprintf(stderr, "Couldn't load sprite `%s`.\n", spr_fname);
		al_destroy_bitmap(main_buffer);
		al_destroy_display(display);
		return 0;
	}
	printf("Initialized.\n");
	al_set_target_bitmap(main_buffer);
	return 1;
}

void snip_sprite(int x, int y, int w, int h)
{
	printf("Spr $%X: (%d, %d) --> (%d, %d)\n", spr_idx, x, y, w, h);
	sprites[spr_idx].x = x;
	sprites[spr_idx].y = y;
	sprites[spr_idx].w = w;
	sprites[spr_idx].h = h;
	
	al_draw_filled_rectangle(x, y, x + w, y + h, al_map_rgb(0,0,0));
	for (int i = 0; i < 35; i++)
	{
		flip();
	}

	al_set_target_bitmap(spr_buffer);
	al_draw_filled_rectangle(x, y, x + w, y + h, transparent_color);
	al_set_target_bitmap(main_buffer);


	al_draw_bitmap(spr_buffer, 0, 0, 0);
	flip();
	spr_idx++;
}

void claim(void)
{
	unsigned int orig_x, orig_y;
	unsigned int w, h;
	// First, find topmost line to have data taken from it
	printf("Finding top line\n");
	for (orig_y = 0; orig_y < SPR_H; orig_y += 1)
	{
		if (!area_is_empty(0, orig_y, SPR_W, orig_y+1))
		{
			printf("Spr $%X: Top at %d\n", spr_idx, orig_y);
			break;
		}
	}

	orig_x = 0;
	// Now that the left is found, eat away the row
	do
	{
		h = 32;
		w = 32;
		printf("Finding left side\n");
		for (orig_x; orig_x < SPR_W; orig_x++)
		{
			al_set_target_bitmap(main_buffer);
			if (!area_is_empty(orig_x, orig_y, orig_x+1, orig_y+h))
			{
				printf("Spr $%X: Left at %d\n", spr_idx, orig_x);
				break;
			}
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
			snip_sprite(orig_x, ycopy, w, h);
		}
		orig_x += w;
	}
	while (!area_is_empty(orig_x, orig_y, SPR_W, orig_y + h));
}

void place_sprites(void)
{
	// Get transparent color from top-left of the image
	transparent_color = al_get_pixel(spr_buffer, 0, 0);

	while (!area_is_empty(0, 0, SPR_W, SPR_H))
	{
		claim();
	}
}

// Cut an 8x8 cell out of PCX data
void pcx_make_tile(unsigned int x, unsigned int y, pcx_t *pcx, FILE *f)
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

// Create binary sprite data file from PCX choppings
void dump_sprite(sprite_t *spr, FILE *f)
{
	for (int x = 0; x < spr->w; x += 8)
	{
		for (int y = 0; y < spr->h; y += 8)
		{
			printf("(%d,%d) --> (%d, %d)\n", spr->x, spr->y, spr->w, spr->h);
			pcx_make_tile(spr->x+x, spr->y+y, &pcx_data, f);
		}
	}
}

void create_spritedata(FILE *f)
{
	for (unsigned int i = 0; i < spr_idx; i++)
	{
		printf("Dumping sprite data %X\n", i);
		dump_sprite(&sprites[i], f);
	}
}

int main(int argc, char **argv)
{
	printf("Genesis metasprite creator\n");
	if (argc < 3)
	{
		printf("Usage: %s <mysprite.pcx> <outname>\n", argv[0]);
		return 0;
	}

	spr_fname = argv[1];
	if (!init())
	{
		return 0;
	}

	printf("Placing sprites\n");
	place_sprites();
	al_destroy_bitmap(spr_buffer);
	printf("Reading PCX data\n");
	pcx_new(&pcx_data, argv[1]);
	printf("Done.\n");

	char *outname_bin = (char *)malloc(sizeof(char) * (strlen(argv[2] + 4)));
	sprintf(outname_bin, "%s.bin", argv[2]);

	char *outname_dat = (char *)malloc(sizeof(char) * (strlen(argv[2] + 4)));
	sprintf(outname_dat, "%s.dat", argv[2]);

	FILE *fout = fopen(outname_bin, "wb");
	create_spritedata(fout);
	fclose(fout);

	free(outname_bin);
	free(outname_dat);
	return 0;
}
