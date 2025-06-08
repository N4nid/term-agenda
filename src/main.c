#include "config.c"
#include <sys/stat.h>

int main(int argc, char *argv[]) {
  createConfig();
  readConfig();
  if (org_agenda_files != NULL) {
    printf("%s\n", org_agenda_files);
  }
  mfree(org_agenda_files);
  return 0;
}
