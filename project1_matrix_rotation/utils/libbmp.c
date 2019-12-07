/**
 * Copyright (c) 2012 MIT License by 6.172 Staff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 **/

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <stdbool.h>
#include "./libbmp.h"

// Read the BMP headers and color tables
static bool read_headers(FILE *f, struct header_s *header,
                         struct info_header_s *info_header,
                         struct color_table_s color_tables[2]) {
  // Read the file header
  if (!fread(header, 1, sizeof(*header), f)) {
    goto bad;
  }

  // The signature "BM" for bitmap files
  assert(header->signature == 0x4D42);

  // Read the BMP info header
  if (!fread(info_header, 1, sizeof(*info_header), f)) {
    goto bad;
  }

  // Make sure this is a binary BMP image
  assert(info_header->bits_per_pixel == 1);

  // Make sure that this image is not compressed
  assert(info_header->compression == 0);

  // Seek to the 2 color tables
  fseek(f, sizeof(*header) + info_header->size, SEEK_SET);

  // Read the 2 color tables in this binary BMP image
  if (!fread(color_tables, 2, sizeof(struct color_table_s), f)) {
    goto bad;
  }

  return true;

  bad:
    // There was some sort of error
    return false;
}

// Reads the binary image from `fname` and saves the bit width and height
// in `_w` and `_h` respectively. Additionally saves the size of a single
// row in the image in bytes in `_row_size` and the 2 color tables used
// in the BMP file in `color_tables`
uint8_t *read_binary_bmp(const char *fname, int *_w, int *_h, int *_row_size,
                         struct color_table_s color_tables[2]) {
  // Sanity checks as per the BMP standard
  static_assert(sizeof(struct header_s) == 14, "Incorrect size of BMP file header struct");
  static_assert(sizeof(struct info_header_s) == 40, "Incorrect size of BMP info header struct");
  static_assert(sizeof(struct color_table_s) == 4, "Incorrect size of color table struct");

  // Read `fname`
  FILE *f = fopen(fname, "rb");

  // There was some sort of error
  if (!f) {
    perror("Error reading BMP file");
    return NULL;
  }

  // Read the BMP headers
  struct header_s header;
  struct info_header_s info_header;

  if (!read_headers(f, &header, &info_header, color_tables)) {
    // There was some sort of error
    perror("Error reading BMP headers");
    return NULL;
  }

  // If the height is negative, then the origin is the top-left of the image.
  // Otherwise the origin is the bottom-left
  int scan_dir = -1;
  uint32_t inverted = 1;
  if (info_header.height < 0) {
    scan_dir = 1;
    inverted = 0;
    info_header.height = -1 * info_header.height;
  }

  // Rows are aligned on 4-byte boundary
  int row_size = ((info_header.bits_per_pixel * info_header.width + 31) / 32) * 4;
  uint32_t image_size = row_size * info_header.height;

  uint8_t *ret_img = malloc(info_header.height * row_size);
  uint8_t *image_data = malloc(image_size);

  if (!ret_img || !image_data) {
    printf("Error: Image size is too large to fit in heap space!\n");
    assert(false);
  }

  // Offset to and read the image data
  fseek(f, header.data_offset, SEEK_SET);
  if (!fread(image_data, 1, image_size, f)) {
    // There was some sort of error
    assert(false);
  }

  uint8_t *image_offset = image_data + inverted * ((info_header.height - 1) * row_size);
  uint8_t *ret_img_offset = ret_img;
  uint32_t h;
  for (h = 0; h < info_header.height; h++) {
    memcpy(ret_img_offset, image_offset, row_size);

    image_offset += scan_dir * row_size;
    ret_img_offset += row_size;
  }

  free(image_data);

  fclose(f);

  // Set the return dimension values
  *_w = info_header.width;
  *_h = info_header.height;
  *_row_size = row_size;

  return ret_img;
}

static void init_header(struct header_s *header,
       const uint32_t file_size, const uint32_t data_offset) {
  // The signature "BM" for bitmap files
  header->signature = 0x4D42;

  // Se the other fields accordingly
  header->file_size = file_size;
  header->reserved = 0;
  header->data_offset = data_offset;

  return;
}

