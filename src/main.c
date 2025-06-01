#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *org_agenda_files;

void readConfig(){
  //org_agenda_files = calloc(, sizeof(int));
  char wow[4] = "wow";
  org_agenda_files = calloc(sizeof(wow), sizeof(wow));
  memcpy(org_agenda_files,wow,sizeof(wow));
}

int main(int argc, char *argv[])
{
  readConfig();
  printf("%s\n",org_agenda_files);
  return 0;
}
