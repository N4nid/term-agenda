#include "util.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *org_agenda_files;

void readConfig() {

  char *wow;
  size_t size = 1;
  wow = (char *)malloc(size * sizeof(char));

  getline(&wow, &size, stdin);

  printf("size of wow %ld\n", size);
  printf("%s", wow);
  printf("length of wow %ld\n", strlen(wow));

  org_agenda_files = (char *)malloc(size);
  memcpy(org_agenda_files, wow, size);
  mfree(wow);
}

int main(int argc, char *argv[]) {
  readConfig();
  printf("%s\n", org_agenda_files);
  mfree(org_agenda_files);
  return 0;
}
