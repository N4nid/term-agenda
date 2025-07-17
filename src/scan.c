#include "config.c"
#include <sched.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct headingMeta {
  size_t totalSize;
  char *name;
  char **tags;
  char *todokwd; // fe. TODO, DONE
  char *scheduled;
  char *deadline;
  char **properties;
};
struct fileMeta {
  char *path;
  int headingCount;
  struct headingMeta *headings; // array of heading meta data
};

int getHeadingLvl(char *headline) {
  // returns the amount of "*" in front of a heading
  // if it is not a heading it will return -1
  // assumes the first character is a "*"

  if (headline == NULL || '*' != headline[0]) {
    return -1;
  }
  char currentChar = headline[0];
  int counter = 0;

  while (currentChar == '*') {
    counter++;
    currentChar = headline[counter];
  }

  if (' ' == headline[counter]) {
    return counter;
  }
  return -1;
}

int countHeadings(char *path) {
  FILE *file = fopen(path, "r");
  char *line;
  size_t linesize = 0;
  int lineNum = 0;
  int headingLvl = -1;
  int headingCounter = 0;

  lineNum = getline(&line, &linesize, file);
  while (lineNum >= 0) { // loop through file
    int lvl = getHeadingLvl(line);

    if (lvl > 0) { // is a heading
      headingCounter++;
    }

    lineNum = getline(&line, &linesize, file);
  }
  free(line);
  line = NULL;
  fclose(file);

  return headingCounter;
}

void freeFileMeta(struct fileMeta file) {
  for (int i = 0; i < file.headingCount; i++) {
    if (file.headings[i].name != NULL) {
      printf("name: %s", file.headings[i].name);
      free(file.headings[i].name);
      file.headings[i].name = NULL;
    }

    if (file.headings[i].todokwd != NULL) {
      printf("  todokwd: %s\n", file.headings[i].todokwd);
      free(file.headings[i].todokwd);
      file.headings[i].todokwd = NULL;
    }
  }
  free(file.headings);
  file.headings = NULL;
}

char *getTodoKeyw(char *heading, size_t *todoKwdSize) {
  // assumes that there are no spaces before the todo keywd
  int headingLenght = strlen(heading);
  size_t todoKeywdLength;
  for (int i = 0; i < todo_keywords_amount; i++) {
    todoKeywdLength = strlen(todo_keywords[i]);
    if (headingLenght >= todoKeywdLength) {
      if (strncmp(todo_keywords[i], heading, todoKeywdLength) == 0) {
        *todoKwdSize = todoKeywdLength + 1;
        return todo_keywords[i];
      }
    }
  }

  return NULL;
}

void scanFile(char *path) {
  if (path == NULL) {
    return;
  }

  FILE *file = fopen(path, "r");
  char *line;
  size_t linesize = 0;
  int lineNum = 0;

  if (file == NULL) {
    printf("[!] error opening file to be scanned");
    fclose(file);
  }

  int headingLvl = -1;
  int headingCount = 0;
  int totalHeadingsCount = countHeadings(path);

  struct fileMeta thisFile;
  thisFile.headings = malloc(sizeof(struct headingMeta) * totalHeadingsCount);
  thisFile.headingCount = totalHeadingsCount;

  lineNum = getline(&line, &linesize, file);
  while (lineNum >= 0) { // loop through file
    int lvl = getHeadingLvl(line);

    if (lvl > 0) { // is a heading
      // set thisFile.headings[i].name to line without the *
      size_t newsize = linesize - (lvl + 1);
      char *lineWithoutAsterisk = line + lvl + 1;
      thisFile.headings[headingCount].name = malloc(newsize);
      memcpy(thisFile.headings[headingCount].name, lineWithoutAsterisk,
             newsize);

      size_t todoKwdSize;
      char *todoKwd = getTodoKeyw(lineWithoutAsterisk, &todoKwdSize);
      if (todoKwd != NULL) {
        printf("size: %ld\n", todoKwdSize);
        thisFile.headings[headingCount].todokwd = malloc(todoKwdSize);
        memcpy(thisFile.headings[headingCount].todokwd, todoKwd, todoKwdSize);
        printf("todo: %s\n", thisFile.headings[headingCount].todokwd);
      } else { // has to be NULL since it by default points somewhere else.
        thisFile.headings[headingCount].todokwd = NULL;
      }

      printf("%s lvl:%d \n", thisFile.headings[headingCount].name, lvl);

      headingCount++;
    }

    lineNum = getline(&line, &linesize, file);
  }
  fclose(file);
  printf("------------\n");

  freeFileMeta(thisFile);

  free(line);
  line = NULL;

  printf("%s\n", path);
}
