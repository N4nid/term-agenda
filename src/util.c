#include <stdlib.h>
#include <string.h>

void mfree(void *ptr) {
  // so that i cant forget to set the pointer to null
  free(ptr);
  ptr = NULL;
}
