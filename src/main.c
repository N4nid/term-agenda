#include "query.c"
#include "util.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

struct fileMeta *files = NULL;

void freeAllGlobals() {
  free(configPath);
  configPath = NULL;

  if (searchString != NULL) {
    for (int i = 0; i < searchAmount; i++) {
      // printf("%s\n", org_agenda_files[i]);
      free(searchString[i]);
      searchString[i] = NULL;
    }
  }
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

  free(customSearch);
  customSearch = NULL;
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

int main(int argc, char *argv[]) {
  atexit(freeAllGlobals);

  createConfig();
  setArgumentOptions(argc, argv);
  if (skipConfig != 1)
    readConfig();
  else
    printf("skipping config\n");

  // no need to do more if there is no search specified
  if (searchAmount == 0) {
    return 0;
  }

  // printf("------ agendafile amount: %d\n", agenda_files_amount);
  scanFiles();

  // char *s = "PROP==['state':'wip']";
  //  char *s = "TAG=='a'";
  //   char *s = "!(TAG=='a' | TAG=='b') & (TAG=='c'|TAG=='d')";
  if (customSearch == NULL) {
    for (int i = 0; i < searchAmount; i++) {
      search(searchString[i], files);
    }
  }
  return 0;
}
