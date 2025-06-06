#include "util.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

char *org_agenda_files = NULL;
char *configPath = NULL;

void createConfig() {
  // create config file if necessary
  // linux only currently

  struct stat sfileInfo;
  configPath = getenv("HOME");
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
  FILE *file = fopen(configPath, "r");
  char *line;
  size_t size = 1;
  int lineNum = 0;

  if (file == NULL) {
    printf("[!] error opening config file");
  }

  lineNum = getline(&line, &size, file);
  while (lineNum >= 0) {
    printf("%s", line);

    lineNum = getline(&line, &size, file);
  }

  fclose(file);
  mfree(line);

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
  readConfig();
  //   printf("%s\n", org_agenda_files);
  //   mfree(org_agenda_files);
  return 0;
}
