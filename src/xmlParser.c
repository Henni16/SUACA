#include "xmlParser.h"
#include <limits.h>

inst_info_t *parse_instruction_file(char *file_name) {
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        printf("Xml file not found\n");
        return NULL;
    }
    char buff[MY_BUFF_SIZE];
    attribute_value_t a;
    inst_info_t *info_array = malloc(XED_IFORM_LAST * sizeof(inst_info_t));
    while (fscanf(file, "%s", buff) != EOF) {
        if (*buff == '<') {
            if (!strcmp(buff + 1, "instruction")) {
                parse_single_instruction(info_array, file);
                //fscanf(file, "%s", buff);
                //split_attribute(buff, &a);
                //printf("\nattribute: %s\n", a.attribute);
                //printf("value: %s\n\n", a.value);
                //skip_cur_element(file);
            } //else
            //printf("%s\n", buff);
        }
    }
    free(info_array);
    fclose(file);
    return NULL;
}

void parse_single_instruction(inst_info_t *info, FILE *file) {
    attribute_value_t att;
    char buff[MY_BUFF_SIZE];
    //get iform enum
    while (fscanf(file, "%s", buff) != EOF) {
        split_attribute(buff, &att);
        if (!strcmp(att.attribute, "iform")) {
            //printf("iform: %s\n", att.value);
            search_end_of_item(file);
            break;
        }
    }
    parse_operands(file, info);
}


void parse_operands(FILE *file, inst_info_t *info) {
    char buff[MY_BUFF_SIZE];
    attribute_value_t att;
    int id = -1;
    latency_reg_t *latreg = NULL;
    while (fscanf(file, "%s", buff) != EOF) {
        if (strcmp(buff + 1, "operand")) break;
        int tofind = 2;
        bool regfound = false;
        while (fscanf(file, "%s", buff)) {
            split_attribute(buff, &att);
            if (!strcmp(att.attribute, "type")) {
                if (!strcmp(att.value, "reg")) {
                    if (latreg != NULL) {
                        latreg->next = newLatReg();
                        latreg = latreg->next;
                    } else
                        latreg = newLatReg();
                    regfound = true;
                }
                tofind--;
            }
            if (!strcmp(att.attribute, "idx")) {
                id = atoi(att.value);
                tofind--;
            }
            if (!tofind) {
                if (regfound) latreg->id = id;
                if (!search_end_of_item(file) && regfound) {
                    if (att.rest != NULL)
                        parse_registers(att.rest, latreg);
                    else
                        extract_registers(file, latreg);
                }
                break;
            }
        }
    }
}


void extract_registers(FILE *file, latency_reg_t *latreg) {
    char buff[MY_BUFF_SIZE];
    fscanf(file, "%s", buff);
    parse_registers(buff, latreg);
}

void parse_registers(char *line, latency_reg_t *latreg) {
    char *regString;
    xed_reg_enum_t reg = str2xed_reg_enum_t(strtok(line, ",<"));
    while (reg != XED_REG_INVALID) {
        //printf("reg: %s\n", xed_reg_enum_t2str(reg));
        regString = strtok(NULL, ",<");
        reg = str2xed_reg_enum_t(regString);
    }
}

bool search_end_of_item(FILE *file) {
    char cur = ' ';
    while (cur != EOF && cur != '>') {
        if (cur == '/') {
            cur = fgetc(file);
            if (cur == '>')
                return true;
        }
        cur = fgetc(file);
    }
    return false;
}


void split_attribute(char *attribute, attribute_value_t *a) {
    a->attribute = strtok(attribute, "=>/");
    a->value = strtok(NULL, "=>/");
    a->rest = strtok(NULL, "");
    a->value = strtok(a->value, "\"");
}

void skip_cur_element(FILE *f) {
    int cur = fgetc(f);
    int level = 1;
    while (cur != EOF) {
        if (cur == '<') {
            cur = fgetc(f);
            if (cur == '/') {
                level--;
                char rteBuff[MY_BUFF_SIZE];
                fscanf(f, "%s", rteBuff);
                if (!level) break;
            } else {
                level++;
            }
        } else if (cur == '/') {
            cur = fgetc(f);
            if (cur == '>') {
                level--;
                if (!level) break;
            }
        }
        cur = fgetc(f);
    }
}


station_t *parse_station_file(char *file_name) {
    station_t *s = malloc(sizeof(station_t));
    s->size = INT_MAX;
    s->cap = INT_MAX;
    s->load_per_cycle = 4;
    s->num_ports = 6;
    s->ports = malloc(s->num_ports * sizeof(port_t *));
    for (size_t i = 0; i < s->num_ports; i++) {
        //create empty port as recursion anchor
        s->ports[i] = newPort(newSimInst(-1, NULL, 0, 0, 0, 0), NULL);
    }
    s->wait_queue = NULL;
    s->station_queue = NULL;
    s->done_insts = NULL;
    return s;
}

int get_max_latency(latency_reg_t *l) {
    int max = -1;
    while (l != NULL) {
        if (l->latency > max)
            max = l->latency;
        l = l->next;
    }
    return max;
}


int get_latency_for_register(latency_reg_t *l, xed_reg_enum_t reg) {
    int max = -1;
    while (l != NULL) {
        if (l->latency > max)
            max = l->latency;
        for (int i = 0; i < l->numregs; ++i) {
            if (l->reg[i] == reg)
                return l->latency;
        }
        l = l->next;
    }
    return max;
}


latency_reg_t *newLatReg() {
    latency_reg_t *ret = malloc(sizeof(latency_reg_t));
    ret->id = -1;
    ret->numregs = 0;
    ret->next = NULL;
    return ret;
}