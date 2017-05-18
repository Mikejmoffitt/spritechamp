#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>

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

void claim(void)
{
	unsigned int orig_x, orig_y;
	unsigned int w, h;
	// First, find topmost line to have data taken from it
	printf("Finding top line\n");
	for (orig_y = 0; orig_y < SPR_H; orig_y++)
	{
		if (!area_is_empty(0, orig_y, SPR_W, orig_y+1))
		{
			printf("Spr $%X: Top at %d\n", spr_idx, orig_y);
			break;
		}
	}

	// Now, seek rightwards until the left side is found
	printf("Finding left side\n");
	for (orig_x = 0; orig_x < SPR_W; orig_x++)
	{
		if (!area_is_empty(orig_x, orig_y, orig_x+1, orig_y+1+32))
		{
			printf("Spr $%X: Left at %d\n", spr_idx, orig_x);
			break;
		}
	}

	// TODO: Do this in a less stupid way
	// How far to the right should we reach?
	h = 32;
	w = 32;
	while (w > 8)
	{
		if (area_is_empty(orig_x, orig_y, orig_x+1+w, orig_y+1+32))
		{
			w -= 1;
		}
		else
		{
			break;
		}
	}
	w = (w+7) & 0xFFF8;
	printf("Spr $%X: Width is %d\n", spr_idx, w);

	printf("Spr $%X: (%d, %d)\n", spr_idx, orig_x, orig_y);

	al_set_target_bitmap(spr_buffer);
	al_draw_filled_rectangle(orig_x + 0.1, orig_y + 0.1, orig_x + w + 0.1, orig_y + h + 0.1, transparent_color);
	al_set_target_bitmap(main_buffer);


	al_draw_bitmap(spr_buffer, 0, 0, 0);
	for (int i = 0; i < 30; i++)
	{
		flip();
	}
}

void place_sprites(void)
{
	// Get transparent color from top-left of the image
	transparent_color = al_get_pixel(spr_buffer, 0, 0);

	if (area_is_empty(0, 0, 8, 8))
	{
		printf("Top left is empty\n");
	}
	if (area_is_empty(0, 0, 256, 128))
	{
		printf("Whole image is empty\n");
	}
	while (!area_is_empty(0, 0, SPR_W, SPR_H))
	{
		claim();
	}
}

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		printf("Usage: %s <mysprite.pcx>\n", argv[0]);
		return 0;
	}

	spr_fname = argv[1];
	if (!init())
	{
		return 0;
	}

	place_sprites();

	while(1)
	{
		al_draw_bitmap(spr_buffer, 0, 0, 0);
		flip();
	}

	return 0;
}
