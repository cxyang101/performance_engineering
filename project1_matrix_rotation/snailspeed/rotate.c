
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

#include "../utils/utils.h"
#include <stdlib.h>
#include <inttypes.h>

void row_column_row(uint64_t *img, uint64_t* restrict C);
void rotate_columns(uint64_t *B, uint64_t* restrict scratch);
void byte_swap(uint64_t *a);
void print_bit_matrix(uint8_t *bit_matrix, const bits_t N, int32_t ncolumns); 

// Defining masks for row-column-row rotation
#define stay_mask6 0xAAAAAAAAAAAAAAAAull
#define stay_mask7 0x0000000000000000ull
#define stay_mask5 0xCCCCCCCCCCCCCCCCull
#define stay_mask4 0xF0F0F0F0F0F0F0F0ull
#define stay_mask3 0xFF00FF00FF00FF00ull
#define stay_mask2 0xFFFF0000FFFF0000ull
#define stay_mask1 0xFFFFFFFF00000000ull

void rotate_bit_matrix(uint8_t *img, const bits_t N) {

  const uint64_t row_size = (N+63)/64;
  // Define scratch spaces for block rotation
  uint64_t A[64];
  uint64_t B[64];
  uint64_t C[64];
  uint64_t D[64];
  uint64_t* restrict scratch = (uint64_t*) malloc(64*sizeof(uint64_t));

  uint64_t* img_64 = (uint64_t*) img; 

  if (N==64){
    uint64_t result[64];

    for (int k=0; k < 64; k++){
        *(result+k) = __builtin_bswap64(*(img_64+k*row_size));
    }
    
    row_column_row(result, scratch);

    for (int k=0; k < 64; k++){
        *(img_64+k*row_size) = __builtin_bswap64(*(result+k));
    }
    return;
  }

  // Look at all (i, j) in first quadrant
  bits_t big_N = N % 128 == 0 ? N : N+128;
  for (uint32_t j=0; j < big_N/128; j++) {
    for (uint32_t i=0; i < N/128; i++) {
      // Offset of a block in upper left quadrant
      uint64_t* offset_A = img_64 + 64*j*row_size + i;
      // Offset of a block in upper right quadrant
      uint64_t* offset_B = img_64 + 64*i*row_size + row_size-j-1;
      // Offset of a block in lower right quadrant
      uint64_t* offset_C = img_64 + 64*(row_size-j-1)*row_size + row_size-i-1;
      // Offset of a block in lower left quadrant
      uint64_t* offset_D = img_64 + 64*(row_size-i-1)*row_size + j;
      
      for (uint64_t k=0; k < 64; k++){
        A[k] = __builtin_bswap64(offset_A[k*row_size]);
        B[k] = __builtin_bswap64(offset_B[k*row_size]);
        C[k] = __builtin_bswap64(offset_C[k*row_size]);
        D[k] = __builtin_bswap64(offset_D[k*row_size]);
      }

      // Rotate the 64x64 matrix efficiently
      row_column_row(A, scratch);
      row_column_row(B, scratch);
      row_column_row(C, scratch);
      row_column_row(D, scratch);

      // Displace first block to the second position,
      // second to third position and so on
      for (int k=0; k<64; k++){
        offset_B[k*row_size] = __builtin_bswap64(A[k]);
        offset_C[k*row_size] = __builtin_bswap64(B[k]);
        offset_D[k*row_size] = __builtin_bswap64(C[k]);
        offset_A[k*row_size] = __builtin_bswap64(D[k]);
      }
    }
  }

  // Rotate middle block if we have odd number of 64x64
  // blocks per image side
  if (N/64 % 2 ==1) {
    int j = N/128;
    uint64_t* offset = img_64 + 64*j*row_size + j;
    for (int k=0; k < 64; k++){
      A[k] = __builtin_bswap64(offset[k*row_size]);
    }
    row_column_row(A, scratch);
    for (int k=0; k<64; k++){
      offset[k*row_size] = __builtin_bswap64(A[k]);
    }
  }
  free(scratch);
}
    
void row_column_row(uint64_t *img, uint64_t* restrict scratch){
  // First, rotate all rows to the left by their index + 1
  for (int i = 0; i < 64; i++){
    uint64_t curr_row = *(img + i);
    img[i] = (curr_row << (i+1) | (curr_row >> (63-i) ));  
  }

  // Next, rotate all columns
  rotate_columns(img, scratch);
 
  // Finally, rotate all rows to the left by their index
  for (int i = 0; i < 64; i++){
    uint64_t curr_row = *(img + i);
    img[i] = (curr_row <<(i)) | (curr_row >> (64-i));
  }
  return;
}

void rotate_columns(uint64_t *B, uint64_t* restrict C){
  // Rotate right half down by 32
  for (int j = 0; j < 32; j++){
    *(C+j) = (*(B+j) & stay_mask1) | (*(B+(j+32)) & ~stay_mask1);
  }
  for (int j = 32; j < 64; j++){
    *(C+j) = (*(B+j) & stay_mask1) | (*(B+(j-32)) & ~stay_mask1);
  }
  // Rotating every other group of 16 bits down by 1
  for (int j = 0; j < 16; j++){
    *(B+j) = (*(C+j) & stay_mask2) | (*(C+(j+48)) & ~stay_mask2);
  }
  for (int j = 16; j < 64; j++){
    *(B+j) = (*(C+j) & stay_mask2) | (*(C+(j-16)) & ~stay_mask2);
  }
  // Rotating every other group of 8 bits down by 8
  for (int j = 0; j < 8; j++){
    *(C+j) = (*(B+j) & stay_mask3) | (*(B+(j+56)) & ~stay_mask3);
  }
  for (int j = 8; j < 64; j++){
    *(C+j) = (*(B+j) & stay_mask3) | (*(B+(j-8)) & ~stay_mask3);
  }
  // Rotating every other group of 4 bits down by 4
  for (int j = 0; j < 4; j++){
    *(B+j) = (*(C+j) & stay_mask4) | (*(C+(j+60)) & ~stay_mask4);
  }
  for (int j = 4; j < 64; j++){
    *(B+j) = (*(C+j) & stay_mask4) | (*(C+(j-4)) & ~stay_mask4);
  }
  // Rotating every other group of 2 bits down by 2
  for (int j = 0; j < 2; j++){
    *(C+j) = (*(B+j) & stay_mask5) | (*(B+(j+62)) & ~stay_mask5);
  }
  for (int j = 2; j < 64; j++){
    *(C+j) = (*(B+j) & stay_mask5) | (*(B+(j-2)) & ~stay_mask5);
  }
  // Rotating every other bit down by 1
  *B = (*C & stay_mask6) | (*(C+63) & ~stay_mask6);
  for (int j = 1; j < 64; j++){
    *(B+j) = (*(C+j) & stay_mask6) | (*(C+(j-1)) & ~stay_mask6);
  }
  // Rotate all rows down by 1

  *C = *(B+63) & ~stay_mask7;
  for (int j = 1; j < 64; j++){
    *(C+j) = *(B+j-1) & ~stay_mask7;
  }
  // Write to resulting matrix
  for (int j = 0; j<64; j++){
    *(B+j) = *(C+j);
  }
  return;
}

