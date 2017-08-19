#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "header.h"

/**
 * Skip white spaces or "\t" chars to get to the first word.
 *
 * @param char[]     line - The scanned line.
 * @param int*       i - Address to save the position.
 */
void skip_white_space(const char line[LINE_MAX], int *i) {
    while ( line[*i] == ' ' || line[*i]=='\t' ) {
        (*i)++;
    }
}

int is_valid_register(char *reg){
    return strlen(reg) == 2 && reg [0] == 'r' && reg[1] <= '7' && reg[1] >= '0';
}
/**
 * Copies the first word of the operation line "line" beginning from "position" to "single_word"
 *
 * @param char[]    line - The line we are handling.
 * @param char[]    single_word - Will hold the words at the end.
 * @param int*      position - The index which we start counting from.
 *
 * @return int - The number of characters the word contains.
 */
int get_new_word(char line[LINE_MAX], char single_word[LINE_MAX], int *position) {
    char *beginning = &line[*position]; /* start points to the start of the word */
    int counter = 0; /* counts the number of characters the word contains */

    while ( line[*position] != ':'
            && line[*position] != ','
            && line[*position] != ';'
            && line[*position] != '\t'
            && line[*position] != ' '
            && line[*position] != '\n'
            && line[*position] != '\0') {
        (*position)++;
        counter++;
    }

    if ( line[*position]==':' ){
        (*position)++;
        counter++; /* we want to know if the word is a label, so we put the ':' also */

    } else if ( line[*position] == ',' ) { /* we want to advance the position by one because the comma is not a part of the word*/
        (*position)++;
    } else if ( line[*position] ) { /* maybe there is a comma somewhere in comma later on */
        skip_white_space(line, position);
        if ( line[*position] == ',' ) {
            (*position)++;
        }
    }

    strncpy(single_word, beginning, (size_t) counter);
    single_word[counter]= '\0';

    return counter;
}

/**
 * Copies the string that comes after a ".entry" operation, beginning from "position", to the variable "string".
 *
 * @param char[]    line - The line we are handling.
 * @param char[]    string - Will hold the words at the end.
 * @param int*      position - The index which we start counting from.
 *
 * @return int - The number of characters the word contains.
 */
int get_entry_string(char line[LINE_MAX], char string[LINE_MAX], int *position) {
    char *beginning=&line[*position]; /* "beginning" points to the beginning of the word */
    int counter = 0; /* counts the number of characters the string contains */

    if (line[(*position)++]!='"') /*invalid string - line should begin with a quotation mark */
        return -1;

    while (line[*position]!='"' && line[*position]!='\n' && line[*position] != '\0')
    {
        (*position)++;
        counter++;
    }
    if (line[*position]=='\n'){ /*if there's no " at the end- invalid string*/
        return -1;
    }
    (*position)++; /*change position to the next char (after the ") */

    strncpy(string, beginning + 1, (size_t) counter); /* copy the word without the first " */
    string[counter++]='\0';

    return counter; /* valid string */
}

/**
 * Return the register number
 *
 * @param char[]    reg - The register string, i.e "r5".
 *
 * @return int - The register number.
 */
int find_reg_num(char reg[LINE_MAX]) {
    return reg[1]-'0';
}

/**
 * Find a label address in the signs table and also report if it's external or not.
 *
 * @param char*             label - The needle.
 * @param table_of_signs*   table_signs - The haystack.
 * @param int               table_signs_size - The size of the haystack.
 * @param int*              is_external - Will indicate at the end weather the sign is external or not.
 *
 * @return int The label address if everything went ok, -1 otherwise.
 */
int find_label_address(char *label, table_of_signs* table_signs, int table_signs_size, int *is_external){
    int i;

    for ( i = 0; i < table_signs_size; i++ ) {
        if ( strcmp(table_signs[i].label_name, label) == 0 ) {
            (*is_external) = table_signs[i].external;
            return table_signs[i].address;
        }
    }

    return -1;
}

/**
 * Calculate size of a given matrix based on the string arg, assuming the string is already in a valid form.
 * i.e if arg = [3][6] then the size is 3 * 6 = 18.
 *
 * @param char* arg - Points the string that have the matrix form, i.e [3][7].
 *
 * @return int - The result.
 */
int calculate_matrix_size(char *arg){
    int i, j, k = 0, rows = 0, columns = 0;
    size_t arg_len = strlen(arg);
    char temp[LINE_MAX];
    int in_first_parenthesis = 1; /* are we in the first or in the second parenthesis? */

    for ( i = 0; i < arg_len; i++ ) {
        if ( arg[i] == '[' ) {
            for ( j = i+1; arg[j] != ']'; j++ ) {
                temp[k++] = arg[j];
            }
            temp[k] = '\0';
            if ( in_first_parenthesis ) {
                rows = atoi(temp);
                in_first_parenthesis = k = 0;
            } else {
                columns = atoi(temp);
            }
        }
    }

    return rows * columns;
}


