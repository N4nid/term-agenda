#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char *fixPath(char *path) {
  int pathLen = strlen(path);
  int addLeadingSlash = 0;
  int expandTilde = 0;
  int newSize = -1;
  int homePathSize = -1;

  // addLeadingSlash
  if (strcmp(path + (pathLen - 1), "/") != 0) { // last char isnt a /
    // add a leading / so the paths are right
    addLeadingSlash = 1;
    newSize = pathLen + 1;
  }

  // expandTilde
  if (*path == '~') { // replace ~ with HOME path
    expandTilde = 1;
    homePathSize = strlen(getenv("HOME"));
    newSize += homePathSize;
  }

  if (newSize != -1) {
    char *tmp = realloc(path, newSize);
    if (tmp) {
      path = tmp;
      if (addLeadingSlash == 1) {
        pathLen += 1;
        strncat(path, "/", pathLen);
      }

      if (expandTilde == 1) {
        char *tmp2 = calloc(pathLen, sizeof(char)); // save path in tmp
        memcpy(tmp2, path + 1, pathLen);

        // path to homepath
        memcpy(path, getenv("HOME"), homePathSize);
        strncat(path, tmp2, pathLen); // append original path
        free(tmp2);
        tmp2 = NULL;
      }
    }
  }
  return path;
}

char **split(char *source, char *delimiter, size_t *size) {
  char *token = strtok_r(source, delimiter, &source);
  char **resultArray = NULL;
  size_t counter = 0;

  while (token != NULL) {
    char **tmp = realloc(resultArray, sizeof(resultArray) * (counter + 1));
    if (tmp) { // TODO handel failure
      resultArray = tmp;
    }

    size_t tokeSize = strlen(token) + 1;
    resultArray[counter] = malloc(tokeSize);
    memcpy(resultArray[counter], token, tokeSize);
    // printf("%s\n", resultArray[counter]);
    //  printf("%s\n", token);

    token = strtok_r(source, delimiter, &source);
    counter++;
  }
  *size = counter;

  return resultArray;
}

char *expandTilde(char *path) {
  if (*path == '~') { // replace ~ with HOME path
    char *tmp = path + 1;

    // set path to HOME dir
    int tmpsize = strlen(path);
    int size = strlen(getenv("HOME"));
    // path = calloc(size, sizeof(char));
    path = calloc(size + tmpsize, sizeof(char));
    // path = realloc(path, (size + tmpsize));
    memcpy(path, getenv("HOME"), size);
    strncat(path, tmp, tmpsize);

    // append actual path to HOME dir -> replacing the ~
    // path = realloc(path, (size + strlen(tmp)));
    // strncat(path, tmp, tmpsize + size);
  }
  return path;
}

char *addLeadingSlash(char *path) {
  int pathLen = strlen(path);
  if (strcmp(path + (pathLen - 1), "/") != 0) { // last char isnt a /
    // add a leading / so the paths are right
    // printf("no leading /\n");
    char *tmp = realloc(path, pathLen + 1);
    if (tmp) {
      path = tmp;
      strncat(path, "/", pathLen);
    }
  }
  return path;
}

// borrowed from
// https://stackoverflow.com/questions/1726302/remove-spaces-from-a-string-in-c
void remove_spaces(char *restrict str_trimmed,
                   const char *restrict str_untrimmed) {
  while (*str_untrimmed != '\0') {
    if (!isspace(*str_untrimmed)) {
      *str_trimmed = *str_untrimmed;
      str_trimmed++;
    }
    str_untrimmed++;
  }
  *str_trimmed = '\0';
}
