#include "scan.c"
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

// void testThreads() {
//
// int max = 10;
// pthread_t threads[max];
// see https://hpc-tutorials.llnl.gov/posix/passing_args/
// pthread_create(&thread1, NULL, testFunc, (void *)configPath);

//  for (int i = 1; i < max; i++) {
//    printf("starting: %d\n", i);
//    pthread_create(&threads[i], NULL, test1, &i);
//  }
//  for (int j = 1; j < max; j++) {
//    printf("waiting for: %d\n", j);
//    pthread_join(threads[j], NULL);
//  }
// pthread_create(&threads[0], NULL, test1, test);
// pthread_join(threads[0], NULL);
//}

int main(int argc, char *argv[]) {
  createConfig();
  readConfig();
  scanFile(org_agenda_files[0]);

  freeAllGlobals();
  return 0;
}
