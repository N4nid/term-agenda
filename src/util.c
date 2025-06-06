#include <stdlib.h>
#include <string.h>

void mfree(void *ptr) {
  free(ptr);
  ptr = NULL; // so that the pointer cant be used accidentally
}
