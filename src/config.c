#include "util.c"
#include "util.h"
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
int isSetTimeFormat = 0;

size_t searchAmount = 0;
char **searchString = NULL;
int skipConfig = 0;
char *customSearch = NULL;
int customSearchLen = 0;

char *configPath = NULL;
// helper vars
int agenda_files_amount = 0;
size_t todo_keywords_amount = 0;
char *agenda_files_path = NULL;
// config options:
// replace "_" with "-" for conf value. eg. cache_dir = cache-dir (in conf file)
int includeHiddenDirs = 0;
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
      "time-format:",      "include-hidden:",  "query:"}; // ! has to end in :
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
  case 0: // org_agenda_files
          // has already been set by cmd args
    if (agenda_files_path != NULL || isSetAgendaFilesPath) {
      free(optionValue);
      optionValue = NULL;
      break;
    }
    if (customSearch != NULL) {
      isSetAgendaFilesPath = 1;
    }
    optionValue = fixPath(optionValue);
    addAgendaFiles(optionValue);
    optionValue = NULL;
    break;
  case 1: // cache_dir
    optionValue = fixPath(optionValue);

    int len = strlen(optionValue) + 1;
    cache_dir = malloc(len * sizeof(char));
    memcpy(cache_dir, optionValue, len);

    free(optionValue);
    optionValue = NULL;
    break;
  case 2: // max_threads
    if (isSetMaxThreads == 0) {
      max_threads = atoi(optionValue);
    }
    if (customSearch != NULL) {
      isSetMaxThreads = 1;
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
    if (customSearch != NULL) {
      isSetTodoKWDS = 1;
    }
    free(optionValue);
    optionValue = NULL;
    break;
  case 4: // tag_inheritance
    if (isSetInheritance == 0) {
      tag_inheritance = atoi(optionValue);
    }
    if (customSearch != NULL) {
      isSetInheritance = 1;
    }
    free(optionValue);
    optionValue = NULL;
    break;
  case 5:                      // recursive_adding
    if (isSetRecAdding == 0) { // has not been set by cmd args
      recursive_adding = atoi(optionValue);
    }
    if (customSearch != NULL) {
      isSetRecAdding = 1;
    }
    free(optionValue);
    optionValue = NULL;
    break;
  case 6: // time_format
    if (isSetTimeFormat == 0) {
      len = strlen(optionValue) + 1;
      time_format = malloc(len * sizeof(char));
      memcpy(time_format, optionValue, len);
    }
    free(optionValue);
    optionValue = NULL;
    break;
  case 7:                               // include-hidden
    if (isSetHiddenDirInclusion == 0) { // has not been set by cmd args
      includeHiddenDirs = atoi(optionValue);
    }
    if (customSearch != NULL) {
      includeHiddenDirs = 1;
    }
    free(optionValue);
    optionValue = NULL;
    break;
  case 8: // query
    len = strlen(optionValue) + 1;
    searchString = malloc(len * sizeof(char));
    memcpy(searchString, optionValue, len);

    free(optionValue);
    optionValue = NULL;
    break;
  }
}

void readConfig() {
  if (configPath == NULL) {
    printf("[!] error opening config file\n");
    printf("[!] configPath is NULL");
  }
  FILE *file = fopen(configPath, "r");
  char *line = NULL;
  size_t size = 0; // should automatically be resized by getline
  int lineLen = 0;

  int parsingTheRightOne = 0;
  char customSearchDelim = '.';
  fpos_t filePos;
  int parsingCustomSearch = -1;

  if (file == NULL) {
    printf("[!] error opening config file\n");
    fclose(file);
  }

  lineLen = getline(&line, &size, file);
  while (lineLen >= 0) {
    // line is not ignoreable (a comment or empty)
    if (!(*line == '\n' || *line == ' ' || *line == '\0' || *line == '#' ||
          customSearch != NULL)) {
      setConfigValue(line);
    }
    if (customSearch != NULL && line[0] == customSearchDelim) {
      // if (line[0] == customSearchDelim) {
      if (parsingCustomSearch == 1) {
        parsingCustomSearch = 0;
        if (parsingTheRightOne == 1) {
          parsingTheRightOne = 2;
        }
      } else {
        parsingCustomSearch = 1;
      }
    }
    if (parsingCustomSearch && customSearch != NULL) {
      if (line[0] == '+' && !parsingTheRightOne) {
        // - 2 to exclude the + and \n
        if (strncmp(customSearch, line + 1, lineLen - 2) == 0 &&
            lineLen - 2 == customSearchLen) {
          parsingTheRightOne = 1;
        }
      } else if (parsingTheRightOne == 1) {
        if (line[0] == '-' || line[0] == '#') { // is a setting
          if (line[0] == '-') {
            setConfigValue(line + 1);
            // printf("l:%s", line + 1);
          }
        } else {
          parsingTheRightOne = 2;
        }
      }
    }
    lineLen = getline(&line, &size, file);
  }

  fclose(file);
  free(line);
  line = NULL;

  if (customSearch != NULL &&
      (parsingCustomSearch == -1 || parsingTheRightOne != 2)) {
    printf("this custom query is not defined\n");
  }
}

void breakDueToMissingArg() {
  printf("argument is missing\n");
  exit(0);
}

