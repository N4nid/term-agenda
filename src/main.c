#include "scan.c"
#include "util.h"
#include <pthread.h>
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

  int max = 2;
  pthread_t threads[max];
  pthread_create(&threads[0], NULL, scanFile, (void *)org_agenda_files[0]);
  pthread_create(&threads[1], NULL, scanFile, (void *)org_agenda_files[1]);
  pthread_join(threads[1], NULL);
  pthread_join(threads[0], NULL);
  //  for (int i = 1; i < max; i++) {
  //    printf("starting: %d\n", i);
  //    // pthread_create(&threads[i], NULL, test1, &i);
  //  }
  //  for (int j = 1; j < max; j++) {
  //    printf("waiting for: %d\n", j);
  //    pthread_join(threads[j], NULL);
  //  }
  // pthread_create(&threads[0], NULL, test1, test);
}

int main(int argc, char *argv[]) {
  createConfig();
  readConfig();
  testThreads();

  freeAllGlobals();
  return 0;
}
