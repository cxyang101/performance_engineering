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
#include <string.h>
#include "./utils.h"
#include "./libbmp.h"
#include <math.h>
#include <signal.h>
#include <unistd.h>

void exitfunc(int sig) {
    printf("End execution due to 58s timeout\n");
    exit(0);
}

// Rotates a bit array clockwise 90 degrees.
//
// The bit array is of `N` by `N` bits where N is a multiple of 64 and N >= 64
static void _rotate_bit_matrix(uint8_t *img, const bits_t N) {
  // Get the number of bytes per row in `img`
  const uint32_t row_size = bits_to_bytes(N);

  uint32_t w, h, quadrant;
  for (h = 0; h < N / 2; h++) {
    for (w = 0; w < N / 2; w++) {
      uint32_t i = w, j = h;
      uint8_t tmp_bit = get_bit(img, row_size, i, j);

      // Move a bit from one quadrant to the next and do this
      // for all 4 quadrants of the `img`
      for (quadrant = 0; quadrant < 4; quadrant++) {
        uint32_t next_i = N - j - 1, next_j = i;
        uint8_t save_bit = tmp_bit;

        tmp_bit = get_bit(img, row_size, next_i, next_j);
        set_bit(img, row_size, next_i, next_j, save_bit);

        // Update the `i` and `j` with the next quadrant's
        // values and the `next_i` and `next_j` will get the
        // new destination values
        i = next_i;
        j = next_j;
      }
    }
  }

  return;
}

// Runs the tester for the input file `fname`. Tests the
// user supplied `rotate_fn` function against a working
// stock rotation function.
//
// Returns `true` if the tester passed
bool run_tester(const char *fname, void (*rotate_fn)(uint8_t*,
                                                     const bits_t)) {
  // Sanity check the input
  assert(fname);
  assert(rotate_fn);

  struct color_table_s color_tables[2];
  int width, height, row_size;
  uint8_t *img = read_binary_bmp(fname, &width, &height,
                                 &row_size, color_tables);

  // Check whether there was an error
  if (!img) {
    return false;
  }

  // Assert that the image is square, that the dimensions are >= 64
  // bits and a multiple of 64.
  //
  // The condition for >= 32 is since the BMP pads the image with 0's
  // to align each row to 4 bytes. If the image is a multiple of 64,
  // then there will be no padding, only image.
  assert(width == height);
  assert(width >= 64);
  assert(width % 64 == 0);
  assert(width == 8 * row_size);

  // Make a copy of `img` for the user function to rotate
  const bytes_t img_size = height * row_size;
  uint8_t *img_copy = malloc(img_size);
  memcpy(img_copy, img, img_size);

  // Call the user-defined `rotate_fn` and time it
  clock_t start = clock();
  rotate_fn(img_copy, width);
  clock_t user_diff = clock() - start;

  // Call our stock rotation function on `img`
  start = clock();
  _rotate_bit_matrix(img, width);
  clock_t stock_diff = clock() - start;

  bool result = memcmp(img, img_copy, img_size) == 0;

  // Clean up after ourselves!
  free(img_copy);
  free(img);

  // Print the time taken to rotate the images using the
  // user-define `rotate_fn` and stock function
  uint32_t user_msec = user_diff * 1000 / CLOCKS_PER_SEC;
  uint32_t stock_msec = stock_diff * 1000 / CLOCKS_PER_SEC;
  printf("Your time taken: %d milliseconds\n", user_msec);
  printf("Stock time taken: %d milliseconds\n", stock_msec);

  return result;
}

