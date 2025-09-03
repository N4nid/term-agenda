#include "scan.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node {
  char *value;
  // see query.c for possible values
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
// matchTypes constants
const int exact = 1;    // ==
const int contains = 2; // ~=

int nodeAmount = 0;
struct node *nodes;

void recPrintTree(struct node *node, int isLeft, int depth) {
  if (node == NULL) {
    return;
  }
  char side = 'r';
  if (isLeft)
    side = 'l';
  if (node->type == statement) {
    printf("%d|%c -YOO: %d %s\n", depth, side, node->type, node->value);
    free(node->value);
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
  printf(" - between: %s\n", out);

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
  char fields[5][6] = {"TAG", "TODO", "SCHED", "DEAD", "PROP"};
  int lens[5] = {3, 4, 5, 4, 4};

  for (int i = 0; i < 5; i++) {
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
  while (current != '=' && current != '~') {
    // printf(" curr; %c\n", current);
    index++;
    current = input[index];
    if (current == '\0')
      return 0;
  }
  if (current == '~')
    node->matchType = contains;
  else
    node->matchType = exact;

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

void breakDueToInvalidInput() {
  printf("\n! INVALID INPUT !\n");
  exit(1);
}

int countNodes(char *input) {
  printf("%s\n", input);
  int amount = 0;
  int pos = 0;
  int isStatement = 2;
  int nextLen = 0;
  char *next = "";

  while (next != NULL) {
    setNext(input, &next, &pos, &isStatement, &nextLen);
    // printf("pos: %d next: [%s] len: %d\n", pos, next, nextLen);
    if (isStatement == 1) {
      isStatement = 0;
      if (nextLen < 7)
        breakDueToInvalidInput();
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
  printf("\n%s\n", search);
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
    printf("pos: %d next: [%s] len: %d\n", pos, next, nextLen);

    if (isStatement == 1 && next != NULL) {
      isStatement = 0;
      if (nextLen < 7)
        breakDueToInvalidInput();

      if (setStatmentNode(next, nextLen, &nodes[*nodeIndex])) {
        // printf(" val: %s\n", testNode.value);
        lastNode = &nodes[*nodeIndex];
        nodes[*nodeIndex].left = NULL;
        nodes[*nodeIndex].right = NULL;

        if (lastNodeType == not) {
          lastOp->left = lastNode;
        } else if (lastNodeType == and || lastNodeType == or) {
          lastOp->right = lastNode;
        }

        lastNodeType = statement;
        *nodeIndex += 1;
      } else {
        breakDueToInvalidInput();
      }
    } else if (next != NULL) { // should be a operator or bracket

      if (next[0] != '(' && next[0] != ')') {
        if (top == NULL) {
          top = &nodes[*nodeIndex];
          printf(" Im Top now :3\n");
        }
      } else if (next[0] == '(') {
        struct node tmp;
        int len = 0;
        char *between = getBetweenBrackets(search + pos - 1, &len);
        string2searchTree(between, &tmp, nodeIndex);
        lastNode = tmp.left;

        if (lastNodeType == not) {
          lastOp->left = lastNode;
        } else if (lastNodeType == and || lastNodeType == or) {
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
            printf(" Im the Top now :3\n");
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
            printf(" Im the Top now :3\n");
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
  printf("\n");
}

void search(char *search) {
  // struct node searchTree;
  struct node *searchTree = calloc(1, sizeof(struct node));
  if (search == NULL) {
    return;
  }
  nodeAmount = countNodes(search);
  printf("NODES AMOUNT:%d\n\n", nodeAmount);

  nodes = calloc(nodeAmount, sizeof(struct node));
  int nodeIndex = 0;

  string2searchTree(search, searchTree, &nodeIndex);
  recPrintTree(searchTree, 0, 1);

  //  for (int i = 0; i < nodeAmount; i++) {
  //    printf("YO: %d\n", nodes[i].type);
  //  }

  free(nodes);
  free(searchTree);
}
