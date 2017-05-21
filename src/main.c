#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <pthread.h>
#include "pcx.h"
#include "types.h"
#include "snipping.h"
#include "pcx_proc.h"


/* Configuration */


ALLEGRO_DISPLAY *display;
ALLEGRO_BITMAP *main_buffer;

#define DISP_W 256
#define DISP_H 128

#ifdef DEBUG_VIS
void flip(void)
{
	al_set_target_backbuffer(display);
	al_draw_bitmap(main_buffer, 0, 0, 0);
	al_flip_display();
	al_set_target_bitmap(main_buffer);
}
#endif

// TODO: Now that the idea works allegro should be ripped out
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
#ifdef DEBUG_VIS
	al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
	display = al_create_display(DISP_W, DISP_H);
	if (!display)
	{
		fprintf(stderr, "Couldn't create display.\n");
		return 0;
	}
	main_buffer = al_create_bitmap(DISP_W, DISP_H);
	if (!main_buffer)
	{
		fprintf(stderr, "Couldn't create main buffer,.\n");
		al_destroy_display(display);
		return 0;
	}
	al_set_target_bitmap(main_buffer);
	al_clear_to_color(al_map_rgb(0,0,0));
#endif

	// printf("Initialized.\n");
	return 1;
}
// Dump all sprite metadata.
void write_metadata(FILE *f, sprite_t *sprites, unsigned int w, unsigned int h)
{
	// This goes up as more files are processed, so it is static.
	static uint32_t tile_idx = 0;
	pak_entry_t pak;
	// TODO: This is just for debugging, remove it later
	pak.id[0] = 'P';
	pak.id[1] = 'A';
	pak.id[2] = 'K';
	pak.id[3] = '\0';
	for (unsigned int i = 0; i < MAX_SPR; i++)
	{
		sprite_t *spr = &sprites[i];

		// Calculate sprite entry offsets
		pak.x[i] = spr->x - (w/2);
		pak.y[i] = spr->y - h + 1;
		
		// Convert tile index to big endian
		uint8_t *data_idx = (uint8_t *)&tile_idx;
		pak.data_idx[0] = data_idx[3];
		pak.data_idx[1] = data_idx[2];
		pak.data_idx[2] = data_idx[1];
		pak.data_idx[3] = data_idx[0];

		// Store size, or end flag, in MD format-ish
		if (spr->w == 0 || spr->h == 0)
		{
			pak.size[i] = PAK_SIZE_INVAL;
		}
		else
		{
			pak.size[i] = (((spr->w / 8)-1) << 2) + ((spr->h / 8)-1);
			tile_idx += pak.size[i];
		}

		// Dump pak to meta file
		fwrite(&pak, sizeof(pak_entry_t), 1, f);
	}
}

int main(int argc, char **argv)
{
	if (argc < 3)
	{
		printf("Usage: %s <metafname> <tilefname> <files...>\n", argv[0]);
		return 0;
	}

	if (!init())
	{
		return 0;
	}

	unsigned int numfiles = argc - 3;

	// Set up file handles
	FILE *fmeta_out;
	fmeta_out = fopen(argv[1], "wb");
	if (!fmeta_out)
	{
		fprintf(stderr, "Couldn't open %s for writing!\n", argv[1]);
		return 1;
	}
	FILE *ftile_out;
	ftile_out = fopen(argv[2], "wb");
	if (!ftile_out)
	{
		fprintf(stderr, "Couldn't open %s for writing!\n", argv[2]);
		return 1;
	}

	// printf("Meta: %p Tile: %p\n", fmeta_out, ftile_out);

	// process each file
	printf("Files to pack: %d\n", numfiles);
	for (unsigned int i = 0; i < numfiles; i++)
	{
		const char *spr_fname = argv[i+3];
		pcx_t pcx_data;
		sprite_t sprites[MAX_SPR];
		printf("[ \"%s\"  - #%d]\n", spr_fname, i);
		
		// 0) Load PCX file
		printf("# Loading PCX data...\n");
		pcx_new(&pcx_data, spr_fname);

		// 1) Load sprite bitmap, determine sprite cuts
		printf("# Cutting sprites from bitmap\n");
		place_sprites(spr_fname, sprites);

		// 2) Dump pak header info to meta file
		printf("# Writing metadata\n" );
		write_metadata(fmeta_out, sprites, pcx_data.w, pcx_data.h);

		// 3) Dump pak tile data to tile file
		printf("# Dumping pak tile data\n");
		pcx_dump_tiledata(&pcx_data, sprites, ftile_out);

		printf("# Freeing PCX\n");
		// 4) Free dynamic memory from PCX file
		pcx_destroy(&pcx_data);
		printf("\n");
	}

	printf("Finished.\n");
	fclose(fmeta_out);
	fclose(ftile_out);
	return 0;
}
