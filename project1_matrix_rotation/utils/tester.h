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

#ifndef TESTER_H
#define TESTER_H

#include "./utils.h"

void exitfunc(int sig);

bool run_tester(const char *fname, void (*rotate_fn)(uint8_t*, const bits_t));

bool run_tester_save_output(const char *fname, const char *output_fname,
                            void (*rotate_fn)(uint8_t*, const bits_t),
                            bool correctness);

bool run_tester_generated_bit_matrix(void (*rotate_fn)(uint8_t*, const bits_t),
                                     const bits_t N);

uint32_t run_tester_tiers(void (*rotate_fn)(uint8_t*, const bits_t),
                          uint32_t tier_timeout, 
                          uint32_t timeout,
                          bits_t start_n, 
                          double increasing_ratio_of_n, 
                          uint32_t highest_tier);

bool run_correctness_tester(void (*rotate_fn)(uint8_t*, const bits_t),
                          bits_t start_n);

#endif  // TESTER_H
