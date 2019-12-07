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
#include <unistd.h>  // For `getopt`
#include <limits.h>  // For `INT_MAX`, `INT_MIN`
#include <string.h>  // For `strcmp`

#include "./utils.h"
#include "./tester.h"

extern void rotate_bit_matrix(uint8_t *img, const bits_t N);

const uint64_t UNUSED = (uint64_t)-1;

#define SET_UNUSED(v)                           \
  do {                                          \
    v = (typeof(v))UNUSED;                      \
  } while (false)

int main(int argc, char *argv[]) {
  int opt;

  enum test_type_e {TEST_NOT_SET, TEST_FILE, TEST_GENERATED, TEST_CORRECTNESS, TEST_TIERS};
  enum test_type_e test_type = TEST_NOT_SET;

  // The flags for a `TEST_FILE` test type
  char *fname = NULL;
  char *output_fname = NULL;

  // The flags for a `TEST_GENERATED` test type
  bits_t N = 0;
  int max_tier = -1;
  uint32_t DEFAULT_MAX_TIER = 10;
  uint32_t MAX_TIER_ALLOW = 40;

  // If the program was called without arguments, this is malformed input
  if (argc == 1) {
    goto help;
  }

  // Parse the CLI input!
  while ((opt = getopt(argc, argv, "ht:f:o:N:s:M:")) != -1) {
    switch (opt) {
    case 'h':  // Help
      goto help;
      break;  // For completeness

    case 't':  // Test type
      // Make sure the input is fresh
      if (test_type != TEST_NOT_SET) {
        goto help;
      }

      if (!strcmp("file", optarg)) {
        test_type = TEST_FILE;

        // The fields that should be unused
        SET_UNUSED(N);
        SET_UNUSED(max_tier);

      } else if (!strcmp("generated", optarg)) {
        test_type = TEST_GENERATED;

        // The fields that should be unused
        SET_UNUSED(fname);
        SET_UNUSED(output_fname);
        SET_UNUSED(max_tier);

      } else if (!strcmp("correctness", optarg)) {
        test_type = TEST_CORRECTNESS;

        // The fields that should be unused
        SET_UNUSED(fname);
        SET_UNUSED(output_fname);
        SET_UNUSED(N);
        SET_UNUSED(max_tier);

      } else if (!strcmp("tiers", optarg)) {
        test_type = TEST_TIERS;

        // The fields that should be unused
        SET_UNUSED(fname);
        SET_UNUSED(output_fname);
        SET_UNUSED(N);

      } else {
        // Malformed input
        goto help;
      }
      break;

    case 'f':  // Input file name
      // Make sure the input is fresh
      if (fname != NULL) {  // Also triggered by `UNUSED`
        goto help;
      }

      fname = optarg;
      break;

    case 'o':  // Output file name
      // Make sure the input is fresh
      if (output_fname != NULL) {  // Also triggered by `UNUSED`
        goto help;
      }

      output_fname = optarg;
      break;

    case 'M':  // Max tiers
      if (max_tier != -1) {  // Also triggered by `UNUSED`
        goto help;
      }

      max_tier = (bits_t)atoi(optarg);

      // Error check `max_tier`, the possible return values of `atoi`
      if (max_tier == INT_MAX || max_tier == INT_MIN) {
        printf("Invalid max tier: Max tier MUST be integer\n");
        goto help;
      }
      if (max_tier > MAX_TIER_ALLOW) {
        printf("Please use lower max tier\n");
        goto help;
      }
      if (max_tier < 0) {
        printf("Max tier must be non-negative\n");
        goto help;
      }
      break;

    case 'N':  // Generated image dimension
      // Make sure the input is fresh
      if (N != 0) {  // Also triggered by `UNUSED`
        goto help;
      }

      N = (bits_t)atoi(optarg);

      // Error check `N`, the possible return values of `atoi`
      if (!N || N == INT_MAX || N == INT_MIN) {
        printf("Invalid Dimension: Dimension MUST be integer\n");
        goto help;
      }
      if (N < 64 || N % 64 != 0) {
        printf("Invalid Dimension: Dimension MUST be a multiple of 64!\n");
        goto help;
      }

      break;

    default:
      goto help;
    }
      }

  // There should not be any extra arguments to be parsed,
  // otherwise this likely is a malformed input
  if (optind < argc) {
    goto help;
  }

  // Execute the respective tester function based on the CLI input
  switch (test_type) {
  case TEST_FILE:
  {
    // The `fname` is a required argument
    if (fname == NULL) {
      goto help;
    }

    // Whether to disregard the output or not
    if (!output_fname) {
      bool result = run_tester(fname, rotate_bit_matrix);
      printf("Result: %s\n", result ? "PASS" : "FAIL");
    } else {
      bool result = run_tester_save_output(fname, output_fname,
                                           rotate_bit_matrix, true);
      printf("Result: %s\n", result ? "PASS" : "FAIL");
    }

    break;
  }
  case TEST_GENERATED:
  {
    // The `N` is a required argument
    if (N == 0) {
      goto help;
    }

    bool result =
          run_tester_generated_bit_matrix(rotate_bit_matrix, N);

    printf("Result: %s\n", result ? "PASS" : "FAIL");

    break;
  }
  case TEST_CORRECTNESS:
  {
    bits_t START_SIZE = 64;

    bool correctness = run_correctness_tester(rotate_bit_matrix, START_SIZE);
    if (correctness)
        printf("PASS: Congrats! You pass all correctness tests\n");
    else
        printf("FAIL: Too bad. You have to fix bugs :'(\n");
    break;
  }
  case TEST_TIERS:
  {
    uint32_t TIER_TIMEOUT = 3000;
    uint32_t TIMEOUT = 58000;
    bits_t START_SIZE = 26624;
    double GROWTH_RATE = 1.1;
    if (max_tier == -1) {
        max_tier = DEFAULT_MAX_TIER;
    }

    uint32_t tier = run_tester_tiers(rotate_bit_matrix, TIER_TIMEOUT, TIMEOUT, 
        START_SIZE, GROWTH_RATE, (uint32_t) max_tier);

    if (tier == -1) {
        printf("FAIL: too slow for large tiers\n");
    } else {
        printf("Result: reached tier %d\n", tier);
    }

    break;
  }
  default:
    // If the `test_type` was not set, this is malformed input
    goto help;
  }

  // Success!
  return 0;

  help:
  printf("usage:\n"
         "\t" "-t {file|generated|       \t Select a test type        \t Required to select test type\n"
         "\t" "  correctness|tiers}\n"
         "\t" "-f file-name              \t Input file name           \t Required for \"file\" test type\n"
         "\t" "-o output-file-name       \t Output file name          \t Optional for \"file\" test type\n"
         "\t" "-N dimension              \t Generated image dimension \t Required for \"generated\" test type\n"
         "\t" "-M max-tier               \t Maximum tier              \t Optional for \"tiers\" test type\n"
         "\t" "-h                        \t This help message\n");

  return 1;
}
