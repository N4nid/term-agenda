#include "config.c"
#include "util.h"
#include <ctype.h>
#include <sched.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

    printf("  tagAmount: %ld\n", file.headings[i].tagsAmount);
    for (int j = 0; j < file.headings[i].tagsAmount; j++) {
      printf("  tag: %s\n", file.headings[i].tags[j]);
      free(file.headings[i].tags[j]);
      file.headings[i].tags[j] = NULL;
    }
    free(file.headings[i].tags);
    file.headings[i].tags = NULL;
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

char **getTags(char *heading, size_t *amount) {
  size_t headingLen = strlen(heading);
  char **tagArray = NULL;
  int between = 0;
  int startPos = -1;
  int endPos = -1;
  int counter = 0;

  for (int i = 0; i < headingLen; i++) {
    // printf("%c", current);
    if (heading[i] == ':') {
      if (between == 1) { // has found a tag
        between = 0;
        endPos = i;
        i--; // so that the : can get used as a starting point for further
             // potential tags (:asd:wow:)

        char **tmp = realloc(tagArray, sizeof(tagArray) * (counter + 1));
        if (tmp) { // TODO handel failure
          tagArray = tmp;
        }

        size_t tagSize = endPos - startPos;
        tagArray[counter] = malloc(tagSize + 1);
        memcpy(tagArray[counter], heading + startPos + 1, tagSize);
        tagArray[counter][tagSize - 1] = '\0';
        // printf("TAGS: %s\n", tagArray[counter]);

        counter++;
      } else {
        startPos = i;
        between = 1;
      }
    }

    if (between == 1) {
      if (isspace(heading[i])) {
        // reset since :wow : is not a valid tag
        // there are more characters which would make a tag invalid, but i dont
        // see it as necessary to include those cases. Atleast for now
        between = 0;
      }
    }
  }

  *amount = counter;
  return tagArray;
}

void setInheritedTags(int currentIndex, struct fileMeta file, int lvl) {
  if ((currentIndex - 1) >= 0 && lvl > 1) {
    // dont have to search if already top lvl
    // linear search upwards to find first "parent" to inherit from
    // printf("  own LVL: %d\n", lvl);
    for (int i = currentIndex - 1; i >= 0; i--) {
      // printf("  searching: %s", file.headings[i].name);
      if (lvl > file.headings[i].lvl) {
        // printf("  found: %d %s", file.headings[i].lvl,
        // file.headings[i].name);
        //  set inherited_tags

        size_t thisTagsAmount = file.headings[currentIndex].tagsAmount;
        size_t otherTagsAmount = file.headings[i].tagsAmount;
        size_t newAmount = thisTagsAmount + otherTagsAmount;

        char **tmp = realloc(file.headings[currentIndex].tags,
                             newAmount * sizeof(char *));

        if (tmp) { // TODO add failure handeling
          file.headings[currentIndex].tags = tmp;
          for (int j = thisTagsAmount; j < newAmount; j++) {
            size_t tagSize =
                strlen(file.headings[i].tags[j - thisTagsAmount]) + 1;

            file.headings[currentIndex].tags[j] = malloc(tagSize);

            memcpy(file.headings[currentIndex].tags[j],
                   file.headings[i].tags[j - thisTagsAmount], tagSize);
          }

          file.headings[currentIndex].tagsAmount = newAmount;
        }

        //        for (int j = 0; j < newAmount; j++) {
        //          printf("test---------- %s\n",
        //          file.headings[currentIndex].tags[j]);
        //        }
        break;
      }
    }
  }
}

// struct fileMeta scanFile(char *path) {
void *scanFile(void *path) {
  struct fileMeta thisFile;
  if (path == NULL) {
    thisFile.isInitialized = 0;
    return NULL;
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

      // handle todokwd
      size_t todoKwdSize;
      char *todoKwd = getTodoKeyw(lineWithoutAsterisk, &todoKwdSize);
      if (todoKwd != NULL) {
        // printf("size: %ld\n", todoKwdSize);
        thisFile.headings[headingCount].todokwd = malloc(todoKwdSize);
        memcpy(thisFile.headings[headingCount].todokwd, todoKwd, todoKwdSize);
        printf("todo: %s\n", thisFile.headings[headingCount].todokwd);
      } else { // has to be NULL since it by default points somewhere else.
        thisFile.headings[headingCount].todokwd = NULL;
      }

      // handle tags
      size_t tagAmount = -1;
      thisFile.headings[headingCount].tags =
          getTags(lineWithoutAsterisk, &tagAmount);
      thisFile.headings[headingCount].tagsAmount = tagAmount;
      // printf("  tagAmount: %ld\n",
      // thisFile.headings[headingCount].tagsAmount);
      for (int j = 0; j < thisFile.headings[headingCount].tagsAmount; j++) {
        printf("  tag: %s\n", thisFile.headings[headingCount].tags[j]);
      }

      // set own lvl
      printf("- %s lvl:%d \n", thisFile.headings[headingCount].name, lvl);
      thisFile.headings[headingCount].lvl = lvl;

      // add inherited_tags
      if (tag_inheritance == 1) {
        setInheritedTags(headingCount, thisFile, lvl);
      }

      headingCount++;
    }

    lineNum = getline(&line, &linesize, file);
  }
  fclose(file);
  printf("------------\n");

  // freeFileMeta(thisFile);

  free(line);
  line = NULL;

  printf("%s\n", path);
  thisFile.isInitialized = 1;
  // return thisFile;
  return NULL;
}