/**
 * Convert integer to word_type.
 * If the number is bigger than the size of a word (10 bits) the value that will be returned is 0.
 * This function is used mainly to append data to the data segment.
 *
 * @param int       int_num - The number to transform.
 * @param int*      error - Function set this to 1 if error occurs.
 *
 * @return word_t - The transformed word.
 */
word_t trans_to_word(int int_num, int line_count, int *error) {
    word_t opcode_num; /* this will be the number after being opcoded */
    opcode_num.oper = opcode_num.amethod_src_operand = opcode_num.amethod_dest_operand = opcode_num.memory = 0; /* init opcode num to be 0 */

    if ( int_num < pow(2, WORD_MAX) && int_num > -pow(2, WORD_MAX) ) { /* if num is in the word size boundaries (10 bits) */
        int mask = 3; /* mask for the ints that should have only one bits */
        int mask2 = 15; /* mask for the "oper" member, which should have four bits */

        /* Start shifting from right to left, set each value to the relevant member */
        opcode_num.memory = (unsigned) int_num & mask;
        int_num >>= 2;

        opcode_num.amethod_dest_operand = (unsigned) int_num & mask;
        int_num >>= 2;

        opcode_num.amethod_src_operand = (unsigned) int_num & mask;
        int_num >>= 2;

        opcode_num.oper = (unsigned) int_num & mask2;

    } else {
        fprintf(stderr, "line %d:\tNumber's size is bigger than the word size (10 bits).\n", line_count);
        (*error) = 1;
    }

    return opcode_num;
}

/**
 * This function is used to transform argument to word_type.
 *
 * @param int   num - The number which represent the argument.
 * @param int   memory_type - The memory type, absolute, external or relocatable.
 *
 * @return word_t - The transformed word.
 */
word_t trans_arg_to_word(int num, int memory_type){
    word_t opcode_num;
    opcode_num.oper = opcode_num.amethod_src_operand = opcode_num.amethod_dest_operand = opcode_num.memory = 0; /* init opcode num to be 0 */

    if ( num < pow(2, WORD_MAX) && num > -pow(2, WORD_MAX) ) { /* if num is in the word size boundaries (10 bits) */
        int mask = 3; /* mask for the ints that should have only one bits */
        int mask2 = 15; /* mask for the "oper" member, which should have four bits */

        opcode_num.memory = (unsigned) memory_type;

        opcode_num.amethod_dest_operand = (unsigned) num & mask;
        num >>= 2;

        opcode_num.amethod_src_operand = (unsigned) num & mask;
        num >>= 2;

        opcode_num.oper = (unsigned) num & mask2;

    } else {
        fprintf(stderr, "Number's size is bigger than the word size (10 bits). The opcode value returned is 0.\n");
    }

    return opcode_num;
}

/**
 * This function encodes a register to word_type.
 *
 * @param int   first_register_num - First register number, this will place in bits 6-9.
 * @param int   first_register_num - First register number, this will place in bits 2-5.
 * @param int   memory_type - The memory type for the word. A, E or R.
 *
 * @return word_t - The encoded word.
 */
word_t trans_regs_to_word(int first_register_num, int second_register_num, int memory_type){
    word_t opcode_num;
    opcode_num.oper = opcode_num.amethod_src_operand = opcode_num.amethod_dest_operand = opcode_num.memory = 0;

    if ( (first_register_num < pow(2, WORD_MAX) && first_register_num > -pow(2, WORD_MAX)) ||  (second_register_num < pow(2, WORD_MAX) && second_register_num > -pow(2, WORD_MAX)) ) {
        int mask = 3; /* mask the last two bits */

        opcode_num.memory = (unsigned) memory_type;

        opcode_num.amethod_dest_operand = (unsigned) second_register_num & mask;
        second_register_num >>= 2;

        opcode_num.amethod_src_operand = (unsigned) second_register_num & mask;

        opcode_num.oper = (unsigned) first_register_num;


    } else {
        fprintf(stderr, "Number's size is bigger than the word size (10 bits). The opcode value returned is 0.\n");
    }

    return opcode_num;

}

/**
 * Get label in the form "label_name[n][m]" and extract only the label.
 *
 * @pararam char* src_label - The label that we want to extract from it.
 * @param char** dest_label - Pointer to the label the will have the extracted value.
 */
void extract_mat_label(char *src_label, char **dest_label){
    int i = 0;

    while ( src_label[i] != '[' && i < strlen(src_label) ) {
        (*dest_label)[i] = src_label[i];
        i++;
    }
    (*dest_label)[i] = '\0';
}

