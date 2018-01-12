#include "xmlParser.h"

int main (void) {
  parse_xml_file("test.xml");
  printf("done\n");
  return 0;
}

inst_info_t* parse_xml_file(char* file_name) {
  FILE* file = fopen(file_name, "r");
  if (file == NULL) {
    printf("Xml file not found\n");
    return NULL;
  }
  char buff[MY_BUFF_SIZE];
  attribute_value_t a;
  inst_info_t* info_array = malloc(XED_IFORM_LAST * sizeof(xed_iform_enum_t));
  while (fscanf(file, "%s", buff) != EOF) {
    if (!strcmp(buff+1, "name")) {
      fscanf(file, "%s", buff);
      split_attribute(buff, &a);
      printf("\nattribute: %s\n", a.attribute);
      printf("value: %s\n\n", a.value);
      //skip_cur_element(file);
    }
    else
      printf("%s\n", buff);
  }
  free(info_array);
  fclose(file);
}


void split_attribute(char* attribute, attribute_value_t* a) {
  a->attribute = strtok(attribute, "=>");
  a->value = strtok(NULL, "=>");
}

void skip_cur_element(FILE* f) {
  char cur;
  int level = 1;
  cur = fgetc(f);
  while (cur != EOF) {
    if (cur == '<') {
      cur = fgetc(f);
      if (cur == '/') {
        level--;
        char rteBuff[255];
        fscanf(f, "%s", rteBuff);
        if (!level) break;
      }
      else {
        level++;
      }
    }
    else if (cur == '/') {
      cur = fgetc(f);
      if (cur == '>') {
        level--;
        if (!level) break;
      }
    }
    cur = fgetc(f);
  }
}
