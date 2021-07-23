#include "jvm.h"
#include "hashmap.h"

void test_hashmap(void) {
  hashmap *map = create_hashmap(); 
  int *numbers = malloc(sizeof(int) * 10);
  numbers[0] = 0;
  numbers[1] = 1;
  numbers[2] = 2;
  numbers[3] = 3;
  numbers[4] = 4;
  numbers[5] = 5;
  numbers[6] = 6;
  numbers[7] = 7;
  numbers[8] = 8;
  numbers[9] = 9;

  hm_insert(map, "1", &numbers[0]);
  hm_insert(map, "3", &numbers[3]);

  assert(*(int *)hm_get(map, "3") == 3);
  hm_insert(map, "3", &numbers[4]);
  assert(*(int *)hm_get(map, "3") == 4);

  destroy_hashmap(map); 
}

int main(void) {

  test_hashmap();

  // Assertions
  printf("Finished all tests successfully\n");
}
