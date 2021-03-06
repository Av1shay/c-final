#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "header.h"

opers valid_operations[]={	/* array of the valid operations */
	{"mov", 0},
	{"cmp", 1},
	{"add", 2},
	{"sub", 3},
	{"not", 4},
	{"clr", 5},
	{"lea", 6},
	{"inc", 7},
	{"dec", 8},
	{"jmp", 9},
	{"bne", 10},
	{"red", 11},
	{"prn", 12},
	{"jsr", 13},
	{"rts", 14},
	{"stop", 15}
};


/**
 * Checks a label syntax validity.
 *
 * @param char* label - The label to check for it's validity.
 *
 * @return int 1 if the syntax is valid, 0 otherwise.
 */
static int check_label(char label[LINE_MAX]) {
    unsigned int i;

	if ( strlen(label) > LABEL_MAX || !isalpha(label[0]) ) {
        return 0;
    }

	for ( i = 1; i < strlen(label); i++ ) { /*for every character of the label*/
        if (! isalnum(label[i]) ) /* if there's a char that is not digit or english letter */
            return 0;
    }

	for ( i = 0; i < 16; i++ ) {/* check if the label is an operation name */
        if (strcmp(label, valid_operations[i].oper_name) == 0)
            return 0;
    }

	if ( strlen(label) == 2 && label[0] == 'r' && label[1] <= '7' && label[1] >= '0' ) { /*  check if the label is a register name */
        return 0;
    }

	return 1;
}


/**
 * Checks an operation validity.
 *
 * @param op The operation.
 * @return the type of the instruction or operation code if the operation is valid,
 * else return -1.
 */
static int check_operation(char op[LINE_MAX]) {
	int is_found = -1;
	int i;

	if ( strcmp(op, ".data") == 0 ) /* if the operation is: ".data" */
		return DATA;
	if ( strcmp(op, ".string") == 0 ) /* if the operation is: ".string" */
		return STRING;
    if ( strcmp(op, ".mat") == 0 ) /* if the operation is: ".string" */
        return MAT;
	if ( strcmp(op, ".entry") == 0 ) /* if the operation is: ".entry" */
		return ENTRY;
	if ( strcmp(op, ".extern") == 0 ) /* if the operation is: ".extern" */
		return EXTERN;

	for ( i = 0; i < NUM_OF_OPERATIONS; i++ ) { /* check if word is a command operation */
        if ( strcmp(op, valid_operations[i].oper_name ) == 0) { /* the operation was found */
            is_found = valid_operations[i].oper_num;
            break;
        }
    }

	return is_found;
}

/**
 * Check for validity of matrix form that been accessed.
 *
 * @param char* arg - The argument (or operand).
 * @return 1 if valid, 0 otherwise.
 */
int is_valid_matrix_access_form(char *arg){
    int i, j, k = 0;
    int first_par_flag = 0; /* this will tell us if we are after the first parenthesis checks */
    int open_pars_counter = 0; /* count the number of parenthesis */
    size_t arg_len = strlen(arg);
    char *extracted_label = malloc(sizeof(char) * arg_len), *first_reg, *second_reg;

    extract_mat_label(arg, &extracted_label);
    if ( !strlen(extracted_label) || !check_word(extracted_label, LABEL) ) { /* check for label validity */
        free(extracted_label);
        return 0;
    }

    free(extracted_label);
    first_reg = malloc(sizeof(char) * 3);
    second_reg = malloc(sizeof(char) * 3);

    for ( i = 0; i < arg_len; i++ ) {
        if ( arg[i] == '[' && !first_par_flag ) {
            first_par_flag = 1;
            open_pars_counter++;
            for ( j = i+1; arg[j] != ']'; j++ ) {
                if ( k > 2 ) { /* register must have two characters */
                    free(first_reg);
                    free(second_reg);
                    return 0;
                }
                first_reg[k++] = arg[j];
            }
            first_reg[k] = '\0';
            k = 0;
        } else if ( arg[i] == '[' ) {
            open_pars_counter++;
            for ( j = i+1; arg[j] != ']'; j++ ) {
                if ( k > 2 ) {
                    free(first_reg);
                    free(second_reg);
                    return 0;
                }
                second_reg[k++] = arg[j];
            }
            second_reg[k] = '\0';
        }
    }

    /* check for registers validity and check that we have exactly two pairs of parenthesis */
    if ( ! is_valid_register(first_reg) || ! is_valid_register(second_reg) || open_pars_counter != 2 ) {
        free(first_reg);
        free(second_reg);
        return 0;
    }

    free(first_reg);
    free(second_reg);

    return 1;

}