// Runs the tester for the input file `fname`. If `correctness` is
// set to `true`, tests the user supplied `rotate_fn` function against
// a working stock rotation function.
//
// This function saves the user's output rotated image, regardless of
// correctness, to `output_fname`.
//
// If `correctness` is `false`, always returns `false`. Otherwise
// returns `true` if the tester passed
bool run_tester_save_output(const char *fname, const char *output_fname,
                            void (*rotate_fn)(uint8_t*, const bits_t),
                            bool correctness) {
  // Sanity check the input
  assert(fname);
  assert(output_fname);
  assert(rotate_fn);

  struct color_table_s color_tables[2];
  int width, height, row_size;
  uint8_t *img = read_binary_bmp(fname, &width, &height,
                                 &row_size, color_tables);

  // Check whether there was an error
  if (!img) {
    return false;
  }

  // Assert that the image is square, that the dimensions are >= 64
  // bits and a multiple of 64.
  //
  // The condition for >= 64 is since the BMP pads the image with 0's
  // to align each row to 4 bytes. If the image is a multiple of 64,
  // then there will be no padding, only image.
  assert(width == height);
  assert(width >= 64);
  assert(width % 64 == 0);
  assert(width == 8 * row_size);

  bool result = false;
  uint8_t *img_copy = NULL;

  if (correctness) {
    // Make a copy of `img` for the stock function to rotate
    const bytes_t img_size = height * row_size;
    img_copy = malloc(img_size);
    memcpy(img_copy, img, img_size);

    // Call the user-defined `rotate_fn` and time it
    clock_t start = clock();
    rotate_fn(img, width);
    clock_t user_diff = clock() - start;

    // Write the rotated output to `output_fname`
    write_binary_bmp(output_fname, img, color_tables, width);

    // Call our stock rotation function on `img_copy`
    start = clock();
    _rotate_bit_matrix(img_copy, width);
    clock_t stock_diff = clock() - start;

    result = memcmp(img_copy, img, img_size) == 0;

    // Print the time taken to rotate the images using the
    // user-define `rotate_fn` and stock function
    uint32_t user_msec = user_diff * 1000 / CLOCKS_PER_SEC;
    uint32_t stock_msec = stock_diff * 1000 / CLOCKS_PER_SEC;
    printf("Your time taken: %d milliseconds\n", user_msec);
    printf("Stock time taken: %d milliseconds\n", stock_msec);

  } else {
    // We are not testing for correctness, so just rotate

    // Call the user-defined `rotate_fn` and time it
    clock_t start = clock();
    rotate_fn(img, width);
    clock_t user_diff = clock() - start;

    // Write the rotated output to `output_fname`
    write_binary_bmp(output_fname, img, color_tables, width);

    // Print the time taken to rotate the image using the
    // user-define `rotate_fn`
    uint32_t user_msec = user_diff * 1000 / CLOCKS_PER_SEC;
    printf("Your time taken: %d milliseconds\n", user_msec);
  }

  // Clean up after ourselves!
  free(img_copy);
  free(img);

  return result;
}

// Runs the tester on a generated bit matrix. Tests the user
// supplied `rotate_fn` function against a working stock rotation
// function
//
// Returns `true` if the tester passed
bool run_tester_generated_bit_matrix(void (*rotate_fn)(uint8_t*,
                                                       const bits_t),
                                     const bits_t N) {
  // Sanity check the input
  assert(rotate_fn);
  assert(N > 0);

  // Checks whether `N` is a multiple of 64
  assert(!(N % 64));

  const bytes_t row_size = bits_to_bytes(N);

  const bytes_t bit_matrix_size = N * row_size;
  uint8_t *bit_matrix = generate_bit_matrix(N, false);
  uint8_t *bit_matrix_copy = copy_bit_matrix(bit_matrix, N);
  
  // Call the user-defined `rotate_fn` and time it
  clock_t start = clock();
  rotate_fn(bit_matrix, N);
  clock_t user_diff = clock() - start;

  // Call our stock rotation function on `img`
  start = clock();
  _rotate_bit_matrix(bit_matrix_copy, N);
  clock_t stock_diff = clock() - start;

  bool result =
    memcmp(bit_matrix, bit_matrix_copy, bit_matrix_size) == 0;

  // Clean up after ourselves!
  free(bit_matrix);
  free(bit_matrix_copy);

  // Print the time taken to rotate the images using the
  // user-define `rotate_fn` and stock function
  uint32_t user_msec = user_diff * 1000 / CLOCKS_PER_SEC;
  uint32_t stock_msec = stock_diff * 1000 / CLOCKS_PER_SEC;
  printf("Your time taken: %d milliseconds\n", user_msec);
  printf("Stock time taken: %d milliseconds\n", stock_msec);

  return result;
}