/**
 * Get label in the form of "label[r2][r3]" and extract only the the registers.
 *
 * @param char*     label - The label.
 * @param char **   reg1 - Pointer to the first extracted register.
 * @param char**    reg2 - Pointer to the first extracted register.
 */
void extract_mat_registers(char *label, char **reg1, char **reg2){
    int i, j, k = 0;
    int in_first_parenthesis = 1; /* are we in the first or in the second parenthesis? */
    size_t arg_len = strlen(label);
    char temp[LINE_MAX];

    for ( i = 0; i < arg_len; i++ ) {
        if ( label[i] == '[' ) {
            for ( j = i+1; label[j] != ']'; j++ ) {
                temp[k++] = label[j];
            }
            temp[k] = '\0';

            /*
             * Check if the register length is valid.
             * Although this function is called after the first scan and the register should be valid,
             * the safer the better.
             */
            if ( strlen(temp) > 3 ) {
                return;
            }

            if ( in_first_parenthesis ) {
                strcpy((*reg1), temp);
                in_first_parenthesis = k = 0;
            } else {
                strcpy((*reg2), temp);
                break;
            }
        }
    }
}

/**
 * Check if a given value is a valid register.
 *
 * @param char* str - The register to check.
 *
 * @return 1 if true, 0 if false.
 */
int is_register(char *str){
    if ( strlen(str) == 2
         &&str[0] == 'r'
         && str[1] <= '7'
         && str[1]>='0' ) {
        return 1;
    }

    return 0;
}


/**
 * Encode argument and place it in the code segment.
 * Since this function called from the second scan, we assume everything is valid.
 *
 * @param char*     arg - The argument to encode
 * @param int       amethod - The addressing method.
 * @param word_t**  code_seg - The code segment to place the argument in.
 * @param int*      seg_size - The size of the code segment.
 */
void encode_argument(char *arg, int amethod, char *additional_arg, int arg_count, word_t **code_seg, int *seg_size, table_of_signs *table_signs, int table_signs_size, data_table **ext_table, int *ext_table_size){
    word_t word_to_append;
    word_t sec_word_to_append; /* if need to encode another word, for matrices for example */
    int address, reg1_num, reg2_num, has_second_word = 0;
    int is_external = 0;
    char *mat_label, *reg1, *reg2;

    /* bail early if arg contains nothing */
    if ( strlen(arg) == 0 ) {
        return;
    }

    switch ( amethod ) {
        case IMMEDIATE:
            /* remove the char "#" */
            memmove(&arg[0], &arg[1], strlen(arg));
            word_to_append = trans_arg_to_word(atoi(arg), A);
            break;
        case DIRECT:
            /* get the address of the variable from the table of signs */
            address = find_label_address(arg, table_signs, table_signs_size, &is_external);

            if ( is_external ) { /* if its external, transform to word when memory is absolute */
                word_to_append = trans_arg_to_word(address, E);

                /* add the external label to the ext_table with the new address (which is the original IC) */
                if ( ! update_ext_table(ext_table, ext_table_size, arg, ((*seg_size)+INITIAL_IC)) ) {
                    return;
                }

            } else { /* else it must be relocatable */
                word_to_append = trans_arg_to_word(address, R);
            }
            break;
        case MATRIX_ACCESS:
            mat_label = malloc((strlen(arg) * sizeof(char)) + 1);
            reg1 = malloc(sizeof(char) * 3);
            reg2 = malloc(sizeof(char) * 3);
            if ( !mat_label || !reg1 || !reg2 ) {
                fprintf(stderr, "Error allocating memory\n");
                return;
            }

            /* handle the matrix label */
            extract_mat_label(arg, &mat_label);
            address = find_label_address(mat_label, table_signs, table_signs_size, &is_external);
            if ( is_external ) { /* if its external, transform to word when memory is absolute */
                word_to_append = trans_arg_to_word(address, E);
            } else { /* else it must be relocatable */
                word_to_append = trans_arg_to_word(address, R);
            }

            /* handle the registers */
            extract_mat_registers(arg, &reg1, &reg2);
            reg1_num = find_reg_num(reg1);
            reg2_num = find_reg_num(reg2);
            sec_word_to_append = trans_regs_to_word(reg1_num, reg2_num, A);
            has_second_word = 1;

            free(mat_label);
            free(reg1);
            free(reg2);
            break;

        default: /* direct register */

            /*
             * Direct register is a bit tricky, since we need to know exactly the arguments count and
             * the addressing methods before we can encode it. So we'll want to check if this is the first arg
             * that being treated or the second, and proceed accordingly.
             *
             */
            if ( arg_count == FIRST_ARG ) {
                reg2_num = find_reg_num(arg);
                if ( is_register(additional_arg) ) { /* if arg2 is also a register */
                    reg1_num = find_reg_num(additional_arg); /* going to be encoded to bits 2-5 */
                    word_to_append = trans_regs_to_word(reg2_num, reg1_num, A);
                } else { /* only arg1 is register, encode it to bits 9-6 */
                    word_to_append = trans_regs_to_word(reg2_num, 0, A);
                }
            } else { /* SECOND_ARG */
                if ( is_register(additional_arg) ) { /* if arg1 is also a register, we already took care of this. leave the function */
                    return;
                }
                /* if only arg2 is a register, this is a destination operand - encode it to bits 2-5 */
                reg1_num = find_reg_num(arg);
                word_to_append = trans_regs_to_word(0, reg1_num, A);
            }

            break;

    }

    code_insert(code_seg, seg_size, word_to_append);
    if ( has_second_word ) {
        code_insert(code_seg, seg_size, sec_word_to_append);
    }

}

