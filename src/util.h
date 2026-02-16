#ifndef UTIL_H
#define UTIL_H
#include <stddef.h>
struct searchOption {
  char *searchString;
  char *agenda_files_path;
  int recursive_adding;
  int includeHiddenDirs;
  int tag_inheritance; // 1 true, 0 false
  int max_threads;
  char *todo_keywordsCSV; // comma seperated list of todo-keywords
  char *time_format;      // fe. %Y-%m-%d
  // char *cache_dir;
};
struct headingMeta {
  int lineNum;
  int lvl; // the heading lvl (eg. how many "*" there are)
  char *name;
  char **tags;
  size_t tagsAmount;
  char *todokwd; // fe. TODO, DONE
  char *scheduled;
  size_t scheduledNum;
  char *deadline;
  size_t deadlineNum;
  char ***properties;
  size_t propertiesAmount;

  // is later set in toFlatArray()
  char *path;
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
