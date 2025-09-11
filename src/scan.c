#define _GNU_SOURCE
#include "config.c"
#include "util.h"
#include <ctype.h>
#include <pthread.h>
#include <sched.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int dateLen = -1;
char *internalTimeFormat = "%Y%m%d%H%M";
int internalTmFLen = 14;

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

int countHeadings(FILE *file) {
  char *line = NULL;
  size_t linesize = 0;
  int lineNum = 0;
  int headingLvl = -1;
  int headingCounter = 0;
  int lvl = -1;

  lineNum = getline(&line, &linesize, file);
  while (lineNum >= 0) { // loop through file
    lvl = getHeadingLvl(line);

    if (lvl > 0) { // is a heading
      headingCounter++;
    }

    lineNum = getline(&line, &linesize, file);
  }
  free(line);
  line = NULL;
  // fclose(file);

  return headingCounter;
}

void freeFileMeta(struct fileMeta file) {
  int debug = 0;

  for (int i = 0; i < file.headingCount; i++) {

    if (file.headings[i].name != NULL) {
      if (debug)
        printf("name: %s", file.headings[i].name);
      free(file.headings[i].name);
      file.headings[i].name = NULL;
    }

    if (file.headings[i].scheduled != NULL) {
      if (debug)
        printf("  scheduled: %s\n", file.headings[i].scheduled);
      free(file.headings[i].scheduled);
      file.headings[i].scheduled = NULL;
    }

    if (file.headings[i].deadline != NULL) {
      if (debug)
        printf("  deadline: %s\n", file.headings[i].deadline);
      free(file.headings[i].deadline);
      file.headings[i].deadline = NULL;
    }

    if (file.headings[i].todokwd != NULL) {
      if (debug)
        printf("  todokwd: %s\n", file.headings[i].todokwd);
      free(file.headings[i].todokwd);
      file.headings[i].todokwd = NULL;
    }

    if (debug)
      printf("  propertiesAmount: %ld\n", file.headings[i].propertiesAmount);
    for (int j = 0; j < file.headings[i].propertiesAmount; j++) {
      for (int k = 0; k < 2; k++) {
        if (debug)
          printf(" |%s|", file.headings[i].properties[j][k]);
        free(file.headings[i].properties[j][k]);
        file.headings[i].properties[j][k] = NULL;
      }
      if (debug)
        printf("\n");
      free(file.headings[i].properties[j]);
      file.headings[i].properties[j] = NULL;
    }
    free(file.headings[i].properties);
    file.headings[i].properties = NULL;

    if (debug)
      printf("  tagAmount: %ld\n", file.headings[i].tagsAmount);
    for (int j = 0; j < file.headings[i].tagsAmount; j++) {
      if (debug)
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

void setDateLen() {
  if (time_format == NULL) {
    return;
  }
  time_t now;
  struct tm *timeinfo;
  char buffer[80];

  time(&now);
  timeinfo = localtime(&now);

  strftime(buffer, sizeof(buffer), time_format, timeinfo);
  dateLen = strlen(buffer) + 1;
  // printf("%s\n", buffer);
  // printf("%d\n", dateLen);
}

// kwd would be SCHEDULED: or DEADLINE:
char *getScheduledDate(char *line, char *kwd, size_t *dateSize,
                       size_t *dateAsInt) {
  if (dateLen == -1) {
    setDateLen();
  }

  char *position = NULL;
  position = strstr(line, kwd);
  if (position == NULL) {
    return NULL;
  }
  // printf("-------------------- %s", position);
  position = strstr(position, "<");
  if (position == NULL) {
    return NULL;
  }
  // printf("-------------------- %s", position);

  int index = 0;
  int found = -1;
  char current = 'a';
  while (current != '\n' || current != '\0') {
    if (current == '>') {
      found = index;
      break;
    }

    index++;
    current = position[index];
  }

  if (found == -1) {
    return NULL;
  }

  *dateSize = index;
  if (*dateSize > dateLen) {
    // printf("cant parse date time format ");
    *dateSize = dateLen;
  }
  char *dateStr = calloc(*dateSize + 1, sizeof(char));

  memcpy(dateStr, position + 1, *dateSize);
  // printf(" .........|%s|\n", dateStr);

  struct tm parsed_time = {0};
  strptime(dateStr, time_format, &parsed_time);
  char buffer[internalTmFLen];
  strftime(buffer, sizeof(buffer), internalTimeFormat, &parsed_time);
  *dateAsInt = atol(buffer);
  // printf("%s - %ld\n", buffer, *dateAsInt);

  return dateStr;
}

char **getProperty(char *line) {
  int lineLen = strlen(line);
  int index = -1;
  int valIndex = -1;
  char *key = NULL;
  char *val = NULL;
  char **KVarr = NULL;
  for (int i = 1; i < lineLen; i++) {
    if (line[i - 1] == ':' && line[i] == ' ') {
      index = i;
      break;
    }
  }
  if (index == -1) {
    return NULL;
  }
  valIndex = index;

  // skip leading spaces
  if (' ' == line[index + 1]) {
    for (int i = index; i < lineLen; i++) {
      if (line[i] != ' ') {
        valIndex = i - 1;
        break;
      }
    }
  }

  KVarr = malloc(2 * sizeof(char *));

  int delta = (lineLen - index) - 1;
  key = calloc(index - 1, sizeof(char));
  val = calloc((lineLen - valIndex), sizeof(char));
  memcpy(key, line + 1, index - 2); // minus ": "
  memcpy(val, line + valIndex + 1, (lineLen - valIndex) - 2);

  KVarr[0] = key;
  KVarr[1] = val;

  // printf("key:%s\nval:%s\n", KVarr[0], KVarr[1]);

  return KVarr;
}

void *scanFile(void *threadWrapperStruct) {
  struct threadWrapper *tw = (struct threadWrapper *)threadWrapperStruct;
  char *path = tw->agendaFilePath;

  struct fileMeta thisFile;
  // struct fileMeta thisFile;
  if (path == NULL) {
    thisFile.isInitialized = 0;
    return NULL;
  }
  if (time_format == NULL) {
    thisFile.isInitialized = 0;
    printf("no time format specified\nplease specify the time format used in "
           "the org files in the config\n");
    return NULL;
  }

  thisFile.path = path;

  FILE *file = fopen(path, "r");
  fpos_t filePos;
  char *line = NULL;
  size_t linesize = 0;
  int lineLen = 0;
  int lineNum = 1;
  int tmpLineNum = 0;

  if (file == NULL) {
    printf("[!] error opening file to be scanned");
    fclose(file);
  }

  int parsingProperties = 0;
  int propertiesCount = 0;
  int hasResetPos = 0;

  fgetpos(file, &filePos);
  int headingLvl = -1;
  int headingCount = -1;
  int totalHeadingsCount = countHeadings(file);
  fsetpos(file, &filePos); // TODO handle failure ??

  thisFile.headings = malloc(sizeof(struct headingMeta) * totalHeadingsCount);
  thisFile.headingCount = totalHeadingsCount;

  lineLen = getline(&line, &linesize, file);
  int lvl = 0;
  while (lineLen >= 0) { // loop through file

    lvl = getHeadingLvl(line);

    if (lvl > 0 && parsingProperties == 0) { // is a heading
      headingCount++;
      thisFile.headings[headingCount].lineNum = lineNum;

      // set thisFile.headings[i].name to line without the *
      size_t newsize = lineLen - (lvl + 1);
      char *lineWithoutAsterisk = line + lvl + 1;
      thisFile.headings[headingCount].name = calloc(newsize, sizeof(char));
      memcpy(thisFile.headings[headingCount].name, lineWithoutAsterisk,
             newsize - 1); // -1 to exclude newline

      // handle todokwd
      size_t todoKwdSize;
      char *todoKwd = getTodoKeyw(lineWithoutAsterisk, &todoKwdSize);
      if (todoKwd != NULL) {
        // printf("size: %ld\n", todoKwdSize);
        thisFile.headings[headingCount].todokwd = malloc(todoKwdSize);
        memcpy(thisFile.headings[headingCount].todokwd, todoKwd, todoKwdSize);
        // printf(" todo: %s\n", thisFile.headings[headingCount].todokwd);
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
        // printf("  tag: %s\n", thisFile.headings[headingCount].tags[j]);
      }

      // set own lvl
      // printf("- %s lvl:%d \n", thisFile.headings[headingCount].name, lvl);
      thisFile.headings[headingCount].lvl = lvl;

      // add inherited_tags
      if (tag_inheritance == 1) {
        setInheritedTags(headingCount, thisFile, lvl);
      }

      // initialize to NULL for this heading (to avoid issues in freeFileMeta())
      thisFile.headings[headingCount].deadline = NULL;
      thisFile.headings[headingCount].scheduled = NULL;
      thisFile.headings[headingCount].propertiesAmount = 0;
      thisFile.headings[headingCount].properties = NULL;
      thisFile.headings[headingCount].deadlineNum = 0;
      thisFile.headings[headingCount].scheduledNum = 0;

    } else {

      // check for ":properties:"
      if (lineLen >= 12 && parsingProperties == 0 && headingCount >= 0) {
        if (strcasestr(line, ":properties:") != NULL) {
          parsingProperties = 1;
          fgetpos(file, &filePos);
          tmpLineNum = lineNum;
          propertiesCount = 0;
        }
      } else if (parsingProperties) {
        // check for ":end:"
        if (strcasestr(line, ":end:") != NULL) {
          if (hasResetPos == 0) {
            hasResetPos = 1;

            thisFile.headings[headingCount].propertiesAmount = propertiesCount;
            thisFile.headings[headingCount].properties =
                calloc(propertiesCount, sizeof(char ***));

            propertiesCount = 0; // reset so that it can be used as a index when
                                 // going over the lines again
            fsetpos(file, &filePos);
            lineNum = tmpLineNum;

          } else {
            hasResetPos = 0;
            parsingProperties = 0;
          }

        } else { // parse key value pairs
          // assumes there is nothing before the key value pairs
          if (':' == line[0]) {
            // printf("%s", line);
            if (hasResetPos == 1) {
              char **KVpair = getProperty(line);
              if (KVpair == NULL) {
                printf("\n---\n");
                printf(" ! FAILED TO GET PROPERTY !\n");
                printf(" is your org-mode syntax valid?\n");
                printf(" failed @ line: %d in: %s\n", lineNum, path);
                printf("---\n\n");

                for (int i = 0; i < propertiesCount; i++) {
                  free(thisFile.headings[headingCount].properties[i]);
                  thisFile.headings[headingCount].properties[i] = NULL;
                }

                free(KVpair);
                KVpair = NULL;
                free(thisFile.headings[headingCount].properties);
                thisFile.headings[headingCount].properties = NULL;
                thisFile.headings[headingCount].propertiesAmount = 0;
                parsingProperties = 0;
                propertiesCount = 0;
                hasResetPos = 0;
                continue;
              }
              // printf("key: %s\nval:%s\n", KVpair[0], KVpair[1]);
              thisFile.headings[headingCount].properties[propertiesCount] =
                  KVpair;
            }

            propertiesCount++;
          }
        }
      }

      // handle scheduled dates
      // 25 is the min of chars needed to have a valid scheduled syntax
      // eg: "SCHEDULED:<"+dateLength+">"
      if (lineLen >= 25 && parsingProperties == 0 && headingCount >= 0) {
        size_t dateSize;
        char *dateSched = NULL;
        size_t schedNum = 0;
        char *dateDead = NULL;
        size_t deadNum = 0;

        dateSched = getScheduledDate(line, "SCHEDULED:", &dateSize, &schedNum);
        dateDead = getScheduledDate(line, "SCHEDULED:", &dateSize, &deadNum);
        if (dateSched != NULL) {
          thisFile.headings[headingCount].scheduled =
              calloc(dateSize, sizeof(char));
          memcpy(thisFile.headings[headingCount].scheduled, dateSched,
                 dateSize - 1);

          thisFile.headings[headingCount].scheduledNum = schedNum;
          // printf("scheduled: %s\n",
          // thisFile.headings[headingCount].scheduled);
          free(dateSched);
          dateSched = NULL;
        }
        if (dateDead != NULL) {
          thisFile.headings[headingCount].deadline =
              calloc(dateSize, sizeof(char));
          memcpy(thisFile.headings[headingCount].deadline, dateDead,
                 dateSize - 1);

          thisFile.headings[headingCount].deadlineNum = deadNum;
          // printf("deadline: %s\n", thisFile.headings[headingCount].deadline);
          free(dateDead);
          dateDead = NULL;
        }
      }
    }

    lineNum++;
    lineLen = getline(&line, &linesize, file);
  }
  fclose(file);
  // printf("------------\n");

  // freeFileMeta(thisFile);

  free(line);
  line = NULL;

  // printf("%s\n", path);
  thisFile.isInitialized = 1;
  tw->returnMeta = thisFile;
  // return thisFile;
  return NULL;
  // pthread_exit(&thisFile);
}
