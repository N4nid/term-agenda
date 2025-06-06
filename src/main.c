#include "util.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

char *org_agenda_files = NULL;

void createConfig() {
  // create config file if necessary
  // linux only currently

  struct stat sfileInfo;
  char *configPath = getenv("HOME");
  strcat(configPath, "/.config/");

  if (stat(configPath, &sfileInfo) == -1) { // ~/.config dir doesnt exist
    mkdir(configPath, 0755);
  }

  strcat(configPath, "term-agenda.conf");

  if (stat(configPath, &sfileInfo) == -1) { // config file doesnt exist
    printf("[i] config file doesnt exist -> creating it\n");
    printf("[i] configPath: %s \n", configPath);

    fopen(configPath, "w");
  }
}

void readConfig() {
  //  char *wow;
  //  size_t size = 1;
  //  wow = (char *)malloc(size * sizeof(char));
  //
  //  getline(&wow, &size, stdin);
  //
  //  printf("size of wow %ld\n", size);
  //  printf("%s", wow);
  //  printf("length of wow %ld\n", strlen(wow));
  //
  //  org_agenda_files = (char *)malloc(size);
  //  memcpy(org_agenda_files, wow, size);
  //  mfree(wow);
}

int main(int argc, char *argv[]) {
  createConfig();
  // readConfig();
  //   printf("%s\n", org_agenda_files);
  //   mfree(org_agenda_files);
  return 0;
}
