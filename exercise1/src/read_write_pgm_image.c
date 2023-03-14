#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <omp.h>
#include <mpi.h>
#include <string.h>
#include "read_write_pgm_image.h"   


#define ALIVE 0      //value of alive cells  (white cells)
#define DEAD 255         //value of dead cells (black cells)

#define MAGIC "P5"
#define MAXVAL 255

#define FILE_FORMAT ".pgm"
#define FILE_NAME "snapshot"
#define FOLDER_NAME "dump"


//function to add extension to file name
char * add_extension(char *file_name){
  char *file_name_extension=malloc(strlen(file_name)+strlen(FILE_FORMAT)+1);
  strcpy(file_name_extension, file_name);
  strcat(file_name_extension, FILE_FORMAT);
  return file_name_extension;
}

//function to create the folder where to save the snapshots
void create_folder(){
  struct stat st={0};
  
  if (stat(FOLDER_NAME, &st)==-1){
    mkdir(FOLDER_NAME, 0700);
  }
}


//write image to file, and add it to dump folder
void write_pgm_image( void *image, char *file_name, int rows, int cols){

  create_folder();

  char *full=malloc(strlen(FOLDER_NAME)+strlen(file_name)+2);
  strcpy(full, FOLDER_NAME);
  strcat(full, "/");
  strcat(full, file_name);
  
  FILE* image_file; 
  image_file = fopen(full, "w"); 
  
  // Writing header
  fprintf(image_file, "%2s %d %d\n%d\n", MAGIC, rows, cols, MAXVAL);
  
  // Writing file
  fwrite( image, 1, rows*cols, image_file);  

  fclose(image_file); 
  return ;
}




//write image to file, and add it to current folder, used to create initial conditions
void initial_pgm_image( void *image, char *file_name, int rows, int cols){
  
  FILE* image_file; 
  image_file = fopen(file_name, "w"); 
  
  // Writing header
  fprintf(image_file, "%2s %d %d\n%d\n", MAGIC, rows, cols, MAXVAL);
  
  // Writing file
  fwrite(image, 1, rows*cols, image_file);  

  fclose(image_file); 
  return ;
}


//generate random initial conditions
void * generate_initial(int rows, int cols){
  char *cImage;
  void *ptr;
  unsigned seed;
  
  cImage= (char*) calloc(rows*cols, sizeof(char));
  
  int id=0;
  srand(time(NULL));
  for (int i=0; i<rows; i++){
    for(int j=0; j<cols; j++){
      cImage[id++]=rand()%2==0 ? (unsigned char) ALIVE : (unsigned char) DEAD;
    }
  }
  ptr=(void*)cImage;   
  return ptr;
}

//given a number create string of 5 digits
char* pad(int number){
  static char str[6];
  sprintf(str, "%05d", number);
  return str;
}

//given a gridreturn pointer to char array with the data
void * generate_pointer(int * grid, int rows, int cols){

  char *cImage;
  void *ptr;
  
  cImage=(char*)calloc(rows*cols, sizeof(char)); 
  int id=0;
  for (int i=0; i<rows; i++){
    for (int j=0; j<cols; j++){
      cImage[id++]=(char)grid[i*cols+j];
    }
  }
  ptr=(void*)cImage;
  return ptr;
}

//function to read an image
unsigned char *read_pgm(char *file_name){

  int row=0;
  FILE *fp=fopen(file_name, "rb");
  char magic[3];
  int w, h, mv;
  
  if (!fp) {
    printf("Error: Unable to open file\n");
    exit(1);
  }  

  // Read the header
  if (fscanf(fp, "%2s %d %d\n%d\n", magic, &w, &h, &mv)!=4){
    printf("Error: Invalid header format\n");
    fclose(fp);
    exit(1);
  } 
  
  //allocate memory for image
  unsigned char *data=(unsigned char*)malloc(w*h);
  
  fread(data, 1, w*h, fp);
  fclose(fp);
  return data;
}  
  

//read number of rows specified in the header  
int read_rows(char *file_name){
  FILE *fp=fopen(add_extension(file_name), "rb");   
  char magic[3];
  int rows, cols, max_value;
  if (!fp) {
    printf("Error: Unable to open file\n");
    return 1;
  }  

  // Read the header
  if (fscanf(fp, "%2s %d %d\n%d\n", magic, &rows, &cols, &max_value)!=4){
    printf("Error: Invalid header format\n");
    fclose(fp);
    return 1;
  } 
  
  return rows;

}  

//read number of columns specified in the header
int read_cols(char *file_name){
  FILE *fp=fopen(add_extension(file_name), "rb");   
  char magic[3];
  int rows, cols, max_value;
  if (!fp) {
    printf("Error: Unable to open file\n");
    return 1;
  }  

  // Read the header
  if (fscanf(fp, "%2s %d %d\n%d\n", magic, &rows, &cols, &max_value)!=4){
    printf("Error: Invalid header format\n");
    fclose(fp);
    return 1;
  } 
  
  return cols;
}  


//generate the image with random initial conditions
void generate_image(char *file_name, int width, int height){
  printf("initializing condition\n");
  void *image=generate_initial(width, height);
  printf("1. The image has been generated\n");
  initial_pgm_image(image, file_name, width, height);
}



//read image and save it in an array
void readin_array(int* grid, char *file_name, int rows, int cols){
  unsigned char *image_data=read_pgm(add_extension(file_name));
  for (int i = 0; i < rows * cols; i++){
    grid[i] = (int) image_data[i];
  }
}

//create the name of the save file and save the image
void save_name(int * grid, int rows, int cols, int step){

  void *image=generate_pointer(grid, rows, cols);//segfault in here

  char *padded=pad(step);
  char *full_file_name=(char *)malloc(strlen(FILE_NAME)+strlen(padded)+strlen(FILE_FORMAT)+1);
  
  strcpy(full_file_name, FILE_NAME);
  strcat(full_file_name, padded);
  strcat(full_file_name, FILE_FORMAT);

  write_pgm_image(image, full_file_name, rows, cols);
}


//compute the full grid and then save the image
void save_image(int *local_grid_ghost, int local_rows, int local_cols, int rows, int cols, int rank, int size, int offset, int step){
  int * local_grid=(int*)malloc(local_rows*local_cols*sizeof(int));

  int id=0;
  for (int i=1; i<=local_rows; i++){
    for (int j=1; j<=local_cols; j++){
      local_grid[id]=local_grid_ghost[i*(local_cols+2)+j];
      id++;
    }
  }

  if (rank != 0){
    MPI_Send(&local_grid[0], local_rows*local_cols, MPI_INT, 0, 0, MPI_COMM_WORLD);
  }else{
    int *full_grid_temp=(int*)malloc(rows*cols*sizeof(int));
    
    for (int i=0; i<local_rows*local_cols; i++){
      full_grid_temp[i]=local_grid[i];
    }

    for (int i=1; i<size; i++){
      if (i<size-1){
        MPI_Recv(&full_grid_temp[i*local_rows*local_cols], local_rows*local_cols, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }else{
        MPI_Recv(&full_grid_temp[i*local_rows*local_cols], (local_rows+offset)*local_cols, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);    
      }
    }

    save_name(full_grid_temp, rows, cols, step);

    free(full_grid_temp);
  }
}