// Runs the tester on generated bit matrices of increasing sizes (tiers).
// Tests the user supplied `rotate_fn` function against a working stock
// rotation function. The tester doubles the dimension of the bit matrix
// until the `rotate_fn` cannot rotate it under (<) `timeout` milliseconds
// or returns an incorrect solution.
//
// Returns the tier number that `rotate_fn` got to
//
uint32_t run_tester_tiers(void (*rotate_fn)(uint8_t*, const bits_t),
                          uint32_t tier_timeout, 
                          uint32_t timeout,
                          bits_t start_n, 
                          double increasing_ratio_of_n,
                          uint32_t highest_tier) {
  // Sanity check the input
  uint32_t MAX_ALLOWED_TIERS = 40;
  assert(highest_tier <= MAX_ALLOWED_TIERS);
  assert(rotate_fn);
  assert(start_n % 64 == 0);

  // set timer
  uint32_t MS_TO_SEC = 1000;
  signal(SIGALRM, exitfunc);
  alarm((uint32_t)(timeout / MS_TO_SEC));

  printf("Setting up test up to tier %u: ", highest_tier);
  
  // Generate tier sizes starting from start_n and increase by increasing_ratio_of_n
  // until it reach largest size that less than or equal to end_n
  bits_t N = start_n;
  bits_t tier_sizes[MAX_ALLOWED_TIERS + 5];

  uint32_t i = 0;
  for (N = start_n; i <= MAX_ALLOWED_TIERS; N = (uint64_t) ceil(N * increasing_ratio_of_n / 64) * 64) {
      tier_sizes[i++] = N;
  }

  printf("Malloc %zux%zu matrix...\n", tier_sizes[highest_tier], tier_sizes[highest_tier]);
  uint8_t *bit_matrix = generate_bit_matrix(tier_sizes[highest_tier], true);

  if (!bit_matrix) {
      printf("Error: Run out of heap space! Please choose smaller tier\n");
      assert(false);
  }

  uint32_t tier = 0;

  printf("Start tiers testing\n");
  // Be sure to increase the matrix dimension on every iteration
  for (tier = 0; tier <= highest_tier; tier++) {
    N = tier_sizes[tier];
    // Call the user-defined `rotate_fn` and time it
    clock_t start = clock();
    rotate_fn(bit_matrix, N);
    clock_t user_diff = clock() - start;

    // Compute the user time in milliseconds
    uint32_t user_msec = user_diff * 1000 / CLOCKS_PER_SEC;

    // Exit if the user time is too much, but was still correct!
    if (user_msec >= tier_timeout) {
      printf("FAIL (timeout) : Tier %d : rotated %zux%zu matrix once in (%d >= %d) milliseconds\n",
             tier, N, N, user_msec, tier_timeout);
      // Exit!
      goto finish;
    } else {  // Success
      // For some fun!
      const char *celebrations[] = {"yay", "woot", "boyah"};
      uint32_t ncelebrations = sizeof(celebrations) / sizeof(celebrations[0]);

      const char *random_celebration = celebrations[rand() % ncelebrations];

      printf("PASS (%s!): Tier %d : Rotated %zux%zu matrix once in %d milliseconds\n",
             random_celebration, tier, N, N, user_msec);
    }
  }

  finish:
    // Clean up after ourselves!
    free(bit_matrix);
    if (tier == highest_tier + 1) {
        printf("Congrats! You reach the highest tiers :)\n");
        printf("Please run this test with higher tier to find your maximum tier.\n");
    }
    tier--;
    // Save the highest tier and best runtime
    // in their respective return fields
    return tier;
}



// Runs the tester on generated bit matrices of increasing sizes (tiers).
// Tests the user supplied `rotate_fn` function against a working stock
// rotation function. The tester doubles the dimension of the bit matrix
// until the `rotate_fn` cannot rotate it under (<) `timeout` milliseconds
// or returns an incorrect solution.
//
// Returns the tier number that `rotate_fn` got to
//
bool run_correctness_tester(void (*rotate_fn)(uint8_t*, const bits_t),
                          bits_t start_n) {
  // Sanity check the input
  assert(rotate_fn);
  assert(start_n % 64 == 0);

  // Start rotating with start_nxstart_n bit matrices and increase N by SQRT_GOLDEN_RATIO
  // the dimension when incrementing the `tier`
  bits_t N = start_n;

  uint32_t tier = 0;
  bool correctness;
  const double SQRT_GOLDEN_RATIO = 1.2720196495141103;

  // Be sure to increase the matrix dimension on every iteration
  for (; N < 10000; N = (uint64_t) ceil(N * SQRT_GOLDEN_RATIO / 64) * 64) {
    uint32_t i;
    uint8_t *bit_matrix = generate_bit_matrix(N, false);
    uint8_t *bit_matrix_copy = copy_bit_matrix(bit_matrix, N);
    const bytes_t row_size = bits_to_bytes(N);
    const bytes_t bit_matrix_size = N * row_size;

    const char *english_multiples[] = {"once", "twice", "three times"};
    uint32_t user_msec = 0;
    for (i = 0; i < 3; i++, tier++) {
      // Call the user-defined `rotate_fn` and time it
      clock_t start = clock();
      rotate_fn(bit_matrix, N);
      clock_t user_diff = clock() - start;

      // Compute the user time in milliseconds
      user_msec += user_diff * 1000 / CLOCKS_PER_SEC;

      // Checking correctness - Call our stock rotation function on bit_matrix
      _rotate_bit_matrix(bit_matrix_copy, N);
      correctness = memcmp(bit_matrix, bit_matrix_copy, bit_matrix_size) == 0;

      if (!correctness) {  // The rotation was not correct
        printf("FAIL : Test %d : Incorrectly rotated %zux%zu matrix\n",
               tier, N, N);

        // Exit!
        return false;
      }

      // For some fun!
      const char *celebrations[] = {"yay", "woot", "boyah"};
      uint32_t ncelebrations = sizeof(celebrations) / sizeof(celebrations[0]);

      const char *random_celebration = celebrations[rand() % ncelebrations];

      printf("PASS (%s!): Test %d : Rotated %zux%zu matrix %s in %d milliseconds\n",
             random_celebration, tier, N, N, english_multiples[i], user_msec);
    }
    // Clean up after ourselves!
    free(bit_matrix);
    free(bit_matrix_copy);
  }
  return true;
}
