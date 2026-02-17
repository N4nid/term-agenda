#include "scan.c"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node {
  char *value;
  int type;
  int field; // fe. todo or property
  int matchType;
  struct node *left;
  struct node *right;
} newNode = {NULL, 0, 0, 0, NULL, NULL};

// types of nodes constants
const int and = 1;
const int or = 2;
const int not = 3;
const int statement = 4;
// fields constants
const int tag = 1;
const int todoKWD = 2;
const int scheduled = 3;
const int deadline = 4;
const int property = 5;
const int name = 6;
// matchTypes constants
const int exact = 1;       // ==
const int contains = 2;    // ~=
const int greaterOrEq = 3; // >=
const int lesserOrEq = 4;  // <=

int nodeAmount = 0;
struct node *nodes;
int headingAmount = 0;
struct headingMeta *headings;

void printResult(int *result, int len) {
  // printf("matches:\n");
  for (int i = 0; i < len; i++) {
    if (result[i] == 1) {
      // printf("%s", headings[i].name);
      printf("%d %s -> %s\n", headings[i].lineNum, headings[i].name,
             headings[i].path);
    }
  }
  printf("\n");
}

void recPrintTree(struct node *node, int isLeft, int depth) {
  if (node == NULL) {
    return;
  }
  char side = 'r';
  if (isLeft)
    side = 'l';
  if (node->type == statement) {
    printf("%d|%c -YOO: %d %s\n", depth, side, node->type, node->value);
    //  free(node->value);
  } else {
    printf("%d|%c -YOO: %d \n", depth, side, node->type);
  }

  depth++;
  recPrintTree(node->left, 1, depth);
  recPrintTree(node->right, 0, depth);
  // free(node);
  // node = NULL;
  return;
}

char *getBetweenBrackets(char *input, int *endPos) {
  // printf(" - inpt: %s\n", input);
  int index = 1;
  int bracketDepth = 1;
  char current = input[index];

  while (current != '\0') {
    if (current == '(') {
      bracketDepth++;
    } else if (current == ')') {
      bracketDepth--;
    } else if (bracketDepth == 0) {
      break;
    }

    index++;
    current = input[index];
  }

  if (bracketDepth != 0) {
    return NULL;
  }

  char *out = calloc(index, sizeof(char));
  memcpy(out, input + 1, index - 2);

  *endPos = index - 1;
  return out;
}

// gets next parsable chunk of text
void setNext(char *stringIn, char **stringOut, int *pos, int *isStatement,
             int *outLen) {
  // filter one char long stuff
  char current = stringIn[*pos];

  // skip spaces
  while (current == ' ' || current == ')') {
    *pos += 1;
    current = stringIn[*pos];
  }

  if (current == '(' || current == ')' || current == '&' || current == '|' ||
      current == '!') {
    *stringOut = calloc(2, sizeof(char));
    memcpy(*stringOut, &current, 1);
    *pos += 1;
    *isStatement = 0;
    *outLen = 1;
    return;
  } else if (current == '\0') {
    *stringOut = NULL;
    return;
  }

  int nextDelim = *pos;
  // printf("  cur: %c\n", current);
  while (current != '\0' && current != '\'' && current != '&' &&
         current != '|' && current != '!' && current != '[' && current != ')') {
    nextDelim++;
    current = stringIn[nextDelim];
    // printf("  cur: %c\n", current);
  }
  // printf("at %d next delim: %c\n", nextDelim, stringIn[nextDelim]);
  if (current == '\'' || current == '[') {
    char forbidden = '\'';
    if (current == '[')
      forbidden = ']';

    nextDelim++;
    current = stringIn[nextDelim];
    // printf("  cur: %c\n", current);
    while (current != forbidden && current != '\0') {
      nextDelim++;
      current = stringIn[nextDelim];
      // printf("  cur: %c\n", current);
    }
  }

  int delta = (nextDelim - *pos) + 2;

  *outLen = delta - 1;
  *isStatement = 1; // is most likely a statment
  *stringOut = calloc(delta, sizeof(char));
  memcpy(*stringOut, stringIn + *pos, delta - 1);
  *pos = nextDelim + 1;
}

