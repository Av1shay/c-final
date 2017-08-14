#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "header.h"


/*
 * Using global variables most of the time consider as bad practice,
 * but if it's done right, they can be very efficient.
 */

table_of_signs *table_signs;
int table_signs_size;

word_t *data_seg; /* data segment */
int dc; /* data counter */

word_t *code_seg; /* code segment */
int ic;  /* instruction counter */

data_table *ent; /* entry table */
int ent_size;  /* size of entry table */

data_table *ext; /* extern table */
int ext_size;  /* size of extern table */

/**
 * First assembler scan.
 *
 * @param FILE* fp - The file to scan from.
 *
 * @return int 0 if everything went OK, 1 otherwise.
 */
int first_scan(FILE *fp){
    int i;
    int matrix_size;
    int line_counter = 0; /* line number */
	int error = 0; /* 1 if we found an error */
	char line[LINE_MAX], temp[LINE_MAX], label[LINE_MAX], oper[LINE_MAX], arg1[LINE_MAX], arg2[LINE_MAX];
	int length; /* length of current word */
	int valid; /* save the result of the isvalid */
	int pos; /* the position on the current line*/
	int is_label; /* 1 if we have label on the current line */
    int register_arg_flag; /* indicates the first argument was a register */
    int insert_status; /* whether a sign insert to the table successfully */
	ic = 100; dc = 0;

	while ( fgets(line, LINE_MAX, fp) ) { /* get line */
		pos = 0;
        register_arg_flag = 0; /* not register yet */
		is_label = 0; /* not label yet */
		line_counter++; /* line counter is increased */

		skip_white_space(line, &pos); /* skip to the first word */
		length = get_new_word(line, temp, &pos); /* get the first word */

		if ( length == 0 ) /* if it's mark or empty line */
            continue;

		/* ------------ LABEL HANDLING --------------- */
		if ( temp[length-1] == ':' ){	/* if we read label */
			temp[length-1] = '\0'; /* "delete" the : by putting \0 there */
			strcpy(label,temp);
			if ( check_word(label, LABEL) == -1 ) { /* check if it's valid label */
				printf("line %d:\tinvalid label: '%s'\n",line_counter,label);
				error = 1;
				continue;
			}
			is_label = 1; /* mark there is a label */
			skip_white_space(line, &pos); /* skip to the next word */
			get_new_word(line, oper, &pos); /* read the operation */
		}
		else {    /* we read the operation immediately */
            strcpy(oper, temp);
        }

		/* ------------ CHECK OPERATION --------------- */
		if ( ( valid = check_word(oper, OPERATION)) == -1 ){ /* check if the word is invalid */
			fprintf(stderr, "invalid operation: %s\n", oper);
			error = 1;
			continue;
		}

		/* ------------ DATA HANDLING --------------- */
		if ( valid == DATA ) { /* the operation we read was .data */

			if ( is_label == 1 ) { /* we have a label on this line, insert it to our table of signs */
                if ( (insert_status = insert_sign(&table_signs, &table_signs_size, label, dc, 0, 0)) != 1 ) {
                    if (insert_status == -1) {
                        printf("line %d:\tThe sign %s declared more then once\n", line_counter, label);
                    }
                    error = 1;
                    break;
                }
            }

			skip_white_space(line, &pos);
			length = get_new_word(line, arg1, &pos); /* get the next word (of data) */
			while ( length > 0 ) { /* while we read a word */
				int num;
				word_t op_num;
				if ( !num_isvalid(arg1) ) { /* if it's invalid number */
					printf("line %d:\t invalid number: %s\n", line_counter, arg1);
					error=1;
					break;
				}
				num = atoi(arg1); /* translate the number */
				op_num = trans_to_word(num); /* change it to word_type */
				if ( !code_insert(&data_seg, &dc, op_num) ) { /* add the data word to the data table */
					error = 1;
					break;
				}
				skip_white_space(line, &pos);
				length = get_new_word(line, arg1, &pos); /* get the next word (of data) */
			}
			skip_white_space(line, &pos);
			if ( line[pos] != '\n' ) { /* if the last char wasn't \n */
				printf("line %d:\tinvalid list number\n",line_counter);
				error=1;
				break;
			}
			continue;
		}

		/* ------------ STRING HANDLING --------------- */
		if ( valid == STRING ) { /* the word was .string */
			if ( is_label ) { /* we have a label on this line, insert it to our table of signs */
                if ( (insert_status = insert_sign(&table_signs, &table_signs_size, label, dc, 0, 0)) != 1 ) {
                    if (insert_status == -1) {
                        printf("line %d:\tThe sign %s declared more then once\n", line_counter, label);
                    }
                    error = 1;
                    break;
                }
            }
			skip_white_space(line, &pos);
			length = get_entry_string(line, arg1, &pos);	/* get the string */
			if ( length < 0 ) {
				printf("line %d:\tString should start and end with \"\n",line_counter);
				error=1;
				break;
			}
			for ( i = 0; i < length; i++ ) { /* for each char on the string (include \0) */
				int num = arg1[i];
                word_t op_num;

				op_num = trans_to_word(num); /* change it to word_type */
				if ( !code_insert(&data_seg, &dc, op_num) ) { /* add this sign to data table */
					error = 1;
					break;
				}
			}
			skip_white_space(line, &pos);
			length = get_new_word(line, arg1, &pos);
			if ( length > 0 ) { /* if there was another word after the string */
				printf("line %d:\t.string should have one argument\n", line_counter);
				error = 1;
				break;
			}
			continue;
		}

        /* ------------ MATRIX HANDLING --------------- */
        if ( valid == MAT ) { /* the word was .mat */

            if ( is_label == 1 ) { /* we have a label on this line, insert it to our table of signs */
                if ( (insert_status = insert_sign(&table_signs, &table_signs_size, label, dc, 0, 0)) != 1 ) {
                    if ( insert_status == -1 ) {
                        printf("line %d:\tThe sign %s declared more then once\n", line_counter, label);
                    }
                    error = 1;
                    break;
                }
            }

            skip_white_space(line, &pos);
            get_new_word(line, arg1, &pos); /* get matrix rows/columns count */
            if ( !is_valid_matrix_form(arg1) ) {
                printf("line %d:\tThe matrix declaration is not valid.\n", line_counter);
                error = 1;
                break;
            }

            matrix_size = calculate_matrix_size(arg1);
            if ( matrix_size < 1 ) {
                printf("line %d:\tMatrix rows and columns must be greater then zero.\n", line_counter);
                error = 1;
                break;
            }

            skip_white_space(line, &pos);
            length = get_new_word(line, arg2, &pos);
            i = 0; /* this will tell us how many numbers there are at the end */
            while ( length > 0 ) { /* while we read a word */
                unsigned int num;
                word_t op_num;
                if ( !num_isvalid(arg2) ) { /* if it's invalid number */
                    printf("line %d:\t invalid number: %s\n", line_counter, arg2);
                    error=1;
                    break;
                }
                num = (unsigned int) atoi(arg2); /* translate the number */
                op_num = trans_to_word(num); /* change it to word_type */
                if ( !code_insert(&data_seg, &dc, op_num) ) { /* add the data number to the data table */
                    error = 1;
                    break;
                }
                skip_white_space(line, &pos);
                length = get_new_word(line, arg2, &pos); /* get the next number */
                i++;
            }

            skip_white_space(line, &pos);
            if ( line[pos] != '\n' || i != matrix_size ) { /* if the last char wasn't \n, or if the numbers count doesn't fit the matrix size */
                printf("line %d:\tError trying to assign list number to the matrix %s, the list is invalid.\n", line_counter, label);
                error = 1;
                break;
            }
            continue;
        }

		/* ------------ ENTRY HANDLING --------------- */
		if ( valid == ENTRY ) {
            /*
             * If the word was .entry, we don't have anything to do right now,
             * we will handle this case in the second scan.
             */
			continue;
		}

		/* ------------ EXTERN HANDLING --------------- */
		if ( valid == EXTERN ) { /*the word was .extern */
			skip_white_space(line, &pos);
            get_new_word(line, arg1, &pos);
			if ( (insert_status = insert_sign(&table_signs, &table_signs_size, arg1, 0, 1, 2)) != 1 ){  /* add the label to the signs table */
				if ( insert_status == -1 ) {
                    fprintf(stderr, "line %d:\tThe sign %s declared more then once\n", line_counter, arg1);
                }
                error = 1;
				break;
			}
			skip_white_space(line, &pos);
			length = get_new_word(line, arg1, &pos);
			if ( length > 0 ){ /* if there was another word after the extern */
				printf("line %d:\t.extern should have one argument\n",line_counter);
				error=1;
				break;
			}
			continue;
		}
		/* ------------ OPERATION HANDLING --------------- */
        /* the word was an operation */
        if ( is_label == 1 ) { /* we have a label on this line */
            if ( (insert_status = insert_sign(&table_signs, &table_signs_size, label, ic, 0, 1)) != 1 ) {
                if ( insert_status == -1 ) {
                    printf("line %d:\tThe sign %s declared more then once\n", line_counter, label);
                }
                error = 1;
                break;
            }
        }
        ic++; /* we surely have a new word for the operation */
        skip_white_space(line, &pos);

		/* ------------ ARG1 HANDLING --------------- */
		get_new_word(line, arg1, &pos);
		if ( (valid = check_word(arg1, ARGUMENT)) == -1 ) { /* if the argument1 is invalid*/
			printf("line %d:\tinvalid argument: '%s'\n", line_counter, arg1);
			error = 1;
			continue;
		}
        switch ( valid ) {
            case DIRECT: case IMMEDIATE:
                ic++; /* we should have new word for the label's address (or specified number) */
                break;
            case MATRIX_ACCESS:
                ic += 2; /* we need two more spaces - one for the address of the matrix, and second for the relevant row/column */
                break;
            case DIRECT_REGISTER:
                ic++;
                register_arg_flag = 1;
            default:
                /* TODO check if the default is needed */
                break;
        }
		skip_white_space(line, &pos);

		/* ------------ ARG2 HANDLING --------------- */
		get_new_word(line, arg2, &pos);
        if ( strlen(arg2) == 0 ) { /*if we have only one argument */
            continue;
        }
		if ( (valid = check_word(arg2, ARGUMENT) ) == -1 ) { /*/if the argument2 is invalid*/
			printf("line %d:\tinvalid argument: '%s'\n", line_counter, arg2);
			error=1;
			continue;
		}
        switch ( valid ) {
            case DIRECT:
                ic++; /* we should have new word for the label's address (or specified number) */
                break;
            case MATRIX_ACCESS:
                ic += 2; /* we need two more spaces - one for the address of the matrix, and second for the relevant row/column */
                break;
            case DIRECT_REGISTER:
                if ( !register_arg_flag ) {
                    ic++;
                }
            default:
                /* TODO check if the default is needed */
                break;
        }

		/* ------------ EXCEPTION ARGS  --------------- */
		skip_white_space(line, &pos);
		if ( (line[pos] != '\n') && (line[pos] != '\0') ){ /*if after the 2 arguments we have more */
			printf("line %d:\ttoo much parameters\n", line_counter);
			error = 1;
			break;
		}
	}

	/* update the table of signs so the data will be placed after to code segment */
	signs_table_update(table_signs, table_signs_size, ic);

	return error;
}


