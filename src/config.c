#include "util.c"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

char *configPath = NULL;
int agenda_files_amount = -1;
// config options:
// replace "_" with "-" for conf value. eg. cache_dir = cache-dir (in conf file)
char **org_agenda_files = NULL;
char *cache_dir = NULL;
int max_threads = 10;    // TODO document default
int tag_inheritance = 1; // 1 true, 0 false

void addAgendaFiles(char *path) {
  path = fixPath(path);

  int pathLen = strlen(path);

  struct stat sfileInfo;
  stat(path, &sfileInfo);

  if (sfileInfo.st_mode & S_IFDIR) { // is a dir
    // borrowed from
    // https://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir(path)) != NULL) {
      int counter = 0;
      org_agenda_files = malloc(1 * sizeof(char));

      while ((ent = readdir(dir)) != NULL) {
        char *extension = ent->d_name;
        int filenameLen = strlen(extension);
        extension = extension + (filenameLen - 4);

        if (strcmp(extension, ".org") == 0) { // is a org file
          // resize array
          char **tmp = realloc(org_agenda_files,
                               (counter + 1) * sizeof(org_agenda_files));
          if (tmp) {
            org_agenda_files = tmp;
          }

          org_agenda_files[counter] =
              calloc(pathLen + filenameLen, sizeof(char));
          strncat(org_agenda_files[counter], path, pathLen);
          strncat(org_agenda_files[counter], ent->d_name, filenameLen);

          counter++;
        }
      }
      closedir(dir);
      agenda_files_amount = counter;
    }
  } else { // is a file
    org_agenda_files = malloc(1 * sizeof(org_agenda_files));

    org_agenda_files[0] = calloc(pathLen, sizeof(char));
    memcpy(org_agenda_files[0], path, pathLen);

    agenda_files_amount = 1;
  }

  free(path);
  path = NULL;
}

void setConfigValue(char *optionString) {
  char *options[] = {
      "org-agenda-files:", "cache-dir:", "max-threads:"}; // ! have to end in =
  int optionIndex = -1;
  int inputStrLen = strlen(optionString);
  int optionLen = -1;
  char *optionValue = NULL;

  // find out which option it is
  for (int i = 0; i < ARRAY_SIZE(options); i++) {
    optionLen = strlen(options[i]);
    if (inputStrLen > optionLen + 1) { // cant be the option if its shorter
      // check if its the same
      if (0 == strncmp(options[i], optionString, optionLen)) {
        printf("%s", optionString);
        optionIndex = i;
        break;
      }
    }
  }

  if (optionIndex == -1) { // no option found
    return;
  }

  int delta = inputStrLen - optionLen; // the amount of chars after :
  optionValue = calloc((inputStrLen - optionLen), sizeof(char));
  strlcpy(optionValue, optionString + optionLen, delta);

  switch (optionIndex) {
  case 0: // org_agenda_files
    addAgendaFiles(optionValue);
    break;
  case 1: // cache_dir
    optionValue = fixPath(optionValue);

    int len = strlen(optionValue) + 1;
    cache_dir = malloc(len * sizeof(char));
    memcpy(cache_dir, optionValue, len * sizeof(char));

    free(optionValue);
    optionValue = NULL;
    break;
  case 2: // max_threads
    max_threads = atoi(optionValue);
    free(optionValue);
    break;
  }
}

void readConfig() {
  FILE *file = fopen(configPath, "r");
  char *line;
  size_t size = 0; // should automatically be resized by getline
  int lineNum = 0;

  if (file == NULL) {
    printf("[!] error opening config file");
    fclose(file);
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
  free(line);
  line = NULL;
  free(configPath);
  configPath = NULL;
}

void createConfig() {
  // create config file if necessary

  configPath = getenv("HOME");
  int size = strlen(configPath);
  configPath = calloc(size, sizeof(char));
  memcpy(configPath, getenv("HOME"), size);

  configPath = realloc(configPath, size + sizeof("/.config/"));
  strcat(configPath, "/.config/");

  struct stat sfileInfo;

  if (stat(configPath, &sfileInfo) == -1) { // ~/.config dir doesnt exist
    mkdir(configPath, 0755);
  }

  configPath = realloc(configPath, size + sizeof("term-agenda.conf"));
  strcat(configPath, "term-agenda.conf");

  if (stat(configPath, &sfileInfo) == -1) { // config file doesnt exist
    printf("[i] config file doesnt exist -> creating it\n");
    printf("[i] configPath: %s \n", configPath);

    fopen(configPath, "w");
  } else {
    printf("config exists: %s \n", configPath);
  }
}
