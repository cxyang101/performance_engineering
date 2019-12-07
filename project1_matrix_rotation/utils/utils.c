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

#include "./utils.h"
#include <string.h>

// Calculates the number of bytes required to hold `nbits` bits
inline bytes_t bits_to_bytes(bits_t nbits) {
  return (nbits + 7) / 8;
}

// Gets the bit value at position (`i`, `j`). The origin is the top left
//
// The `row_size` are the number of bytes per row in `img`
uint8_t get_bit(uint8_t *img, const bytes_t row_size, uint32_t i, uint32_t j) {
  uint64_t byte_offset = j * row_size + (i / 8);
  uint8_t byte_mask = 0b10000000 >> (i % 8);

  uint8_t *img_byte = img + byte_offset;

  return (*img_byte & byte_mask) != 0;
}

// Sets the bit at position (`i`, `j`) to `value`. The origin is the top left
//
// The `row_size` are the number of bytes per row in `img`
void set_bit(uint8_t *img, const bytes_t row_size, uint32_t i, uint32_t j, uint8_t value) {
  // Sanity check the input
  assert(value == 0 || value == 1);

  uint64_t byte_offset = j * row_size + (i / 8);
  uint8_t byte_mask = 0b10000000 >> (i % 8);

  uint8_t *img_byte = img + byte_offset;

  if (value) {
    // Or in the bit set in `byte_mask`
    *img_byte |= byte_mask;
  } else {
    // Mask out the bit set in `byte_mask`
    *img_byte &= ~byte_mask;
  }

  return;
}

void print_bit_matrix(uint8_t *bit_matrix, const bits_t N, int32_t ncolumns) {
  bytes_t nbytes = bits_to_bytes(N);

  uint32_t dimension = ncolumns < 0 ? N : ncolumns;

  uint32_t j;
  for (j = 0; j < N; j++) {
    uint32_t i;
    for (i = 0; i < dimension; i++) {
      uint8_t bit_val = get_bit(bit_matrix, nbytes, i, j);

      printf("%c ", bit_val ? '1' : '0');
    }

    printf("\n");
  }

  return;
}

uint8_t *generate_bit_matrix(const bits_t N, bool suppress_error) {
  // Sanity check the input
  assert(N > 0);
  assert(!(N % 64));

  bytes_t nbytes = bits_to_bytes(N);

  uint8_t *ret;
  ret = malloc(nbytes * N);
  if (!ret) {
    if (!suppress_error)
        printf("Error: Run out of heap space! Please try smaller matrix size.\n");
    return NULL;
  }

  uint64_t i;
  uint64_t* pt = (uint64_t *)ret;
  
  // seed the rand function with a random seed
  srand(time(0));
  uint64_t scrambled = (((uint64_t) rand()) << 32) | rand();
  for (i = 0; i < (nbytes * N / 8); i++) {
     *(pt + i) = scrambled;
     scrambled = scrambled*(2*scrambled + 1);
     scrambled = scrambled*(2*scrambled + 1);
     scrambled = (scrambled << 32) | (scrambled >> 32);
  }

  return ret;
}

uint8_t *copy_bit_matrix(uint8_t *bit_matrix, const bits_t N) {
  // Sanity check the input
  assert(N > 0);
  assert(!(N % 64));

  bytes_t nbytes = bits_to_bytes(N);

  uint8_t *ret;
  ret = malloc(nbytes * N);
  if (!ret) {
    printf("Error: Run out of heap space! Please try smaller matrix size.\n");
    assert(false);
  }

  // Copy the `bit_matrix`
  memcpy(ret, bit_matrix, nbytes * N);

  return ret;
}