/**
 * Second assembler scan.
 *
 * @param FILE* fp - The file to scan from.
 *
 * @return int 0 if everything went OK, 1 otherwise.
 */
int second_scan(FILE *fp){
    int line_counter = 0; /* line number */
    int error = 0; /* errors indicator */
    char line[LINE_MAX], temp[LINE_MAX];	/* the line, and temp array to save each word */
    char oper[LINE_MAX], arg1[LINE_MAX], arg2[LINE_MAX];
    int length; /* length of current word *//*table_format_type *table_signs;*/
    int pos; /* the position on the current line */
    int arg1_exists;
    int arg2_exists;
    int address;
    word_t current_code; /* current code */

    rewind(fp);
    ic = 0;


    int arg1_amethod, arg2_amethod;

    while ( fgets(line, LINE_MAX, fp) ) { /* get line */

        pos = arg1_exists = arg2_exists =  arg1_amethod = arg2_amethod = 0;
        current_code.oper = current_code.amethod_src_operand = current_code.amethod_dest_operand = current_code.memory = 0;
        line_counter++; /* line counter is increased */

        /* reset the arguments */
        arg1[0] = '\0';
        arg2[0] = '\0';

        skip_white_space(line, &pos); /* skip to the first word */
        length = get_new_word(line, temp, &pos); /* get the first word */

        if ( length == 0 ) {/* if it's mark or empty line */
            continue;
        }

        /* ------------ LABEL HANDLING --------------- */
        if (temp[length-1]==':'){	/* we read label */
            skip_white_space(line, &pos);
            get_new_word(line, oper, &pos); /* read the operation */
        } else {    /* we read the operation immediately */
            strcpy(oper, temp);
        }

        /* ------------ CHECK OPERATION --------------- */
        address = check_word(oper, OPERATION);

        /* do nothing in these cases */
        if ( address == DATA || address == STRING || address == MAT || address == EXTERN ) {
            continue;
        }


        /* ------------ ENTRY HANDLING --------------- */
        if ( address == ENTRY ) {
            skip_white_space(line, &pos);
            get_new_word(line, arg1, &pos);
            if ( ! ent_table_update(&ent, &ent_size, arg1, table_signs, table_signs_size) ) { /* update the ent table */
                printf("line %d:\tError trying to add value to the entry table.\n", line_counter);
                error = 1;
                break;
            }
            length = get_new_word(line, arg2, &pos);
            if ( length > 0 ) { /* if there was another word after the entry */
                printf("line %d:\t.entry should have one argument\n", line_counter);
                error = 1;
                break;
            }
            continue;
        }

        /* ------------ OPERATION HANDLING --------------- */
        address = check_word(oper, OPERATION);

        current_code.oper = (unsigned) address;
        current_code.memory = 0;

        /* get arg1 */
        skip_white_space(line, &pos);
        get_new_word(line, arg1, &pos);

        /* get arg2 */
        skip_white_space(line, &pos);
        get_new_word(line, arg2, &pos);

        /* check what we have */
        if ( strlen(arg1) > 0 ) {
            arg1_exists = 1;
            arg1_amethod = (unsigned) check_word(arg1, ARGUMENT);

            if ( strlen(arg2) > 0 ) { /* check for arg2 either */
                arg2_exists = 1;
                arg2_amethod = (unsigned) check_word(arg2, ARGUMENT);
            }
        }

        if ( ! is_address_valid(address, arg1_exists ? arg1_amethod : NO_ARG, arg2_exists ? arg2_amethod : NO_ARG) ) {
            printf("line %d:\tinvalid address\n", line_counter);
            error = 1;
            continue;
        }

        /* if we have two arguments, the first one is going to be the source operand */
        if ( arg1_exists && arg2_exists ) {
            current_code.amethod_src_operand = (unsigned) arg1_amethod;
            current_code.amethod_dest_operand = (unsigned) arg2_amethod;
        }
        /* if we have one argument, he is going to be destination operand */
        else if ( arg1_exists &&  !arg2_exists ) {
            current_code.amethod_dest_operand = (unsigned) arg1_amethod;
        }

        /* encode the operation */
        if ( ! code_insert(&code_seg, &ic, current_code) ) {
            printf("line %d:\tFailed to insert code.\n", line_counter);
            error = 1;
            continue;
        }

        /* encode the arguments */
        if ( arg1_exists ) {
            encode_argument(arg1, arg1_amethod, arg2, FIRST_ARG, &code_seg, &ic, table_signs, table_signs_size, &ext, &ext_size);
        } else { /* if the destination operand is't exists, it must be the last word in the line, we can go to the next iteration */
            continue;
        }

        if ( arg2_exists ) {
            encode_argument(arg2, arg2_amethod, arg1, SECOND_ARG, &code_seg, &ic, table_signs, table_signs_size, &ext, &ext_size);
        } else {
            continue; /* no need to check for exception args if arg2 isn't exists */
        }


        /* ------------ EXCEPTION ARGS  --------------- */
        skip_white_space(line, &pos);
        if ( line[pos] != '\n' && line[pos] != '\0' ) { /*if after the 2 arguments we have more */
            printf("line %d:\ttoo much parameters\n",line_counter);
            error = 1;
            break;
        }
    }

    return error;
}

