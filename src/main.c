#include "util.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *org_agenda_files;

void readConfig() {

  char *wow;
  size_t size = 0;
  getline(&wow, &size, stdin);
  printf("size of wow %ld\n", sizeof(*wow));
  printf("%s", wow);
  printf("length of wow %ld\n", strlen(wow));

  // org_agenda_files = calloc(, sizeof(int));
  // org_agenda_files = calloc(sizeof(wow), sizeof(char));
  org_agenda_files = (char *)malloc(size);
  memcpy(org_agenda_files, wow, size);
  printf("size of org_agenda_files %ld\n", sizeof(*org_agenda_files));
  mfree(wow);
}

int main(int argc, char *argv[]) {
  readConfig();
  printf("%s\n", org_agenda_files);
  mfree(org_agenda_files);
  return 0;
}
