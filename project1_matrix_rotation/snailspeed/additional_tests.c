#include <assert.h>

#include "../utils/utils.c"
#include "rotate.c"


void compare_imgs(uint8_t *img1, uint8_t *img2, const bits_t N){
  const bits_t n_words = N / 8;
  for (int i = 0; i < n_words; i++){
    assert(*(img1+i)==*(img2+i));
  }
  printf("Test is successfully passed!");
}


void run_test_1(){
  printf("Starting test 1.");
  
  const bits_t N = 64;

  uint64_t test_img_64[64];
  uint64_t rotated_img_64[64];

  for (int i = 0; i < N; i++){
    test_img_64[i] = 1<<(63-i);
    rotated_img_64[i] = 1<<(i);
  }
  print_bit_matrix(test_img_64);
  print_bit_matrix(rotated_img_64);
  uint8_t *test_img = (uint8_t*) test_img_64;
  uint8_t *rotated_img = (uint8_t*) rotated_img_64;
  
  rotate_bit_matrix(test_img, N);

  compare_imgs(test_img, rotated_img, N);
}




int main(){
  run_test_1();
  return 0;
}