/**
 * Convert number from base 10 to base 4.
 *
 * @param int   number - The number to convert.
 * @return int - The converted number.
 */
int convert_to_base_four(int number){
    if( number == 0 ) {
        return number;
    }

    return (number % 4) + 10 * convert_to_base_four(number / 4);
}

/**
 * Reverse the chars of a given string.
 *
 * @param char*     str - The string to reverse.
 * @return char* - The reversed string.
 */
char *reverse_string(char *str){
    size_t i;
    int j;
    char *temp = malloc(strlen(str) * sizeof(char));

    for ( i = strlen((str))-1, j = 0; i != -1; i--, j++ ) {
        temp[j] = str[i];
    }
    temp[j] = '\0';

    return temp;
}

/**
 * Convert a number to base 4 "mozar" as described in the maman book.
 * a - 0, b - 1, c - 2, d - 3.
 *
 * @param word_t    word - The word to convert.
 * @param char **   p - Pointer that will point to the converted string at the end.
 */
void convert_num_to_base_four_mozar(int num, char **p){
    int i = 0;
    int base_4_num = convert_to_base_four(num);
    char *temp;
    int digit;

    /*
     * Get each digit of the base-4 number.
     * This will give us the digits from the least significant digit, so we'll reverse the string at the end.
     */
    while ( base_4_num ) {
        (*p) = realloc((*p), (sizeof(char) + i));
        digit = base_4_num % 10;

        switch ( digit ) {
            case 0:
                (*p)[i] = 'a';
                break;
            case 1:
                (*p)[i] = 'b';
                break;
            case 2:
                (*p)[i] = 'c';
                break;
            default: /* 3 */
                (*p)[i] = 'd';
        }

        base_4_num/= 10;
        i++;
    }

    if ( i == 0 ) { /* if num == 0 and the while loop didn't do even one iteration */
        (*p)[i] = 'a';
        i++;
    }

    (*p) = realloc((*p), (sizeof(char) + 1));
    (*p)[i] = '\0';
    temp = (*p);
    (*p) = reverse_string(*p);
    free(temp); /* free the unreversed string */
}

/**
 * Convert a word (word type) to base 4 "mozar".
 *
 * @param word_t    word - The word to convert.
 * @param char**    p - Pointer that will point to the converted word at the end.
 */
void convert_word_to_base_four_mozar(word_t word, char **p){
    int mask = 3; /* mask the last two bits */
    int num_to_convert; /* will hold the number that we gonna convert each time */
    char *p1, *p2, *p3, *p4, *p5; /* these pointers point on each individual base 4 mozar char */

    p1 = malloc(sizeof(char));
    p2 = malloc(sizeof(char));
    p3 = malloc(sizeof(char));
    p4 = malloc(sizeof(char));
    p5 = malloc(sizeof(char));

    num_to_convert = word.oper & mask;
    convert_num_to_base_four_mozar(num_to_convert, &p2);
    word.oper >>= 2;

    num_to_convert = word.oper & mask;
    convert_num_to_base_four_mozar(num_to_convert, &p1);

    convert_num_to_base_four_mozar(word.amethod_src_operand, &p3);
    convert_num_to_base_four_mozar(word.amethod_dest_operand, &p4);
    convert_num_to_base_four_mozar(word.memory, &p5);

    strcat((*p), p1);
    strcat((*p), p2);
    strcat((*p), p3);
    strcat((*p), p4);
    strcat((*p), p5);

    free(p1);
    free(p2);
    free(p3);
    free(p4);
    free(p5);
}

/**
 * Free all names strings in the signs table.
 *
 * @param table_signs
 * @param table_size
 */
void free_signs_names(table_of_signs *table_signs, int table_size){
    int i;

    for ( i = 0; i < table_size; i++ ) {
        free(table_signs[i].label_name);
    }
}