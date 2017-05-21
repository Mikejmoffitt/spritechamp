#include <stdio.h>
#include <stdlib.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <string.h>
#include <unistd.h>
#include "pcx.h"
#include "types.h"
#include "snipping.h"
#include "pcx_proc.h"


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
void write_metadata(FILE *f, sprite_t *sprites, unsigned int w,
                    unsigned int h, unsigned int pak_count)
{
	// Tile position throughout cahracter, across entire binary
	// This goes up as more files are processed, so it is static.
	static uint16_t tile_idx = 0;

	// tile count local to one metasprite
	uint16_t tilecount = 0;

	pak_entry_t pak;
	pak.numtiles[0] = 0;
	pak.numtiles[1] = 0;
	
	// Record starting tile number for this metasprite
	uint8_t *tile_idx_b = (uint8_t *)&tile_idx;
	pak.tile_idx[0] = tile_idx_b[1];
	pak.tile_idx[1] = tile_idx_b[0];

	// Tile data source within binary blob
	uint32_t tile_loc_adjusted = (sizeof(pak_entry_t) * pak_count) + (tile_idx * 32);
	uint8_t *data_idx = (uint8_t *)&tile_loc_adjusted;
	pak.data_idx[0] = data_idx[3];
	pak.data_idx[1] = data_idx[2];
	pak.data_idx[2] = data_idx[1];
	pak.data_idx[3] = data_idx[0];

	// For each sprite within the metasprite:
	for (unsigned int i = 0; i < MAX_SPR; i++)
	{
		sprite_t *spr = &sprites[i];

		// Calculate sprite entry offsets
		pak.x[i] = spr->x - (w/2);
		pak.y[i] = spr->y - h + 1;

		// Store size, or end flag, in MD format-ish
		if (spr->w == 0 || spr->h == 0)
		{
			pak.size[i] = PAK_SIZE_INVAL;
		}
		else
		{
			pak.size[i] = (((spr->w / 8)-1) << 2) + ((spr->h / 8)-1);
			tile_idx += (spr->w / 8) * (spr->h / 8);
			tilecount += (spr->w / 8) * (spr->h / 8);
		}
	}

	pak.numtiles[0] = (tilecount & 0xFF00) >> 8;
	pak.numtiles[1] = tilecount & 0xFF;

	// Dump pak to meta file
	fwrite(&pak, sizeof(pak_entry_t), 1, f);
}

int main(int argc, char **argv)
{
	if (argc < 3)
	{
		printf("Usage: %s <metafname> <tilefname> <files...>\n {-n}", argv[0]);
		printf("       PCX filenames should be formatted as  NNN_xxx.pcx\n");
		return 0;
	}
	unsigned int numfiles = argc - 3;

	// Recount number of files by iterating through filenames and keeping
	// the leading number from the highest one
	for (unsigned int i = 0; i < argc - 3; i++)
	{
		char fname_cpy[256];
		strncpy(fname_cpy, argv[i+3], 256);
		// Find PCX filename without leading dir
		char *ret_str = strtok(fname_cpy, "/\\");
		char *ret_chk = ret_str;
		while (ret_chk != NULL)
		{
			ret_chk = strtok(NULL, "/\\");
			if (ret_chk != NULL)
			{
				ret_str = ret_chk;
			}
		}
		printf("Got filename: %s\n", ret_str);

		ret_chk = strtok(ret_str, "_");
		if (strtol(ret_chk, NULL, 16)+1 > numfiles)
		{
			numfiles = strtol(ret_chk, NULL, 16) + 1;
		}
		printf("%s\n", ret_chk);
	}

	if (!init())
	{
		return 0;
	}

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

	unsigned int fname_idx = 0;

	printf("Files to pack: %d\n", numfiles);
	for (unsigned int i = 0; i < numfiles; i++)
	{
		pcx_t pcx_data;
		sprite_t sprites[MAX_SPR];
		const char *spr_fname = argv[fname_idx+3];

		// -1) Check if skip are needed
		char fnamecpy[256];
		strncpy(fnamecpy, spr_fname, 256);
		// Find PCX filename without leading dir
		char *ret_str = strtok(fnamecpy, "/\\");
		char *ret_chk = ret_str;
		while (ret_chk != NULL)
		{
			ret_chk = strtok(NULL, "/\\");
			if (ret_chk != NULL)
			{
				ret_str = ret_chk;
			}
		}

		ret_chk = strtok(ret_str, "_");
		if (strtol(ret_chk, NULL, 16) > i)
		{
			// Make a dummy metasprite, mark as invalid
			sprite_t dummy[MAX_SPR];
			for (unsigned int j = 0; j < MAX_SPR; j++)
			{
				dummy[j].w = 0;
				dummy[j].h = 0;
			}
			write_metadata(fmeta_out, dummy, 0, 0, numfiles);
			printf("[ DUMMY - #%d]\n", i);
			continue;
		}
		printf("[ \"%s\" - #%d]\n", spr_fname, i);
		fname_idx++;

		// 0) Load PCX file
		//printf("# Loading PCX data...\n");
		if (!pcx_new(&pcx_data, spr_fname))
		{
			fclose(fmeta_out);
			fclose(ftile_out);
			return 1;
		}

		// 1) Load sprite bitmap, determine sprite cuts
		//printf("# Cutting sprites from bitmap\n");
		place_sprites(spr_fname, sprites);

		// 2) Dump pak header info to meta file
		//printf("# Writing metadata\n");
		write_metadata(fmeta_out, sprites, pcx_data.w, pcx_data.h, numfiles);

		// 3) Dump pak tile data to tile file
		//printf("# Dumping pak tile data\n");
		pcx_dump_tiledata(&pcx_data, sprites, ftile_out);

		//printf("# Freeing PCX\n");
		// 4) Free dynamic memory from PCX file
		pcx_destroy(&pcx_data);
		//printf("\n");
	}

	printf("Finished.\n");
	fclose(fmeta_out);
	fclose(ftile_out);
	return 0;
}
