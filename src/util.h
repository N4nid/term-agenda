#ifndef UTIL_H
#define UTIL_H
#include <stddef.h>
struct headingMeta {
  int lvl; // the heading lvl (eg. how many "*" there are)
  char *name;
  char **tags;
  size_t tagsAmount;
  char *todokwd; // fe. TODO, DONE
  char *scheduled;
  char *deadline;
  char ***properties;
  size_t propertiesAmount;
};
struct fileMeta {
  int isInitialized;
  char *path;
  int headingCount;
  struct headingMeta *headings; // array of heading meta data
};
struct threadWrapper {
  char *agendaFilePath;
  struct fileMeta returnMeta;
};

#endif
