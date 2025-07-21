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

  int max = agenda_files_amount;
  pthread_t threads[max];
  struct fileMeta files[agenda_files_amount];
  struct threadWrapper tw[agenda_files_amount];

  printf("------ agendafiel amount: %d\n", max);
  for (int i = 0; i < max; i++) {
    printf("starting: %d\n", i);
    tw[i].agendaFilePath = org_agenda_files[i];
    pthread_create(&threads[i], NULL, scanFile, (void *)&tw[i]);
  }
  for (int j = 0; j < max; j++) {
    printf("waiting for: %d\n", j);
    pthread_join(threads[j], NULL);

    files[j] = tw[j].returnMeta;
    freeFileMeta(files[j]);
  }
}

int main(int argc, char *argv[]) {
  createConfig();
  readConfig();
  testThreads();

  freeAllGlobals();
  return 0;
}
