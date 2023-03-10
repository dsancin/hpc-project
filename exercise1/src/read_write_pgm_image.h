#ifndef READ_WRITE_PGM_DOT_H    /* This is an "include guard" */
#define READ_WRITE_PGM_DOT_H    /* prevents the file from being included twice. */
                     /* Including a header file twice causes all kinds */
                     /* of interesting problems.*/

/**
 * This is a function declaration.
 * It tells the compiler that the function exists somewhere.
 */

//add extension to the file name
char * add_extension(char *file_name);   

//function to create the folder where to save the snapshots
void create_folder();

//function to write image to file, and add it to dump folder
void write_pgm_image( void *image, char *file_name, int rows, int cols);

//function write image to file, and add it to current folder, used to create initial conditions
void initial_pgm_image( void *image, char *file_name, int rows, int cols);

//generate random initial conditions
void * generate_initial(int rows, int cols);

//function that given a number create string of 5 digits
char* pad(int number);

//function that given a gridreturn pointer to char array with the data
void * generate_pointer(int * grid, int rows, int cols);

//function to read an image
unsigned char *read_pgm(char *file_name);

//function to read number of rows specified in the header  
int read_rows(char *file_name);

//function to read number of cols specified in the header  
int read_cols(char *file_name);

//function that generate the image with random initial conditions
void generate_image(char *file_name, int width, int height);

//function to read image and save it in an array
void readin_array(int* grid, char *file_name, int rows, int cols);

//function to create the name of the save file and save the image
void save_name (int * grid, int rows, int cols, int step);

//function that compute the full grid and then save the image
void save_image(int *local_grid_ghost, int local_rows, int local_cols, int rows, int cols, int rank, int size, int offset, int step);


#endif /* READ_WRITE_PGM_DOT_H */

