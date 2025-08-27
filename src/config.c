#include "util.c"
#include <dirent.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

char *configPath = NULL;
// helper vars
int agenda_files_amount = -1;
size_t todo_keywords_amount = -1;
// config options:
// replace "_" with "-" for conf value. eg. cache_dir = cache-dir (in conf file)
char **org_agenda_files = NULL;
int recursive_adding = 10; // TODO document default
char *cache_dir = NULL;
int max_threads = 10;                 // TODO document default
int tag_inheritance = 1;              // 1 true, 0 false
char *todo_keywordsCSV = "TODO,DONE"; // comma seperated list of todo-keywords
char **todo_keywords = NULL;

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
      if (org_agenda_files == NULL) {
        org_agenda_files = malloc(sizeof(char *));
      }
      size_t size;

      if (agenda_files_amount == -1) { // could already be set due to recursion
        agenda_files_amount = 0;
      }

      while ((ent = readdir(dir)) != NULL) {
        char *extension = ent->d_name;
        int filenameLen = strlen(extension);
        extension = extension + (filenameLen - 4);

        if (strncmp(extension, ".org", 4) == 0) { // is a org file
          printf(" ---- FILE:%s\n", ent->d_name);
          // resize array
          size = (agenda_files_amount + 1) * sizeof(org_agenda_files);
          char **tmp = realloc(org_agenda_files, size);
          if (tmp) {
            org_agenda_files = tmp;
          }

          org_agenda_files[agenda_files_amount] =
              calloc(pathLen + filenameLen, sizeof(char));
          strncat(org_agenda_files[agenda_files_amount], path, pathLen);
          strncat(org_agenda_files[agenda_files_amount], ent->d_name,
                  filenameLen);

          agenda_files_amount++;
        } // is a dir (exclude "." and "..")
        else if (ent->d_type == DT_DIR && strncmp(ent->d_name, "..", 3) != 0 &&
                 strncmp(ent->d_name, ".", 2) != 0 && recursive_adding == 1) {

          printf(" -------- DIR:%s\n", ent->d_name);
          char *newPath = calloc(pathLen + filenameLen, sizeof(char));
          strncat(newPath, path, pathLen);
          strncat(newPath, ent->d_name, filenameLen);
          printf(" -------- DIRPATH:%s\n", newPath);

          addAgendaFiles(newPath);
          newPath = NULL;
        }
      }

      closedir(dir);
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
      "org-agenda-files:", "cache-dir:",       "max-threads:", "todo-keywords:",
      "tag-inheritance:",  "recursive-adding:"}; // ! has to end in :
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
    optionValue = NULL;
    if (max_threads <= 0) {
      printf("\n*********************************\n* ! INVALID max-threads "
             "VALUE ! *\n*********************************\n");
      exit(-1);
    }

    break;
  case 3: // todo_keywords
    todo_keywords = split(optionValue, ",", &todo_keywords_amount);
    //    for (int i = 0; i < todo_keywords_amount; i++) {
    //      printf("%s\n", todo_keywords[i]);
    //    }
    free(optionValue);
    optionValue = NULL;
    break;
  case 4: // tag_inheritance
    tag_inheritance = atoi(optionValue);
    free(optionValue);
    optionValue = NULL;
    break;
  case 5: // recursive_adding
    recursive_adding = atoi(optionValue);
    free(optionValue);
    optionValue = NULL;
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
