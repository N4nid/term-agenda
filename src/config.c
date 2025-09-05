#include "util.c"
#include <dirent.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

// cmdline options
int isSetRecAdding = 0;
int isSetAgendaFilesPath = 0;
int isSetInheritance = 0;
int isSetTodoKWDS = 0;
int isSetMaxThreads = 0;
int isSetHiddenDirInclusion = 0;

char *configPath = NULL;
// helper vars
int includeHiddenDirs = 0;
int agenda_files_amount = 0;
size_t todo_keywords_amount = 0;
char *agenda_files_path = NULL;
// config options:
// replace "_" with "-" for conf value. eg. cache_dir = cache-dir (in conf file)
char **org_agenda_files = NULL;
int recursive_adding = 1; // TODO document default
char *cache_dir = NULL;
int max_threads = 0;                  // TODO document default
int tag_inheritance = 1;              // 1 true, 0 false
char *todo_keywordsCSV = "TODO,DONE"; // comma seperated list of todo-keywords
char **todo_keywords = NULL;
char *time_format = NULL; // fe. %Y-%m-%d

void addAgendaFiles(char *path) {
  int pathLen = strlen(path);

  struct stat sfileInfo;
  stat(path, &sfileInfo);

  if (sfileInfo.st_mode & S_IFDIR) { // is a dir
    // printf("  IS A DIR\n");
    //  borrowed from
    //  https://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir(path)) != NULL) {

      if (org_agenda_files == NULL) {
        org_agenda_files = malloc(sizeof(char *));
      }
      size_t size;

      char *newPath;
      int len = 0;
      while ((ent = readdir(dir)) != NULL) {
        char *extension = ent->d_name;
        int filenameLen = strlen(extension);
        extension = extension + (filenameLen - 4);

        if (strncmp(extension, ".org", 4) == 0) { // is a org file
          // printf(" ---- FILE:%s\n", ent->d_name);
          //  resize array
          size = (agenda_files_amount + 1) * sizeof(org_agenda_files);
          char **tmp = realloc(org_agenda_files, size);
          if (tmp) {
            org_agenda_files = tmp;
          }

          len = pathLen + filenameLen + 2;
          org_agenda_files[agenda_files_amount] = calloc(len, sizeof(char));
          memcpy(org_agenda_files[agenda_files_amount], path, pathLen);
          memcpy(org_agenda_files[agenda_files_amount] + pathLen, ent->d_name,
                 filenameLen);
          // strncat(org_agenda_files[agenda_files_amount], ent->d_name,
          // filenameLen);

          agenda_files_amount++;
        } // is a dir (exclude "." and "..")
        else if (ent->d_type == DT_DIR && strncmp(ent->d_name, "..", 3) != 0 &&
                 strncmp(ent->d_name, ".", 2) != 0 && recursive_adding == 1) {

          if ('.' == ent->d_name[0] && includeHiddenDirs == 0) {
            continue;
          }

          len = pathLen + filenameLen + 2;
          newPath = calloc(len, sizeof(char));
          memcpy(newPath, path, pathLen);
          strncat(newPath, ent->d_name, filenameLen);
          strncat(newPath, "/", 2);
          // printf(" -------- DIRPATH:|%s|\n", newPath);

          // printf("+ GOING :C\n");
          addAgendaFiles(newPath);
          // printf("+ IM BACK :D\n");
          newPath = NULL;
        }
      }
      closedir(dir);
      dir = NULL;

    } else {
      printf("-- could not open dir ! --\n");
    }
  } else { // is a file
    // printf(" -IS A FILE\n");
    org_agenda_files = malloc(1 * sizeof(org_agenda_files));

    org_agenda_files[0] = calloc(pathLen + 1, sizeof(char));
    memcpy(org_agenda_files[0], path, pathLen);

    agenda_files_amount = 1;
  }

  free(path);
  path = NULL;
}

void setConfigValue(char *optionString) {
  char *options[] = {
      "org-agenda-files:", "cache-dir:",       "max-threads:",
      "todo-keywords:",    "tag-inheritance:", "recursive-adding:",
      "time-format:",      "include-hidden:"}; // ! has to end in :
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
        // printf("%s", optionString);
        optionIndex = i;
        break;
      }
    }
  }

  if (optionIndex == -1) { // no option found
    return;
  }

  int delta = inputStrLen - optionLen; // the amount of chars after :
  optionValue = calloc(delta + 1, sizeof(char));
  strncpy(optionValue, optionString + optionLen, delta - 1);
  strncat(optionValue, "\0", 1);
  // strlcpy(optionValue, optionString + optionLen, delta);

  switch (optionIndex) {
  case 0:                            // org_agenda_files
    if (agenda_files_path != NULL) { // has already been set by cmd args
      break;
    }
    optionValue = fixPath(optionValue);
    addAgendaFiles(optionValue);
    // free(optionValue);
    optionValue = NULL;
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
    if (isSetMaxThreads == 0) {
      max_threads = atoi(optionValue);
    }
    free(optionValue);
    optionValue = NULL;
    if (max_threads < 0) {
      printf("\n*********************************\n* ! INVALID max-threads "
             "VALUE ! *\n*********************************\n");
      exit(-1);
    }

    break;
  case 3: // todo_keywords
    if (!isSetTodoKWDS) {
      todo_keywords = split(optionValue, ",", &todo_keywords_amount);
    }
    free(optionValue);
    optionValue = NULL;
    break;
  case 4: // tag_inheritance
    if (isSetInheritance == 0) {
      tag_inheritance = atoi(optionValue);
    }
    free(optionValue);
    optionValue = NULL;
    break;
  case 5:                      // recursive_adding
    if (isSetRecAdding == 0) { // has not been set by cmd args
      recursive_adding = atoi(optionValue);
    }
    free(optionValue);
    optionValue = NULL;
    break;
  case 6: // time_format
    len = strlen(optionValue) + 1;
    time_format = malloc(len * sizeof(char));
    memcpy(time_format, optionValue, len * sizeof(char));

    free(optionValue);
    optionValue = NULL;
    break;
  case 7:                               // include-hidden
    if (isSetHiddenDirInclusion == 0) { // has not been set by cmd args
      includeHiddenDirs = atoi(optionValue);
    }
    free(optionValue);
    optionValue = NULL;
    break;
  }
}

void readConfig() {
  FILE *file = fopen(configPath, "r");
  char *line = NULL;
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

  int size = strlen(getenv("HOME"));
  configPath =
      calloc(size + 26, sizeof(char)); // 26 for "/.config/term-agenda.conf"
  memcpy(configPath, getenv("HOME"), size);
  strcat(configPath, "/.config/");

  struct stat sfileInfo;
  int exists = 0;
  exists = stat(configPath, &sfileInfo);

  if (exists == -1) { // ~/.config dir doesnt exist
    mkdir(configPath, 0755);
  }

  strcat(configPath, "term-agenda.conf");

  exists = stat(configPath, &sfileInfo);
  if (exists == -1) { // config file doesnt exist
    printf("[i] config file doesnt exist -> creating it\n");
    printf("[i] configPath: %s \n", configPath);

    fopen(configPath, "w");
  } else {
    // printf("config exists: %s \n", configPath);
  }
}
