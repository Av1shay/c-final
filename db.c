#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "header.h"


/**
 * Add "sign_name" to the signs table.
 * The sign will be entered in the upcoming position (table_size-1) and will get the parameters "num" for num and type for the type of the address:
 * d (for data), c (for code), n (for non-final external), f (for final external)
 *
 * @param signs_table
 * @param table_size
 * @param sign_name
 * @param address the address of the sign
 * @param external whether it is an external variable or not, 0 - false, 1 - true, 2 - unknown
 * @param operation whether it is an operation or not.
 * @return 1 if the sign entered successfully, -1 if the sign already exists in the table, -2 on memory error.
 */
int insert_sign(table_of_signs **table, int *table_size, char *sign_name, int address, int external, int operation){
	size_t new_table_size;

    /* check if the sign already exists in the table */
    if ( sign_already_exists(*table, (*table_size), sign_name) ) {
        return -1;
    }

	(*table_size)++;	
	new_table_size = (*table_size) * sizeof(table_of_signs);
	*table = (table_of_signs *) realloc(*table, new_table_size); /* memory allocation for the table new cell */

	(*table)[(*table_size)-1].label_name = (char *)malloc(strlen(sign_name)); /*memory allocation for the new sign name */

	if ( !(*table) || !(*table)[(*table_size)-1].label_name ) { /* if there is a memory allocation problem */
		printf("cannot allocate memory for signs table");
		(*table_size)--; /* the size hasn't changed */
		return -2;
	}

	/* else, add the new sign */
	strcpy((*table)[(*table_size)-1].label_name, sign_name);
	(*table)[(*table_size)-1].address = address;
	(*table)[(*table_size)-1].external = external;
	(*table)[(*table_size)-1].operation = operation;

	return 1;
}


/**
 * Updates the table signs so that data will appear after the code (data label = data label + instruction counter).
 *
 * @param signs_table The table to update.
 * @param table_size The size of the table so we'll be able to loop over it.
 * @param inst_count The IC counter.
 */
void signs_table_update(table_of_signs *table, int table_size, int inst_count) {
	int i;
	for ( i = 0; i < table_size; i++ ) {
        /* if the sign is not a part of an operation, then it is a data */
        if ( !table[i].operation ) {
            table[i].address += inst_count;
        }
    }
}

/* ----------------------------------------------------------------------------------------------------------------------------- */

/**
 * Update the entry table.
 *
 * @param signs_table
 * @param signs_size
 * @param entry_table
 * @param entry_size
 * @return
 */
int ent_table_update(data_table **ent_table, int *ent_size, char *ent_label, table_of_signs *table_signs, int table_of_sings_size) {
	int i;
    size_t new_table_size;

    (*ent_size)++;
    new_table_size = (*ent_size) * sizeof(data_table);
    *ent_table = realloc(*ent_table, new_table_size); /* memory allocation for the table new cell */
    (*ent_table)[(*ent_size)-1].label_name = (char *)malloc(strlen(ent_label)); /*memory allocation for the new sign name */

    if ( !(*ent_table) || !(*ent_table)[(*ent_size)-1].label_name ) { /* if there is a memory allocation problem */
        printf("Failed to add %s to the entry table.\n", ent_label);
        (*ent_size)--; /* the size hasn't changed */
        return 0;
    }

    for ( i = 0; i < table_of_sings_size; i++ ) { /*/ search the label on the signs_table*/
        if ( strcmp(table_signs[i].label_name, ent_label) == 0 && !table_signs[i].external ){ /* the label was found, and it's not define as external, update the entry table */
            strcpy((*ent_table)[(*ent_size)-1].label_name, ent_label);
            (*ent_table)[(*ent_size)-1].address = table_signs[i].address;

            return 1;
        }
    }

    (*ent_size)--; /* the size hasn't changed */

    return 0; /* failed to update the entry table */
}

/**
 * Add "new_word_code" (the binary code of the word) to data_code_image (data image or code image)
 *
 * @param data_code_image The data code image to insert the code to.
 * @param size The size of the image.
 * @param new_word_code The code to insert.
 *
 * @return 1 if everything went ok, 0 otherwise.
 */
int code_insert(word_t **data_code_image, int *size, word_t new_word_code){
	(*size)++;
	*data_code_image=(word_t *) realloc((*data_code_image), (*size)*sizeof(word_t));

	if ( !(*data_code_image) ) {
		printf("Cannot allocate memory for segment\n");
		(*size)--; /* the size hasn't changed */
		return 0;
	}
	(*data_code_image)[(*size)-1] = new_word_code;

	return 1;
}


/**
 * Print the code and data segments to the .ob file (first the code than the data).
 *
 * @param word_t*       code_image - Pointer to the code image.
 * @param word_t*       data_image - Pointer to the data image.
 * @param int           inst_count - The size of the code segment.
 * @param int           data_count - The size of the data segment.
 * @param FILE*         obj_file - The .obj file that we want to print the data to.
 */
