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

  free(cache_dir);
  cache_dir = NULL;

  free(time_format);
  time_format = NULL;

  free(agenda_files_path);
  agenda_files_path = NULL;

  free(todo_keywordsCSV);
  todo_keywordsCSV = NULL;

  freeSearchOption(searchOptions);

  if (files != NULL) {
    for (int i = 0; i < agenda_files_amount; i++) {
      freeFileMeta(files[i]);
    }
    free(files);
    files = NULL;
  }

  if (searchString != NULL) {
    freeStrArray(&searchString, &searchAmount);
  }

  if (org_agenda_files != NULL) {
    freeStrArray(&org_agenda_files, &agenda_files_amount);
  }

  if (todo_keywords != NULL) {
    freeStrArray(&todo_keywords, &todo_keywords_amount);
  }

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

void setDefaults() {
  char *kwds = "TODO,DONE";
  todo_keywordsCSV = copy2str(kwds);
  char *format = "%Y-%m-%d";
  time_format = copy2str(format);
}

void freeAfterSearch() {
  // free stuff because it would be lost otherwise
  // only necessary for customSearches
  // since there the config values change per search (possibly)

  if (files != NULL) {
    for (int i = 0; i < agenda_files_amount; i++) {
      freeFileMeta(files[i]);
    }
    free(files);
    files = NULL;
  }

  free(customSearchString);
  customSearchString = NULL;
}

int main(int argc, char *argv[]) {
  atexit(freeAllGlobals);

  setDefaults();
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

  if (customSearch == NULL) { // "normal" search
    scanFiles();
    for (int i = 0; i < searchAmount; i++) {
      search(searchString[i], files);
    }
  } else { // custom Search

    for (int i = 0; i < searchAmount; i++) {
      loadOptions(searchOptions[i]);
      scanFiles();
      search(customSearchString, files);
      freeAfterSearch();
    }
  }

  return 0;
}
