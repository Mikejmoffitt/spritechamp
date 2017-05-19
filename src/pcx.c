// Yanked straight out of trptool

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

//#include "misc.h"
#include "pcx.h"

#define A8_16(arr,idx) (arr[idx] | (arr[idx+1] << 8))

static void prv_read_data(PcxFile *pcx, FILE *fp) {
  size_t ptr = 0;
  for(size_t y = 0; y < pcx->h; y++) {
    size_t x = 0;
    while(x < pcx->w) {
      uint8_t byte;
      uint8_t run = 1;
      fread(&byte, 1, 1, fp);
      if((byte & 0xC0) == 0xC0) {
        run = byte & 0x3F;
        fread(&byte, 1, 1, fp);
      }
      memset(pcx->data+ptr, byte, run);
      ptr += run;
      x += run;
    }
  }
}

static void prv_dump_one_code(FILE *fp, uint8_t byte, uint8_t run) {
  if(run >= 0x40)
    run = 0x3F;
  if(run == 1 && byte < 0xC0) {
    fwrite(&byte, 1, 1, fp);
  }else{
    run |= 0xC0;
    fwrite(&run, 1, 1, fp);
    fwrite(&byte, 1, 1, fp);
  }
}

static void prv_write_data(PcxFile *pcx, FILE *fp) {
  size_t ptr = 0;
  for(size_t y = 0; y < pcx->h; y++) {
    uint8_t byte = pcx->data[ptr];
    uint8_t run = 0;
    for(size_t x = 0; x < pcx->w; x++, ptr++) {
      if(pcx->data[ptr] == byte) {
        run++;
        if(run < 0x40)
          continue;
      }
      prv_dump_one_code(fp, byte, run);
      byte = pcx->data[ptr];
      run = 1;
    }
    prv_dump_one_code(fp, byte, run);
  }
}

static void prv_read_8bpp_pal(PcxFile *pcx, FILE *fp) {
  uint8_t byte;
  fseek(fp, -769, SEEK_END);
  fread(&byte, 1, 1, fp);
  if(byte != 0xC)
    return;
  pcx->pal = malloc(768);
  fread(pcx->pal, 3, 256, fp);
}

int pcx_new(PcxFile *pcx, const char *fname) {
  FILE *fp = fopen(fname, "rb");
  *pcx = (PcxFile){0};

  uint8_t head[16];
  fread(head, 16, 1, fp);
  if(head[0] != 0x0A) {
    fprintf(stderr,"Bad PCX file: ID\n");
    return 0;
  }
  if(head[1] != 0x05) {
    fprintf(stderr,"Bad PCX file: only v3.0 supported\n");
    return 0;
  }
  if(head[2] != 0x01) {
    fprintf(stderr,"Bad PCX file: encoding\n");
    return 0;
  }
  pcx->bpp = head[3];
  if(pcx->bpp != 8) {
    fprintf(stderr,"Bad PCX file: %d BPP unsupported\n", pcx->bpp);
    return 0;
  }
  pcx->w = A8_16(head, 8) - A8_16(head, 4) + 1;
  pcx->h = A8_16(head,10) - A8_16(head, 6) + 1;
  pcx->stride = pcx->w * pcx->bpp / 8;

  fread(pcx->pal16, 3, 16, fp);
  fseek(fp, 64, SEEK_CUR); // Don't care

  pcx->data = calloc(pcx->w * pcx->bpp / 8, pcx->h);
  prv_read_data(pcx, fp);

  if(pcx->bpp == 8)
    prv_read_8bpp_pal(pcx, fp);

  return 1;
}

int pcx_save(PcxFile *pcx, const char *fname) {
  FILE *fp = fopen(fname, "wb");
  uint8_t head[16];
  head[ 0] = 0x0A;
  head[ 1] = 0x05;
  head[ 2] = 0x01;
  head[ 3] = pcx->bpp;
  head[ 4] = head[5] = 0;
  head[ 6] = head[7] = 0;
  head[ 8] = (pcx->w - 1) & 0xFF;
  head[ 9] = (pcx->w - 1) >> 8;
  head[10] = (pcx->h - 1) & 0xFF;
  head[11] = (pcx->h - 1) >> 8;
  head[12] = 16; head[13] = 0;
  head[14] = 144; head[15] = 0;
  fwrite(head, 16, 1, fp);
  fwrite(pcx->pal16, 3, 16, fp);
  head[0] = 0;
  head[1] = 1;
  head[2] = pcx->w & 0xFF;
  head[3] = pcx->w >> 8;
  head[4] = 0x75; head[5] = 0x34;
  head[6] = 0; head[7] = 0;
  head[8] = 0xC0; head[9] = 0xFC;
  fwrite(head, 10, 1, fp);
  // Fill remainder of header
  memset(head, 0, 16);
  fwrite(head, 6, 1, fp);
  fwrite(head, 16, 1, fp);
  fwrite(head, 16, 1, fp);
  fwrite(head, 16, 1, fp);

  prv_write_data(pcx, fp);

  if(pcx->bpp == 8) {
    uint8_t c = 0xC;
    fwrite(&c, 1, 1, fp);
    fwrite(pcx->pal, 3, 256, fp);
  }

  return 1;
}