/**
 * Checks argument syntax validity.
 *
 * @param char[]    arg - The argument.
 * @return int - if the syntax is valid - return the addressing methods, -1 otherwise.
 */
static int check_argument(char arg[LINE_MAX]){
    if ( arg[0] == '#' ) {
        if ( !num_isvalid(arg+1) ) { /* if the rest of the number isn't valid - error*/
            return -1;
        }
        if ( atoi(arg+1) > pow(2, 8) || atoi(arg+1) < -pow(2, 8)){ /* if the immediate number exceed the bunderies of 8 bits */
            return -1;
        }

        return IMMEDIATE; /* no error - immediate addressing */
    }
	if ( check_word(arg, LABEL) ) { /* direct addressing */
        return DIRECT;
    }
    if ( is_valid_matrix_access_form(arg) ) {
        return MATRIX_ACCESS;
    }
	if ( strlen(arg) == 2 && arg [0] == 'r' && arg[1] <= '7' && arg[1] >= '0' ) {
        return DIRECT_REGISTER;
    }

	return -1;
}

/**
 * Checks if the syntax of a word is valid.
 *
 * @param word
 * @param type
 * @return -1 if invalid. a return value that is different than -1 represents a valid word.
 * validation process is done according to the type of the word ("type")
 */
int check_word(char word[LINE_MAX], int type){
	if (type == LABEL)
		return check_label(word); /* returns 1 if the label is valid, 0 otherwise */

	if ( type == OPERATION )
		return check_operation(word); /* returns the type of the instruction or operation code if the operation is valid */

	/* type == ARGUMENT */
	return check_argument(word); /* returns a value that represents the type of address method */

}

/**
 * Check if the argument is valid number.
 *
 * @param arg
 * @return bool
 */
int num_isvalid(char *arg){
	unsigned int i;

	if ( arg[0] == '\0' ) { /* if the word is empty - it's invalid, return 0 */
        return 0;
    }

    if ( arg[0] != '+' && arg[0] != '-' && !isdigit(arg[0]) ) { /* it could start with '+', '-' or digit */
        return 0;
    }

    for ( i = 1; i < strlen(arg); i++ ) { /* the other string must be digits */
        if ( !isdigit(arg[i]) ) { /* if there is a character that is not a digit */
            printf("**%i", arg[i]);
            return 0;
        }
    }

    return 1;
}

/**
 * Check if a given argument is in the form of a matrix that going to set (i.e - "[2][3]").
 *
 * @param char* arg - The argument to test.
 *
 * @return int - 1 if it's valid, 0 otherwise.
 */
int is_valid_matrix_form(char *arg){
    int i, parentheses_counter = 0, in_open_parentheses = 0;
    size_t arg_length = strlen(arg);

    for ( i = 0; i < arg_length; i++ ) {
        if ( arg[i] == '[' ) {
            if ( in_open_parentheses == 1 ) { /* check if there is already one open parentheses */
                return 0;
            }
            in_open_parentheses = 1;
        } else if( arg[i] == ']' ) {
            if ( !in_open_parentheses || (i > 0 && arg[i-1] == '[') ) { /* if we see a closing parentheses and there was no open parenthesis, we have an error */
                return 0;
            }
            in_open_parentheses = 0;
            parentheses_counter++;
        }
    }

    if ( parentheses_counter != 2 ) { /* there must be exactly two pairs */
        return 0;
    }

    return 1;
}

/**
 * Check if a given sign exists in the table of signs.
 *
 * @param table
 * @param row_counter
 * @param sign_name
 * @return 1 if the sign does exist, 0 otherwise
 */
int sign_already_exists(table_of_signs *table, int row_counter, char *sign_name){
    int i;

    for ( i = 0; i < row_counter; i++ ) {
        if ( strcmp(table[i].label_name, sign_name) == 0 ) {
            return 1;
        }
    }

    return 0;
}

/**
 * Check if the addressing methods fits the operation.
 *
 * @param oper
 * @param int     marg1 - Adressing method or the first argument.
 * @param int     marg2 - Addressing method of the second argument.
 * @return bool
 */
