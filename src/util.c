#include <ctype.h>
#include <stdlib.h>
#include <string.h>

void mfree(void *ptr) {
  if (ptr != NULL) {
    free(ptr);
    ptr = NULL; // so that the pointer cant be used accidentally
  }
}

// stolen from
// https://stackoverflow.com/questions/1726302/remove-spaces-from-a-string-in-c
void remove_spaces(char *restrict str_trimmed,
                   const char *restrict str_untrimmed) {
  while (*str_untrimmed != '\0') {
    if (!isspace(*str_untrimmed)) {
      *str_trimmed = *str_untrimmed;
      str_trimmed++;
    }
    str_untrimmed++;
  }
  *str_trimmed = '\0';
}
