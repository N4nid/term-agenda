#include "scan.c"
#include "util.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

void freeAllGlobals() {
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

  free(todo_keywords);
  todo_keywords = NULL;
}

void testThreads() {
  // see vvv for more info
  // https://w3.cs.jmu.edu/kirkpams/OpenCSF/Books/csf/html/ThreadArgs.html

  int max = max_threads;
  pthread_t threads[max];
  struct fileMeta files[agenda_files_amount];
  struct threadWrapper tw[agenda_files_amount];

  printf("------ agendafile amount: %d\n", agenda_files_amount);
  printf("------ maxThreads: %d\n", max);

  for (int filesScanned = 0; filesScanned < agenda_files_amount;
       filesScanned += max) {

    // as to not go out of bounds -> set max to files left
    if ((agenda_files_amount - filesScanned) < max) {
      max = (agenda_files_amount - filesScanned);
    }

    // start threads
    for (int i = filesScanned; i < (filesScanned + max); i++) {
      printf("## starting: %d\n", i);
      tw[i].agendaFilePath = org_agenda_files[i];
      pthread_create(&threads[i], NULL, scanFile, (void *)&tw[i]);
    }
    // collect results
    for (int j = filesScanned; j < (filesScanned + max); j++) {
      printf("++ waiting for: %d\n", j);
      pthread_join(threads[j], NULL);

      files[j] = tw[j].returnMeta;
      freeFileMeta(files[j]);
    }
  }
}

int main(int argc, char *argv[]) {
  createConfig();
  readConfig();
  testThreads();

  freeAllGlobals();
  return 0;
}