int setStatmentNode(char *input, int inputLen, struct node *node) {
  int field = -1;
  char fields[6][6] = {"TAG", "TODO", "SCHED", "DEAD", "PROP", "NAME"};
  int lens[6] = {3, 4, 5, 4, 4, 4};

  for (int i = 0; i < 6; i++) {
    if (strncmp(input, fields[i], lens[i]) == 0) {
      // printf("- IS A: %s\n", fields[i]);
      field = i + 1; // see the constants at the top
    }
  }
  if (field == -1)
    return 0;

  node->type = statement;
  node->field = field;

  // find match type
  int index = lens[field - 1];
  char current = input[index];
  while (current != '=' && current != '~' && current != '<' && current != '>') {
    // printf(" curr; %c\n", current);
    index++;
    current = input[index];
    if (current == '\0')
      return 0;
  }
  if (current == '~')
    node->matchType = contains;
  else if (current == '=')
    node->matchType = exact;
  else if (current == '<')
    node->matchType = lesserOrEq;
  else if (current == '>')
    node->matchType = greaterOrEq;

  index += 2;
  current = input[index];
  while (current == ' ') {
    index++;
    current = input[index];
    // printf(" curr; %c\n", current);
  }
  int delta = inputLen - index;
  node->value = calloc(delta + 1, sizeof(char));
  memcpy(node->value, input + index, delta);
  return 1;
}

void breakDueToInvalidInput(char *msg) {
  printf("\n! INVALID INPUT !\n");
  printf("! %s !\n", msg);
  exit(1);
}

int countNodes(char *input) {
  // printf("%s\n", input);
  int amount = 0;
  int pos = 0;
  int isStatement = 2;
  int nextLen = 0;
  char *next = "";
  int len = strlen(input);

  while (next != NULL) {
    setNext(input, &next, &pos, &isStatement, &nextLen);
    // printf("pos: %d next: [%s] len: %d\n", pos, next, nextLen);
    if (isStatement == 1) {
      isStatement = 0;
      if (nextLen < 7)
        breakDueToInvalidInput("[c] too short for valid syntax");
      amount++;
    } else if (next != NULL) { // should be a operator or bracket
      if (next[0] != '(' && next[0] != ')') {
        amount++;
      }
    }

    free(next);
  }

  return amount;
}

void string2searchTree(char *search, struct node *tree, int *nodeIndex) {
  // printf("\n%s\n", search);
  int pos = 0;
  int isStatement = 2;
  int nextLen = 0;
  int lastOpType = 0;
  int lastNodeType = 0;
  char *next = "";
  int lastInitialized = -1;

  struct node *top = NULL;
  struct node *lastNode = NULL;
  struct node *lastOp = NULL;
  // struct node nodes[nodeAmount];

  while (next != NULL) {
    setNext(search, &next, &pos, &isStatement, &nextLen);
    // printf("pos: %d next: [%s] len: %d\n", pos, next, nextLen);

    if (isStatement == 1 && next != NULL) {
      isStatement = 0;
      if (nextLen < 7)
        breakDueToInvalidInput("too short for valid syntax");

      if (setStatmentNode(next, nextLen, &nodes[*nodeIndex])) {
        // printf(" val: %s\n", testNode.value);
        lastNode = &nodes[*nodeIndex];
        nodes[*nodeIndex].left = NULL;
        nodes[*nodeIndex].right = NULL;

        if (lastNodeType == and || lastNodeType == or || lastNodeType == not) {
          lastOp->right = lastNode;
        }
        if (top == NULL) {
          top = &nodes[*nodeIndex];
          // printf(" Im Top now :3\n");
        }

        lastNodeType = statement;
        *nodeIndex += 1;
      } else {
        breakDueToInvalidInput("not a valid statement");
      }
    } else if (next != NULL) { // should be a operator or bracket

      if (next[0] != '(' && next[0] != ')') {
        if (top == NULL || (top != NULL && top->type == statement)) {
          top = &nodes[*nodeIndex];
          // printf(" Im Top now :3\n");
        }
      } else if (next[0] == '(') {
        struct node tmp;
        int len = 0;
        char *between = getBetweenBrackets(search + pos - 1, &len);
        string2searchTree(between, &tmp, nodeIndex);
        lastNode = tmp.left;

        if (lastNodeType == and || lastNodeType == or || lastNodeType == not) {
          lastOp->right = lastNode;
        }

        lastNodeType = statement;
        pos += len;
        // printf("- change: %s", search + pos);
        free(between);
      }

      if (next[0] == '&') {
        nodes[*nodeIndex].left = lastNode;
        nodes[*nodeIndex].type = and;

        if (lastOpType == not || lastOpType == and) {
          nodes[*nodeIndex].left = lastOp;
          if (lastOp == top) {
            top = &nodes[*nodeIndex];
            // printf(" Im the Top now :3\n");
          }
        } else if (lastOpType == or) {
          nodes[*nodeIndex].left = lastOp->right;
          lastOp->right = &nodes[*nodeIndex];
        } else {
          nodes[*nodeIndex].left = lastNode;
        }

        lastNodeType = and;
        lastOpType = and;
        lastOp = &nodes[*nodeIndex];
        *nodeIndex += 1;
      } else if (next[0] == '|') {
        nodes[*nodeIndex].type = or;

        if (lastOpType != 0) {
          nodes[*nodeIndex].left = lastOp;
          if (lastOp == top) {
            top = &nodes[*nodeIndex];
            // printf(" Im the Top now :3\n");
          }
        } else {
          nodes[*nodeIndex].left = lastNode;
        }

        lastOpType = or;
        lastNodeType = and;
        lastOp = &nodes[*nodeIndex];
        *nodeIndex += 1;
      } else if (next[0] == '!') {
        nodes[*nodeIndex].type = not;

        if (lastOp != NULL) {
          lastOp->right = &nodes[*nodeIndex];
        }

        lastOpType = not;
        lastNodeType = not;
        lastOp = &nodes[*nodeIndex];
        *nodeIndex += 1;
      }
    }

    free(next);
  }
  tree->left = top;
  tree->right = NULL;
  // printf("\n");
}

