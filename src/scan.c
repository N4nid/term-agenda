#include <stdio.h>
#include <stdlib.h>

struct headingMeta {
  char *name;
  char **tags;
  char *status; // fe. TODO, DONE
  char *scheduled;
  char *deadline;
  char **properties;
};
struct fileMeta {
  char *path;
  struct headingMeta **headings; // array of heading meta data
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

void scanFile(char *path) {
  if (path == NULL) {
    return;
  }

  FILE *file = fopen(path, "r");
  char *line;
  size_t size = 0;
  int lineNum = 0;

  if (file == NULL) {
    printf("[!] error opening file to be scanned");
    fclose(file);
  }

  int headingLvl = -1;

  lineNum = getline(&line, &size, file);
  while (lineNum >= 0) { // loop through file
    int lvl = getHeadingLvl(line);
    if (lvl > 0) { // is a heading
      printf("%s lvl:%d \n", line, lvl);
      // handel heading
    }

    lineNum = getline(&line, &size, file);
  }
  fclose(file);
  printf("%s\n", path);
}
