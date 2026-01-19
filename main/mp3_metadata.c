/**
 * @file mp3_metadata.c
 * @brief MP3 metadata reading implementation.
 *
 * Parses ID3v2 tags from MP3 files to extract title, artist, and album
 * information.
 */

#include "mp3_metadata.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Convert syncsafe integer (ID3v2.3 style).
 *
 * @param b Pointer to 4 bytes.
 * @return Decoded 32-bit integer.
 */
static uint32_t syncsafe(uint8_t *b) {
  return (b[0] << 21) | (b[1] << 14) | (b[2] << 7) | b[3];
}

/**
 * @brief Convert syncsafe integer (ID3v2.4 style).
 *
 * @param b Pointer to 4 bytes.
 * @return Decoded 32-bit integer.
 */
static uint32_t syncsafe32(uint8_t *b) {
  return ((b[0] & 0x7F) << 21) | ((b[1] & 0x7F) << 14) | ((b[2] & 0x7F) << 7) |
         (b[3] & 0x7F);
}

/**
 * @brief Read text data from ID3 frame.
 *
 * Handles different text encodings (ISO-8859-1, UTF-8, UTF-16).
 *
 * @param f File pointer.
 * @param size Size of the text data.
 * @param out Output buffer.
 * @param max Maximum size of output buffer.
 */
static void read_text(FILE *f, uint32_t size, char *out, int max) {
  if (max < 2) {
    out[0] = 0;
    return;
  }

  uint8_t enc;
  if (fread(&enc, 1, 1, f) != 1) {
    out[0] = 0;
    return;
  }
  size--;

  if (size == 0) {
    out[0] = 0;
    return;
  }

  int o = 0;

  /* ===== ISO-8859-1 / UTF-8 ===== */
  if (enc == 0 || enc == 3) {
    if (size >= (uint32_t)max)
      size = max - 1;

    size_t read = fread(out, 1, size, f);
    out[read] = 0;
    return;
  }

  /* ===== UTF-16 (with BOM) ===== */
  if (enc == 1 || enc == 2) {
    uint8_t bom[2] = {0};
    fread(bom, 1, 2, f);
    size -= 2;

    bool le = true;
    if (bom[0] == 0xFE && bom[1] == 0xFF)
      le = false;

    while (size >= 2 && o < max - 1) {
      uint8_t c[2];
      fread(c, 1, 2, f);
      size -= 2;

      uint16_t ch = le ? (c[0] | (c[1] << 8)) : (c[1] | (c[0] << 8));

      if (ch == 0)
        break;

      /* ASCII only (lossy) */
      out[o++] = (ch < 128) ? ch : '?';
    }

    out[o] = 0;
    return;
  }

  /* Unknown encoding */
  out[0] = 0;
}

/**
 * @brief Parse ID3v2 tags from the MP3 file.
 *
 * Reads and parses ID3v2.3 or ID3v2.4 tags to extract metadata.
 *
 * @param f File pointer to the MP3 file.
 * @param meta Pointer to the metadata structure to fill.
 */
static void parse_id3(FILE *f, mp3_metadata_t *meta) {
  uint8_t hdr[10];
  fread(hdr, 1, 10, f);

  if (memcmp(hdr, "ID3", 3)) {
    fseek(f, 0, SEEK_SET);
    return;
  }

  uint8_t ver = hdr[3]; // 3 = v2.3, 4 = v2.4

  uint32_t tag_size = syncsafe(&hdr[6]);
  uint32_t read = 0;

  while (read < tag_size) {
    uint8_t fh[10];
    char id[5] = {0};

    if (fread(fh, 1, 10, f) != 10)
      break;

    memcpy(id, fh, 4);

    if (fh[0] == 0)
      break;

    uint32_t sz = (ver == 4)
                      ? syncsafe32(&fh[4]) // ✅ ID3v2.4
                      : ((fh[4] << 24) | (fh[5] << 16) | (fh[6] << 8) | fh[7]);

    read += 10 + sz;

    if (sz == 0)
      break;

    if (!strcmp(id, "TIT2"))
      read_text(f, sz, meta->title, sizeof(meta->title));
    else if (!strcmp(id, "TPE1"))
      read_text(f, sz, meta->artist, sizeof(meta->artist));
    else if (!strcmp(id, "TALB"))
      read_text(f, sz, meta->album, sizeof(meta->album));
    else
      fseek(f, sz, SEEK_CUR);
  }
}

/**
 * @brief Read metadata from an MP3 file.
 *
 * Opens the file, parses ID3 tags, and fills the metadata structure.
 *
 * @param path Path to the MP3 file.
 * @param meta Pointer to the metadata structure to fill.
 * @return true on success, false on failure.
 */
bool mp3_read_metadata(const char *path, mp3_metadata_t *meta) {
  memset(meta, 0, sizeof(*meta));

  FILE *f = fopen(path, "rb");
  if (!f)
    return false;

  parse_id3(f, meta);
  fclose(f);
  return true;
}