void freeTree() {
  for (int i = 0; i < nodeAmount; i++) {
    // printf("YO: %d\n", nodes[i].type);
    if (nodes[i].type == statement) {
      free(nodes[i].value);
    }
  }
  free(nodes);
}

// merges second into first
// both must have the same len
void merge(int type, int *first, int *second, int len) {
  if (type == and) {
    for (int i = 0; i < len; i++) {
      if (first[i] && second[i]) {
        first[i] = 1;
      } else {
        first[i] = 0;
      }
    }
  } else if (type == or) {
    for (int i = 0; i < len; i++) {
      if (first[i] || second[i]) {
        first[i] = 1;
      } else {
        first[i] = 0;
      }
    }
  }
}

// gets text till next occurence of the char
char *getBetweenChar(char *input, char delim, int *pos) {
  int indexStart = 0;
  int index = 0;
  char current = input[index];

  // find pos of delim
  while (current != delim && current != '\0') {
    index++;
    current = input[index];
    if (current == '\0') {
      return NULL;
    }
  }
  index += 1; // as to be able to read until the next delim
  current = input[index];
  indexStart = index;

  while (current != delim && current != '\0') {
    index++;
    current = input[index];
    if (current == '\0') {
      return NULL;
    }
  }

  char *between = calloc(index, sizeof(char));
  memcpy(between, input + indexStart, index - indexStart);
  // printf("   between:%s\n", between);

  *pos = index + 1;

  return between;
}

// for todoKWD,name,scheduled,deadline
char *getSimpleField(int field, struct headingMeta *heading) {
  if (field == todoKWD) {
    return heading->todokwd;
  } else if (field == name) {
    return heading->name;
  } else if (field == deadline) {
    return heading->deadline;
  } else if (field == scheduled) {
    return heading->scheduled;
  }
  return NULL;
}

int matchNum(long this, long should, int matchType) {
  if (matchType == greaterOrEq) {
    if (this >= should) {
      return 1;
    }
  } else if (matchType == lesserOrEq) {
    if (this <= should) {
      return 1;
    }
  }
  return 0;
}

int matchString(char *this, char *should, int matchType) {
  if (matchType == exact) {
    if (strcmp(this, should) == 0) {
      return 1;
    }
  } else if (matchType == contains) {
    char *pos = strstr(this, should);
    if (pos != NULL) {
      return 1;
    }
  }
  return 0;
}

