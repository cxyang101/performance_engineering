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

#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <malloc.h>
#include <assert.h>

typedef size_t bits_t;
typedef size_t bytes_t;

size_t bits_to_bytes(bits_t nbits);

uint8_t get_bit(uint8_t *img, const bytes_t row_size, uint32_t i, uint32_t j);

void set_bit(uint8_t *img, const bytes_t row_size, uint32_t i, uint32_t j, uint8_t value);

void print_bit_matrix(uint8_t *bit_matrix, const bits_t N, int32_t subportion);

uint8_t *generate_bit_matrix(const bits_t N, bool suppress_error);

uint8_t *copy_bit_matrix(uint8_t *bit_matrix, const bits_t N);

#endif  // UTILS_H