void printHelp() {
  char *help = "\
commandline options:\n\
note that they all override settings specified in the config file\n\
-q <query> syntax is detailed below\n\
-Q <custom query> use a query defined in the config\n\
-p <path to org file[s]>\n\
-a include \"hidden\" dirs (dir starting with a dot)\n\
   has to be set before -p since it whould otherwise not have any effect\n\
-f format to use when parsing dates\n\
-t set the todo keywords which should be recognized\n\
   there must not be a space after or before the comma\"\n\
   fe.: -t \"TODO,DONE,STRT\"\n\
-T set the amount of threads to use for parsing the org files\n\
   set to 0 to use as many threads as there are files\n\
-R disables recursively adding org files to the list of files to be searched\n\
-I disables the usage of tag inheritance\n\
-s skip reading the config (mainly for testing)\n\
\n\
query syntax:\n\
the query can consist of multiple conditions that should be met\n\
\n\
fields:\n\
  TAG\n\
  TODO\n\
  NAME (the heading name)\n\
  SCHED (provide date in the same format as specified in the config option)\n\
  DEAD\n\
  PROP\n\
\n\
match types:\n\
  exact: ==\n\
  contains: ~=\n\
\n\
logical operators:\n\
  and &\n\
  or |\n\
  not !\n\
  (brackets)\n\
\n\
examples:\n\
TAG=='work' & (TODO=='TODO' | TAG=='important')\n\
PROP==['key':'value']\n\
\n\
lists all headings that have a property with the value 'value2'\n\
PROP~=['':'value2']\n\
\n\
lists all headings that have a property with the key 'key2'\n\
PROP~=['key2':'']\n\
\n\
Further options are detailed in the example config\n\
";
  printf("%s", help);
}

void setArgumentOptions(int argc, char *argv[]) {
  int setQuery = 0;

  // count amount of querys
  for (int i = 1; i < argc; i++) {
    if (strncmp("-q", argv[i], 3) == 0) {
      if (argv[i + 1] == NULL)
        breakDueToMissingArg();
      searchAmount++;
      i++;
    }
  }

  // printf("search Amount %ld\n", searchAmount);
  int searchIndex = 0;

  for (int i = 1; i < argc; i++) {
    // printf("%s\n", argv[i]);
    if (strncmp("-q", argv[i], 3) == 0) {
      if (argv[i + 1] == NULL)
        breakDueToMissingArg();

      if (searchString == NULL) {
        searchString = calloc(searchAmount, sizeof(char *));
      }

      int len = strlen(argv[i + 1]);
      searchString[searchIndex] = calloc(len + 2, sizeof(char));

      memcpy(searchString[searchIndex], argv[i + 1], len);
      setQuery = 1;
      searchIndex++;
      i++;

    } else if (strncmp("-Q", argv[i], 3) == 0) {
      if (argv[i + 1] == NULL)
        breakDueToMissingArg();

      int len = strlen(argv[i + 1]);
      customSearchLen = len;
      customSearch = calloc(len + 2, sizeof(char));
      memcpy(customSearch, argv[i + 1], len);
      skipConfig = 0;
      readConfig();
      free(customSearch);
      customSearch = NULL;
      break;

    } else if (strncmp("-p", argv[i], 3) == 0) {
      if (argv[i + 1] == NULL)
        breakDueToMissingArg();

      int len = strlen(argv[i + 1]);
      agenda_files_path = calloc(len + 2, sizeof(char));
      memcpy(agenda_files_path, argv[i + 1], len);
      agenda_files_path = fixPath(agenda_files_path);
      addAgendaFiles(agenda_files_path);
      i++;
      isSetAgendaFilesPath = 1;

    } else if (strncmp("-t", argv[i], 3) == 0) {
      if (argv[i + 1] == NULL)
        breakDueToMissingArg();

      todo_keywordsCSV = argv[i + 1];
      todo_keywords = split(todo_keywordsCSV, ",", &todo_keywords_amount);
      i++;
      isSetTodoKWDS = 1;

    } else if (strncmp("-f", argv[i], 3) == 0) {
      if (argv[i + 1] == NULL)
        breakDueToMissingArg();

      int len = strlen(argv[i + 1]);
      time_format = calloc(len + 2, sizeof(char));
      memcpy(time_format, argv[i + 1], len);
      isSetTimeFormat = 1;
      i++;

    } else if (strncmp("-r", argv[i], 3) == 0) {
      isSetRecAdding = 1;
      recursive_adding = 1;

    } else if (strncmp("-h", argv[i], 3) == 0) {
      printHelp();

    } else if (strncmp("-i", argv[i], 3) == 0) {
      isSetInheritance = 1;
      tag_inheritance = 0;

    } else if (strncmp("-s", argv[i], 3) == 0) {
      skipConfig = 1;

    } else if (strncmp("-T", argv[i], 3) == 0) {
      if (argv[i + 1] == NULL)
        breakDueToMissingArg();
      max_threads = atoi(argv[i + 1]);
      isSetMaxThreads = 1;
      if (max_threads < 0) {
        printf("\n*********************************\n* ! INVALID max-threads "
               "VALUE ! *\n*********************************\n");
        exit(-1);
      }

    } else if (strncmp("-a", argv[i], 3) == 0) {
      isSetHiddenDirInclusion = 1;
      includeHiddenDirs = 1;
    }
  }

  for (int i = 0; i < searchAmount; i++) {
    // printf("-- search: %s", searchString[i]);
  }
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

    FILE *file = fopen(configPath, "w");
    free(file);
    file = NULL;
  } else {
    // printf("config exists: %s \n", configPath);
  }
}