/**
 * Handling User Interactive. Get Files, Processing, And Generating Error & INFO.
 *
 * @param int       argc - Number of argument.
 * @param char**    argv - Array of arguments.
 *
 * @return 0 if everything went OK, 1 otherwise.
 */
int main(int argc, char *argv[]){
	FILE *fp;  /*the source file*/
	FILE *obj_file;  /*the object file*/
	FILE *entry_file;  /*the ENTRY file*/
	FILE *extern_file;  /*the EXTERN file*/
	int i;

	for ( i = 1; i < argc; i++ ) {  /*for each file*/
		char *name;

        /* allocate memory for the tables that will hold the data */
		table_signs = (table_of_signs *) malloc(sizeof(table_of_signs));
		code_seg = (word_t *) malloc(sizeof(word_t)); /* code segment in OB file */
		data_seg = (word_t *) malloc(sizeof(word_t));  /* data segment in OB file */
		ent = (data_table *) malloc(sizeof(data_table));  /* table on ENTRY file */
		ext = (data_table *) malloc(sizeof(data_table));  /* table on EXTERN file */

		if ( !code_seg || !data_seg || !table_signs || !ent || !ext ) {
			fprintf(stderr, "Cannot allocate memory.\n");
            return 1;
		}

		table_signs_size = ent_size = ext_size = 0;  /*the lines of ENTRY and EXTERN file*/

		name = malloc(strlen(argv[i])+3);/*file name*/
		strcpy(name, argv[i]);  /*copy the file name*/
		name = strcat(name,".as");  /*add .as*/
		if (!(fp = fopen(name,"r") )) {  /*oper .as for read*/
			fprintf(stderr, "Cannot open file: %s\n", name);
			continue;
		}

		if ( first_scan(fp) == 1 ) {  /* if there was a problem on first scan */
			fclose(fp);
			printf("  ===========\n");
			free(code_seg);
			free(data_seg);
			free(table_signs);
			continue;
		}
		if ( second_scan(fp) == 1 ) {  /* if there was a problem on second scan */
			fclose(fp);
			printf("  ===========\n");
			free(code_seg);
			free(data_seg);
			free(table_signs);
			continue;
		}

		/*  ------------ So Far So Good --------------- */
		if ( ic + dc > 0 ) {  /* if the length of the OB file is >0 */
			strcpy(name, argv[i]);
			name = strcat(name, ".ob");  /* add .ob */
			if ( !(obj_file = fopen(name,"w") ) ) { /*  open the file for write */
				printf( "ERROR: cannot open file: %s\n", name);
				return 1;
			}
			ob_print(code_seg, data_seg, ic, dc, obj_file);  /* print to OB */
			printf("INFO: %s was created.\n",name);
			fclose(obj_file);
		}

		if ( ent_size > 0 ){  /* if the length of the ENTRY file is > 0 */
			name = malloc(strlen(argv[i])+4);
			strcpy(name, argv[i]);
			name = strcat(name, ".ent");  /* add .ent */
			if ( !(entry_file = fopen(name,"w")) ) {
				fprintf(stderr, "Cannot open file: %s\n", name);
				return 1;
			}
			e_print(ent, ent_size, entry_file);  /* print to ENTRY */
			printf("INFO: %s was created.\n",name);
			fclose(entry_file);
		}

        if ( ext_size > 0 ) {  /* if the length of the EXTERN file is >0 */
			strcpy(name, argv[i]); 
			name=strcat(name,".ext");  /* add .ext */
			if ( !(extern_file = fopen(name,"w")) ) {
				fprintf(stderr, "Cannot open file: %s\n", name);
				return 0;
			}
			e_print(ext, ext_size, extern_file);   /*print to EXTERN*/
			printf("INFO: %s was created.\n", name);
			fclose(extern_file);
		}
		 /*------------ Free Allocation & Close Files ---------------*/
		free(code_seg);
		free(data_seg);
		free(table_signs);
		fclose(fp);	
		printf("  ===========\n");
	}
	return 0;
}
