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

#ifndef LIBBMP_H
#define LIBBMP_H

#include <stdint.h>

// BMP standard read from:
//  http://www.ece.ualberta.ca/~elliott/ee552/studentAppNotes/2003_w/misc/bmp_file_format/bmp_file_format.htm
//  https://en.wikipedia.org/wiki/BMP_file_format
struct header_s {
  uint16_t signature;
  uint32_t file_size;
  uint32_t reserved;
  uint32_t data_offset;
} __attribute__((packed));

struct info_header_s {
  uint32_t size;
  uint32_t width;
  uint32_t height;
  uint16_t planes;
  uint16_t bits_per_pixel;
  uint32_t compression;
  uint32_t image_size;
  uint32_t X_pixels_per_M;
  uint32_t Y_pixels_per_M;
  uint32_t colors_used;
  uint32_t important_colors;
} __attribute__((packed));

struct color_table_s {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t reserved;
} __attribute__((packed));


uint8_t *read_binary_bmp(const char *fname, int *_w, int *_h, int *_row_size,
                         struct color_table_s color_tables[2]);

void write_binary_bmp(const char *output_fname, uint8_t *image_data,
                      struct color_table_s color_tables[2],
                      const uint32_t N);

#endif  // LIBBMP_H
