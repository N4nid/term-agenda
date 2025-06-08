#include "util.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

char *configPath = NULL;
// config options:
// replace "_" with "-" for conf value. eg. cache_dir = cache-dir
char *org_agenda_files = NULL;
char *cache_dir = NULL;

void setConfigValue(char *optionString) {
  char *options[] = {"org-agenda-files=", "cache-dir="}; // ! have to end in =
  int optionIndex = -1;
  int inputStrLen = strlen(optionString);

  for (int i = 0; i < ARRAY_SIZE(options); i++) {
    int optionLen = strlen(options[i]);
    if (inputStrLen > optionLen) {
      if (0 == strncmp(options[i], optionString, optionLen)) {
        printf("%s\n", options[i]);
      }
    }
  }
  printf("%s", optionString);
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
    if (!(*line == '\n' || *line == ' ' || *line == '#')) {
      // line is not ignoreable (a comment or empty)
      setConfigValue(line);
    }
    lineNum = getline(&line, &size, file);
  }

  fclose(file);
  mfree(line);
}

void createConfig() {
  // create config file if necessary

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
