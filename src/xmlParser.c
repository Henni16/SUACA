#include "xmlParser.h"
#include <limits.h>

inst_info_t **parse_instruction_file(char *file_name, char *architecture_name) {
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        printf("Xml file not found\n");
        return NULL;
    }
    char buff[MY_BUFF_SIZE];
    attribute_value_t a;
    inst_info_t **info_array = calloc(XED_IFORM_LAST, sizeof(inst_info_t));
    while (fscanf(file, "%s", buff) != EOF) {
        if (*buff == '<') {
            if (!strcmp(buff + 1, "instruction")) {
                parse_single_instruction(info_array, file, architecture_name);
            }
        }
    }
    fclose(file);
    return info_array;
}

void parse_single_instruction(inst_info_t **info, FILE *file, char *architecture_name) {
    xed_iform_enum_t iform = xed_iform_enum_t_last();
    attribute_value_t att;
    inst_info_t *my_info = malloc(sizeof(inst_info_t));
    my_info->latencies = NULL;
    char buff[MY_BUFF_SIZE];
    //get iform enum
    while (fscanf(file, "%s", buff) != EOF) {
        split_attribute(buff, &att);
        if (!strcmp(att.attribute, "iform")) {
            iform = str2xed_iform_enum_t(att.value);
            if (iform == XED_IFORM_INVALID) {
                //printf("Invalid iform: %s\n", att.value);
                skip_cur_element(file);
                return;
            }
            info[iform] = my_info;
            search_end_of_item(file);
            break;
        }
    }
    parse_operands(file, my_info);
    fscanf(file, "%s", buff);
    split_attribute(buff, &att);
    if (!strcmp(att.value, architecture_name)) {
        if (att.rest) {
#if XML_DEBUG > 0
            printf("Instruction: %s empty for architecture: %s\n", xed_iform_enum_t2str(iform), ARCHITECTURE_NAME);
#endif
            free_info(info[iform]);
            info[iform] = NULL;
            skip_cur_element(file);
            return;
        }
        parse_architecture(file, my_info);
        skip_cur_element(file);
    } else {
        if (!att.rest)
            skip_cur_element(file);
        while (fscanf(file, "%s", buff)) {
            if (strcmp(buff + 1, "architecture")) {
#if XML_DEBUG > 0
                printf("Instruction: %s not found for architecture: %s\n", xed_iform_enum_t2str(iform), ARCHITECTURE_NAME);
#endif
                free_info(info[iform]);
                info[iform] = NULL;
                return;
            }
            fscanf(file, "%s", buff);
            split_attribute(buff, &att);
            if (!strcmp(att.value, architecture_name)) {
                parse_architecture(file, my_info);
                skip_cur_element(file);
                return;
            } else {
                skip_cur_element(file);
            }
        }
    }
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
        bool flagfound = false;
        while (fscanf(file, "%s", buff)) {
            split_attribute(buff, &att);
            if (!strcmp(att.attribute, "type")) {
                if (!strcmp(att.value, "reg")) {
                    if (latreg != NULL) {
                        latreg->next = newLatReg();
                        latreg = latreg->next;
                    } else {
                        latreg = newLatReg();
                        info->latencies = latreg;
                    }
                    regfound = true;
                } else if (!strcmp(att.value, "flag")) {
                    //TODO proper use of flags
                    //see xed_decoded_inst_get_rflags_info()
                    if (latreg != NULL) {
                        latreg->next = newLatReg();
                        latreg = latreg->next;
                    } else {
                        latreg = newLatReg();
                        info->latencies = latreg;
                    }
                    latreg->reg[0] = XED_REG_RFLAGS;
                    flagfound = true;
                }
                tofind--;
            }
            if (!strcmp(att.attribute, "idx")) {
                id = atoi(att.value);
                tofind--;
            }
            if (!tofind) {
                if (regfound || flagfound) latreg->id = id;
                if (flagfound && att.rest == NULL) {
                    skip_cur_element(file);
                    break;
                }
                if (att.rest != NULL && regfound) {
                    parse_registers(att.rest, latreg);
                } else if (att.rest != NULL) {
                    break;
                } else if (!search_end_of_item(file) && regfound) {
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
        add_reg_to_lat_reg(reg, latreg);
        regString = strtok(NULL, ",<");
        reg = str2xed_reg_enum_t(regString);
    }
}


void parse_architecture(FILE *file, inst_info_t *info) {
    //printf("architecture found\n");
    skip_cur_element(file);
}


bool search_end_of_item(FILE *file) {
    int cur = ' ';
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
    ret->cap = 1;
    ret->next = NULL;
    ret->reg = malloc(ret->cap * sizeof(xed_reg_enum_t));
    return ret;
}


void add_reg_to_lat_reg(xed_reg_enum_t reg, latency_reg_t *latreg) {
    if (latreg->numregs >= latreg->cap) {
        latreg->cap *= 2;
        void *tmp = realloc(latreg->reg, latreg->cap * sizeof(xed_reg_enum_t));
        if (tmp == NULL)
            printf("fail\n");
        latreg->reg = tmp;
    }
    latreg->reg[latreg->numregs++] = reg;
}


void free_info(inst_info_t *info) {
    latency_reg_t *r = info->latencies;
    latency_reg_t *r2;
    while (r != NULL) {
        r2 = r;
        r = r->next;
        free(r2);
    }
    free(info);
}

void free_info_array(inst_info_t **array) {
    for (int i = 0; i < XED_IFORM_LAST; ++i) {
        if (!array[i]) continue;
        free_info(array[i]);
    }
    free(array);
}