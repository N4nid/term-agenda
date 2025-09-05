#include "query.c"
#include "util.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

struct fileMeta *files = NULL;
char *searchString = NULL;
int skipConfig = 0;

void freeAllGlobals() {
  free(configPath);
  configPath = NULL;

  free(searchString);
  searchString = NULL;

  for (int i = 0; i < agenda_files_amount; i++) {
    // printf("%s\n", org_agenda_files[i]);
    free(org_agenda_files[i]);
    org_agenda_files[i] = NULL;
  }
  free(org_agenda_files);
  org_agenda_files = NULL;
  // printf("%s\n", cache_dir);
  free(cache_dir);
  cache_dir = NULL;

  for (int i = 0; i < todo_keywords_amount; i++) {
    // printf("%s\n", org_agenda_files[i]);
    free(todo_keywords[i]);
    todo_keywords[i] = NULL;
  }
  free(time_format);
  time_format = NULL;

  free(todo_keywords);
  todo_keywords = NULL;

  if (files != NULL) {
    for (int i = 0; i < agenda_files_amount; i++) {
      freeFileMeta(files[i]);
    }
  }
  free(files);
  files = NULL;
}

void scanFiles() {
  // see vvv for more info
  // https://w3.cs.jmu.edu/kirkpams/OpenCSF/Books/csf/html/ThreadArgs.html

  int max = max_threads;
  if (max_threads == 0) {
    max = agenda_files_amount;
  }
  pthread_t threads[max];
  int threadIndex = 0;

  files = calloc(agenda_files_amount, sizeof(struct fileMeta));
  struct threadWrapper tw[agenda_files_amount];

  // printf("------ agendafile amount: %d\n", agenda_files_amount);

  for (int filesScanned = 0; filesScanned < agenda_files_amount;
       filesScanned += max) {

    // as to not go out of bounds -> set max to files left
    if ((agenda_files_amount - filesScanned) < max) {
      max = (agenda_files_amount - filesScanned);
    }

    // start threads
    for (int i = filesScanned; i < (filesScanned + max); i++) {
      // printf("## starting: %d\n", i);

      // since the threads array only has "max" size
      threadIndex = i - filesScanned;

      tw[i].agendaFilePath = org_agenda_files[i];
      // printf("--------- %s\n", org_agenda_files[i]);
      pthread_create(&threads[threadIndex], NULL, scanFile, (void *)&tw[i]);
    }

    // collect results
    for (int j = filesScanned; j < (filesScanned + max); j++) {
      // printf("++ waiting for: %d\n", j);

      // since the threads array only has "max" size
      threadIndex = j - filesScanned;
      pthread_join(threads[threadIndex], NULL);

      files[j] = tw[j].returnMeta;
    }
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
-p <path to org file[s]>\n\
-a include \"hidden\" dirs (dir starting with a dot)\n\
   has to be set before -p since it whould otherwise not have any effect\n\
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
  SCHED\n\
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
  for (int i = 1; i < argc; i++) {
    // printf("%s\n", argv[i]);
    if (strncmp("-q", argv[i], 3) == 0) {
      if (argv[i + 1] == NULL)
        breakDueToMissingArg();
      int len = strlen(argv[i + 1]);
      searchString = calloc(len + 2, sizeof(char));
      memcpy(searchString, argv[i + 1], len);
      setQuery = 1;
      i++;
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
}

int main(int argc, char *argv[]) {
  atexit(freeAllGlobals);

  createConfig();
  setArgumentOptions(argc, argv);
  if (!skipConfig)
    readConfig();
  else
    printf("skipping config\n");
  // printf("------ agendafile amount: %d\n", agenda_files_amount);
  scanFiles();

  // char *s = "PROP==['state':'wip']";
  //  char *s = "TAG=='a'";
  //   char *s = "!(TAG=='a' | TAG=='b') & (TAG=='c'|TAG=='d')";
  search(searchString, files);
  return 0;
}
