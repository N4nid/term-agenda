#include "config.c"
#include <stdio.h>
#include <sys/stat.h>

void freeAllGlobals() {

  for (int i = 0; i < agenda_files_amount; i++) {
    printf("%s\n", org_agenda_files[i]);
    free(org_agenda_files[i]);
    org_agenda_files[i] = NULL;
  }
  free(org_agenda_files);
  org_agenda_files = NULL;
  // printf("%s\n", cache_dir);
  free(cache_dir);
  cache_dir = NULL;
}

int main(int argc, char *argv[]) {
  createConfig();
  readConfig();

  freeAllGlobals();
  return 0;
}