int is_address_valid(int oper, int src_operand, int dest_operand){
    switch ( oper ) {
        case 0: /* mov */
            if ( src_operand == NO_ARG || dest_operand == NO_ARG || dest_operand == IMMEDIATE )
                return 0; /* arg1 miss || arg2 miss || src_operand == immediate */
            break;
        case 1: /* cmp */
            if ( src_operand == NO_ARG || dest_operand == NO_ARG )
                return 0; /*  arg1 miss || arg2 miss */
            break;
        case 2: /* add */
            if ( src_operand == NO_ARG || dest_operand == NO_ARG || dest_operand == IMMEDIATE )
                return 0; /* arg1 miss || arg2 miss || src_operand == immediate */
            break;
        case 3: /* sub */
            if ( src_operand == NO_ARG || dest_operand == NO_ARG || dest_operand == IMMEDIATE )
                return 0;	/* sub, arg1 miss || arg2 miss || src_operand == immediate */
            break;
        case 4: /* not */
            if ( src_operand != NO_ARG || dest_operand == NO_ARG || dest_operand == IMMEDIATE )
                return 0;	/* arg1 present || arg2 miss || src_operand!= immediate */
            break;
        case 5: /* clr */
            if ( src_operand != NO_ARG || dest_operand == NO_ARG || dest_operand == IMMEDIATE)
                return 0;	/* clr, arg1 present || arg2 miss || src_operand == immediate */
            break;
        case 6: /* lea */
            if ( src_operand == NO_ARG || src_operand == IMMEDIATE || src_operand == DIRECT_REGISTER || dest_operand == NO_ARG || dest_operand == IMMEDIATE )
                return 0; /* arg1 miss || arg2 miss || arg1 == immediate || arg1 == DIRECT REGISTER addressings || arg2 == immediate */
            break;
        case 7: /* inc */
            if ( src_operand != NO_ARG || dest_operand == NO_ARG || dest_operand == IMMEDIATE )
                return 0; /* arg1 present || arg2 miss || src_operand == immediate */
            break;
        case 8: /* dec */
            if ( src_operand != NO_ARG || dest_operand == NO_ARG || dest_operand == IMMEDIATE )
                return 0; /* arg1 present || arg2 miss || src_operand == immediate */
            break;
        case 9: /* jmp */
            if ( src_operand != NO_ARG || dest_operand == NO_ARG || dest_operand == IMMEDIATE  )
                return 0; /* arg1 present || arg2 miss || src_operand = =immediate */
            break;
        case 10: /* bne */
            if ( src_operand != NO_ARG || dest_operand == NO_ARG || dest_operand == IMMEDIATE )
                return 0; /* arg1 present || arg2 miss || src_operand = =immediate */
            break;
        case 11: /* red */
            if ( src_operand != NO_ARG || dest_operand == NO_ARG || dest_operand == IMMEDIATE )
                return 0; /* arg1 present || arg2 miss || src_operand = =immediate */
            break;
        case 12: /* prn */
            if ( src_operand != NO_ARG || dest_operand == NO_ARG )
                return 0; /* arg1 present || arg2 miss */
            break;
        case 13: /* jsr */
            if ( src_operand != NO_ARG || dest_operand == NO_ARG || dest_operand == IMMEDIATE )
                return 0; /* arg1 present || arg2 miss || src_operand = =immediate */
            break;
        case 14: /* rts */
            if ( src_operand != NO_ARG || dest_operand != NO_ARG )
                return 0; /* arg1 present || arg2 present */
            break;
        case 15: /* stop */
            if ( src_operand != NO_ARG || dest_operand != NO_ARG )
                return 0; /* arg1 present || arg2 present */
            break;

        default:
            break;
    }

    return 1;
}

/**
 * Check if a label is defined.
 *
 * @param char*             label - The label to test.
 * @param int               addressing_method - The addressing method.
 * @param table_of_signs*   signs_table The signs table.
 * @param int               signs_table_size - The size of the signs table.
 *
 * @return 1 if the label defined, 0 if not.
 */
int is_label_defined(char *label, int addressing_method, table_of_signs *signs_table, int signs_table_size){
    int i;
    char *extracted_label;

    switch ( addressing_method ) {
        case IMMEDIATE: case DIRECT_REGISTER:
            return 1;
        case DIRECT:
            for ( i = 0; i < signs_table_size; i++ ) {
                if( strcmp(signs_table[i].label_name, label) == 0 ) {
                    return 1;
                }
            }
            return 0;
        case MATRIX_ACCESS:
            extracted_label = malloc((sizeof(char) * strlen(label)) + 1);
            extract_mat_label(label, &extracted_label);
            for ( i = 0; i < signs_table_size; i++ ) {
                if( strcmp(signs_table[i].label_name, extracted_label) == 0 ) {
                    free(extracted_label);
                    return 1;
                }
            }
            free(extracted_label);
            return 0;

        default:
            return 1;
    }
}