void ob_print(word_t *code_image, word_t *data_image, int inst_count, int data_count, FILE *obj_file) {
	int i, j;

    /* allocate memory for pointers that will hold the base 4 "mozar" strings */
    char *code_size = malloc(sizeof(char));
    char *data_size = malloc(sizeof(char));
    char *address = malloc(sizeof(char));
    char *machine_code_oper = malloc(sizeof(char));
    char *machine_code_amethod1 = malloc(sizeof(char));
    char *machine_code_amethod2 = malloc(sizeof(char));
    char *machine_code_memory = malloc(sizeof(char));

    fprintf(obj_file, "%-30s", "Base 4 Address");
    fprintf(obj_file, "Base 4 Machine-Code\n");
    fputc('\n', obj_file);

    convert_num_to_base_four_mozar(inst_count, &code_size);
    convert_num_to_base_four_mozar(data_count, &data_size);

    fprintf(obj_file, "%-30s", code_size);
    fprintf(obj_file, "%s", data_size);
    free(code_size);
    free(data_size);

    fputc('\n', obj_file);
    fputc('\n', obj_file);

    /* print the code segment */
	for ( j = 0, i = INITIAL_IC; j < inst_count; j++, i++ ) {
		convert_num_to_base_four_mozar(i, &address);
        convert_num_to_base_four_mozar(code_image[j].oper, &machine_code_oper);
        convert_num_to_base_four_mozar(code_image[j].amethod_src_operand, &machine_code_amethod1);
        convert_num_to_base_four_mozar(code_image[j].amethod_dest_operand, &machine_code_amethod2);
        convert_num_to_base_four_mozar(code_image[j].memory, &machine_code_memory);

		fprintf(obj_file, "%-30s", address);
		fprintf(obj_file, "%s%s%s%s\n", machine_code_oper, machine_code_amethod1, machine_code_amethod2, machine_code_memory);

        /* reset the pointers for the next iteration */
        memset(address, '\0', 1);
        memset(machine_code_oper, '\0', 1);
        memset(machine_code_amethod1, '\0', 1);
        memset(machine_code_amethod2, '\0', 1);
        memset(machine_code_memory, '\0', 1);
	}

    /* print the data segment */
	for (j = 0, i = (inst_count + INITIAL_IC); j < data_count; j++, i++){
        convert_num_to_base_four_mozar(i, &address);
        convert_num_to_base_four_mozar(data_image[j].oper, &machine_code_oper);
        convert_num_to_base_four_mozar(data_image[j].amethod_src_operand, &machine_code_amethod1);
        convert_num_to_base_four_mozar(data_image[j].amethod_dest_operand, &machine_code_amethod2);
        convert_num_to_base_four_mozar(data_image[j].memory, &machine_code_memory);

        fprintf(obj_file, "%-30s", address);
        fprintf(obj_file, "%s%s%s%s\n", machine_code_oper, machine_code_amethod1, machine_code_amethod2, machine_code_memory);

        /* reset the pointers for the next iteration */
        memset(address, '\0', 1);
        memset(machine_code_oper, '\0', 1);
        memset(machine_code_amethod1, '\0', 1);
        memset(machine_code_amethod2, '\0', 1);
        memset(machine_code_memory, '\0', 1);
	}
    free(address);
    free(machine_code_oper);
    free(machine_code_amethod1);
    free(machine_code_amethod2);
    free(machine_code_memory);
}

/* ----------------------------------------------------------------------------------------------------------------------------- */

/**
 * Print entry/extern tables to entry/extern file
 *
 * @param en_xtrn_table
 * @param size
 * @param en_xtrn_file
 */
void e_print(data_table *table, int size, FILE *file){
	int i;
    char *base_4_mozar_address = malloc(sizeof(char));

	for ( i = 0; i < size; i++ ) { /* for each cell of the table */
		convert_num_to_base_four_mozar(table[i].address, &base_4_mozar_address);
        fprintf(file, "%-30s", table[i].label_name);
        fprintf(file, "%s\n", base_4_mozar_address);
	}
    free(base_4_mozar_address);
}

/* ----------------------------------------------------------------------------------------------------------------------------- */

/* End of db.c file */


int update_ext_table(data_table **table, int *table_size, char *label, int address){
    size_t new_table_size;

    (*table_size)++;
    new_table_size = (*table_size) * sizeof(data_table);
    *table = realloc(*table, new_table_size); /* memory allocation for the table new cell */
    (*table)[(*table_size)-1].label_name = (char *)malloc(strlen(label)); /*memory allocation for the new sign name */

    if ( !(*table) || !(*table)[(*table_size)-1].label_name ) { /* if there is a memory allocation problem */
        printf("Failed to add the label %s to te externa label.\n", label);
        (*table_size)--; /* the size hasn't changed */
        return 0;
    }

    strcpy((*table)[(*table_size)-1].label_name, label);
    (*table)[(*table_size)-1].address = (unsigned) address;

    return 1;
}