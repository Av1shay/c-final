#define LINE_MAX 81
#define WORD_MAX 10
#define LABEL_MAX 31
#define NUM_OF_OPERATIONS 16
#define INITIAL_IC 100
#define NO_ARG 20
#define BASE_4_WORD_SIZE 5

/* Addressing methods */
#define IMMEDIATE 0
#define DIRECT 1
#define MATRIX_ACCESS 2
#define DIRECT_REGISTER 3


enum {A = 0, E, R}; /* memory type - A for absolute, E for external, and R for relocatable memory */
enum {LABEL = 1, OPERATION, ARGUMENT};
enum {DATA = 16, STRING, MAT, ENTRY, EXTERN};
enum {FIRST_ARG, SECOND_ARG};

/* struct that represents the signs table */
typedef struct{
    char *label_name;
    int address;
    int external : 2;
    int operation : 2;
} table_of_signs;

typedef struct{
	char *oper_name;
	int oper_num;
} opers;


/*type of word_type, represents a 10-bits "word" in the memory */
typedef struct{
    unsigned int oper : 4;	/* operation name */
    unsigned int amethod_src_operand : 2;	/* addressing method of the source operand */
    unsigned int amethod_dest_operand : 2;	/* addressing method of the destination operand */
    unsigned int memory : 2; /* memory type, absolute, external or relocatable */
} word_t;

/* a general table */
typedef struct{
	char *label_name;
	int address; /* address of the label that will be covert to basis 4 "mozar" as described in the maman booklet */
} data_table;

/* validations */
int check_word(char *, int);
int num_isvalid(char *);
int is_valid_matrix_form(char *arg);
int sign_already_exists(table_of_signs *table, int row_counter, char *sign_name);
int is_address_valid(int, int, int);

/* utilities */
void skip_white_space(const char line[LINE_MAX], int *i);
int get_new_word(char line[LINE_MAX], char single_word[LINE_MAX], int *position);
int get_entry_string(char line[LINE_MAX], char string[LINE_MAX], int *position);
int find_reg_num(char reg[LINE_MAX]);
word_t trans_to_word(int int_num);
int calculate_matrix_size(char *arg);
word_t trans_regs_to_word(int first_register_num, int second_register_num, int memory_type);
void encode_argument(char *arg, int amethod, char *additional_arg, int arg_count, word_t **code_seg, int *seg_size, table_of_signs*, int table_signs_size, data_table **ext_table, int *ext_table_size);
int find_label_address(char *label, table_of_signs*, int table_signs_size, int *is_external);
void convert_word_to_base_four_mozar(word_t word, char **p);
void convert_num_to_base_four_mozar(int num, char **p);

/* db functions */
int insert_sign(table_of_signs **table, int *table_size, char *sign_name, int address, int external, int operation);
void signs_table_update(table_of_signs *table, int table_size, int inst_count);
int update_ent_table(data_table **ent_table, int *ent_size, char *ent_label, table_of_signs *table_signs, int table_of_sings_size);
int update_ext_table(data_table **table, int *table_size, char *label, int address);
int code_insert(word_t **data_code_image,int *size, word_t new_word);
void ob_print(word_t *code_image, word_t *data_image, int inst_count, int data_count, FILE *obj_file);
void e_print(data_table *table, int table_size, FILE *file);