void check(int *result, int field, char *value, int matchType) {
  char *val = value;
  int gotBetween = 0;
  int pos = 0;
  if (value[0] == '\'') {
    val = getBetweenChar(value, '\'', &pos);
    gotBetween = 1;
  }
  char *propField = NULL;
  char *propVal = NULL;
  if (field == property) {
    propField = getBetweenChar(value, '\'', &pos);
    propVal = getBetweenChar(value + pos, '\'', &pos);
  }

  for (int i = 0; i < headingAmount; i++) {
    // check simple string values
    if (field != property && field != tag) {
      char *headingVal = getSimpleField(field, &headings[i]);
      if (headingVal == NULL)
        continue;
      if (matchType == exact || matchType == contains) {
        result[i] = matchString(headingVal, val, matchType);
        // printf("|%s| = |%s|\n", headingVal, val);
      } else if (field == scheduled || field == deadline) {
        long headingNumVal = 0;
        long valNum = -1;
        if (field == scheduled)
          headingNumVal = headings[i].scheduledNum;
        else
          headingNumVal = headings[i].deadlineNum;
        // printf("|%s| = |%s|\n", headingVal, val);

        // convert given date into num
        struct tm parsed_time = {0};
        strptime(val, time_format, &parsed_time);
        char buffer[internalTmFLen];
        strftime(buffer, sizeof(buffer), internalTimeFormat, &parsed_time);
        valNum = atol(buffer);
        // printf("|%ld| = |%ld|\n", headingNumVal, valNum);
        // printf("------\n");

        result[i] = matchNum(headingNumVal, valNum, matchType);
      } else { // can only match other fields with exact or conatins
        result[i] = 0;
      }

    } else {
      // check tags
      if (field == tag) {
        for (int j = 0; j < headings[i].tagsAmount; j++) {
          if (matchString(headings[i].tags[j], val, matchType)) {
            result[i] = 1;
            break;
          }
        }
        // check properties
      } else {
        for (int j = 0; j < headings[i].propertiesAmount; j++) {
          int hasVal = 0;
          int hasField = 0;

          hasField =
              matchString(headings[i].properties[j][0], propField, matchType);
          hasVal =
              matchString(headings[i].properties[j][1], propVal, matchType);

          if (hasField && hasVal) {
            result[i] = 1;
            break;
          }
        }
      }
    }
  }

  if (gotBetween)
    free(val);
  if (pos != 0) {
    free(propField);
    free(propVal);
  }
}

void computeResult(struct node *searchTree, int *results, int len) {
  if (searchTree == NULL) {
    return;
  }
  int type = searchTree->type;
  if (type == and || type == or) {
    int *results2 = calloc(len, sizeof(int));
    computeResult(searchTree->left, results, len);
    computeResult(searchTree->right, results2, len);
    merge(type, results, results2, len);
    free(results2);
  } else if (type == not) {
    computeResult(searchTree->right, results, len);
    // inverse
    for (int i = 0; i < len; i++) {
      if (results[i] == 0) {
        results[i] = 1;
      } else {
        results[i] = 0;
      }
    }
  } else if (type == statement) {
    int field = searchTree->field;
    check(results, field, searchTree->value, searchTree->matchType);
  }
}

void toFlatArray(struct fileMeta *files) {
  for (int i = 0; i < agenda_files_amount; i++) {
    headingAmount += files[i].headingCount;
  }

  headings = calloc(headingAmount, sizeof(struct headingMeta));
  int index = 0;

  for (int i = 0; i < agenda_files_amount; i++) {
    for (int j = 0; j < files[i].headingCount; j++) {
      headings[index] = files[i].headings[j];
      headings[index].path = files[i].path;
      // printf("path: %s\n", headings[index].path);
      //  printf("path: %s\n", files[i].path);
      index++;
    }
  }
}

void search(char *search, struct fileMeta *files) {
  if (search == NULL) {
    // printf("no search specified\nsee -h for help regarding the usage\n");
    return;
  }
  struct node *searchTree = calloc(1, sizeof(struct node));
  nodeAmount = countNodes(search);
  // printf("NODES AMOUNT:%d\n\n", nodeAmount);

  nodes = calloc(nodeAmount, sizeof(struct node));
  int nodeIndex = 0;

  string2searchTree(search, searchTree, &nodeIndex);
  // recPrintTree(searchTree->left, 0, 1);

  toFlatArray(files);
  int len = headingAmount;
  int *results = calloc(len, sizeof(int));
  computeResult(searchTree->left, results, len);

  printf("search: %s in %ld files\n", search, agenda_files_amount);
  printResult(results, len);

  free(results);
  free(searchTree);
  free(headings);
  freeTree();
}