// Initializes the info header of a binary image with dimensions `N` by `N` bits
static void init_info_header(struct info_header_s *info_header, const uint32_t N) {
  // Set the size of the `info_header`
  info_header->size = sizeof(struct info_header_s);
  assert(info_header->size == 40);

  // Set the dimensions of the bitmap
  info_header->width = info_header->height = N;

  // Number of planes is always 1
  info_header->planes = 1;

  // For binary images, `bits_per_pixel` is 1
  info_header->bits_per_pixel = 1;

  // There is no image compression
  info_header->compression = 0;
  info_header->image_size = 0;

  // The X and Y pixels per meter are hard-coded to 2835
  info_header->X_pixels_per_M = info_header->Y_pixels_per_M = 2835;

  // In a binary image, only 2 colors are used
  info_header->colors_used = 2;

  // All colors are important
  info_header->important_colors = 0;

  return;
}

// Write the binary `image_data` encoding an image `N` by `N` bits to `output_fname`.
//
// The output image will use the 2 color tables supplied. Bits set to 0 will use the
// color in the 0th color table and likewise bits set to 1 will use the 1st color table
void write_binary_bmp(const char *output_fname, uint8_t *image_data,
                      struct color_table_s color_tables[2],
                      const uint32_t N) {
  // Sanity checks as per the BMP standard
  static_assert(sizeof(struct header_s) == 14, "Incorrect size of BMP file header struct");
  static_assert(sizeof(struct info_header_s) == 40, "Incorrect size of BMP info header struct");
  static_assert(sizeof(struct color_table_s) == 4, "Incorrect size of color table struct");

  // For now, writes will only support 1-byte aligned images
  assert(N > 0);
  assert(!(N % 8));

  struct header_s header;
  struct info_header_s info_header;

  // Create a file `output_fname` if necessary
  FILE *f = fopen(output_fname, "wb+");

  // There was some sort of error
  if (!f) {
    perror("Error writing BMP file");
    return;
  }

  // First set the `info_header` accordingly
  init_info_header(&info_header, N);

  //
  // Write the `image_data`
  //

  // Seek past all of the metadata to start writing the `image_data`
  const uint32_t data_offset = sizeof(header) + sizeof(info_header) + 2 * sizeof(color_tables[0]);
  fseek(f, data_offset, SEEK_SET);

  // Some useful constants for writing rows to the file
  const uint32_t row_size = N / 8;
  uint8_t *image_data_offset = image_data + (N - 1) * row_size;

  // Have an array of 0's to pad each row to a 4-byte alignment as per the BMP file format
  const uint32_t npad = row_size & 0b11;
  uint8_t zeros[3] = {0};

  // The `image_data` gets traversed from bottom to top since our `height`
  // is positive and as per the definition of the BMP file format
  uint32_t i;
  for (i = 0; i < N; i++) {
    // Write the row to `f`
    fwrite(image_data_offset, 1, row_size, f);

    // Fill the rest of the row with 0's to make it 4-byte aligned
    fwrite(zeros, 1, npad, f);

    // Decrement the `image_data_offset` for the next row
    image_data_offset -= row_size;
  }

  // After writing the image data, set the file `header`

  // Get the `file_size`
  fseek(f, 0, SEEK_END);
  int32_t file_size = ftell(f);

  // Sanity check for errors
  assert(file_size >= 0);

  init_header(&header, file_size, data_offset);

  //
  // Write the file metadata
  //

  // Seek back to the beginning
  fseek(f, 0, SEEK_SET);

  // Write the respective headers and such
  fwrite(&header, 1, sizeof(header), f);
  fwrite(&info_header, 1, sizeof(info_header), f);

  // The 0th color table is the color of bits that are 0's
  // and the 1st is for the bits that are 1's
  fwrite(&color_tables[0], 1, sizeof(color_tables[0]), f);
  fwrite(&color_tables[1], 1, sizeof(color_tables[1]), f);

  // Close the file once finished!
  fclose(f);

  return;
}
