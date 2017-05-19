#ifndef PCX_H_
#define PCX_H_

typedef struct {
  uint8_t bpp;
  uint16_t w, h;
  size_t stride;
  uint8_t pal16[3*16];
  uint8_t *data;
  uint8_t *pal;
} PcxFile;

int pcx_new(PcxFile *pcx, const char *fname);
int pcx_save(PcxFile *pcx, const char *fname);

#endif
