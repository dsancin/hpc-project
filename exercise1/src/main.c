#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <mpi.h>
#include <string.h>
#include <getopt.h>
#include "read_write_pgm_image.h"   
#include "evolution.h"



//?should the values be swapped?
#define ALIVE 0      //value of alive cells  (white cells)
#define DEAD 255         //value of dead cells (black cells)

#define INIT 0         
#define RUN 1

#define ORDERED 0
#define STATIC 1

#define FILE_FORMAT ".pgm"


int action=INIT;  //when init it initialize the matrix, when it's run it plays the game
int k=1000;       //dimension of the grid
int n=100;        //number of iterations of the game
int e=STATIC;     //when static game runs static evolution, when ordered game runs ordered evolution  
int s=0;          //every s-th iteration save the current grid
char *file_name=NULL;   //name of the file


//function to parse the command line arguments
void get_args (int argc, char **argv) {
  char *optstring= "irk:f:n:e:s:";
  
  int c;
  
  while ((c=getopt(argc, argv, optstring)) !=-1){
    switch(c){
    case 'i':
      action=INIT;
      break;
    case 'r':
      action=RUN;
      break;
    case 'k':
      k=atoi(optarg);
      break;
    case 'e':
      e=atoi(optarg);
      break;      
    case 'f':
      file_name=(char*)malloc(sizeof(optarg)+1);
      sprintf(file_name, "%s", optarg);
      break;
    case 'n':
      n=atoi(optarg);
      break;    
    case 's':   
      s=atoi(optarg);
      break;    
    default:
      printf("argument -%c not known/n", c); break;
    }
  }
}


int main(int argc, char **argv){

  int rank, size;
  MPI_Init(&argc, &argv);                          

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);    
  MPI_Comm_size(MPI_COMM_WORLD, &size);    

  get_args(argc, argv);  //parse command line arguments

  //check if mandatory arguments (filename) are present  
  if (rank==0 && file_name==NULL){
    printf("filename is mandatory");
    MPI_Abort(MPI_COMM_WORLD, 1);
  }  

  MPI_Barrier(MPI_COMM_WORLD);  

  //if given the instruction processor 0 generate initial conditions   
  if (action==INIT && rank==0){  
    char *new_file_name=(char*) malloc(strlen(file_name)+strlen(FILE_FORMAT)+1);
    strcpy(new_file_name, file_name);
    strcat(new_file_name, FILE_FORMAT);   
    generate_image(new_file_name, k, k); 
    free(new_file_name);
  }

  //run static evolution
  if (action==RUN && e==STATIC){
    int rows;   
    int cols;

    //process 0 get the number of rows and columns of image and sends them to other processes    
    if (rank==0){
      rows=read_rows(file_name);
      cols=read_cols(file_name);
      //cycle to send
      for (int i=1; i<size; i++){
        MPI_Send(&rows, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        MPI_Send(&cols, 1, MPI_INT, i, 0, MPI_COMM_WORLD);    
      } 
    }else{      //other processes receive 
      MPI_Recv(&rows, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Recv(&cols, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    int grid_size=rows*cols;    //dimension of the grid
    int local_rows=rows/size;   //number of rows on which each process will work
    int local_cols=cols;        //number of rows on which each process will work
    
    //check if the number of rows is divisible by number of processes, if not the last process will work on remaining rows
    int offset=rows%size;
    if (rank==size-1){                 
      local_rows=local_rows+offset;
    }

    int local_grid_size=local_rows*local_cols;      //size of the local grid of each process
    
    //sizes including ghost rows and columns
    int local_rows_ghost=local_rows+2;
    int local_cols_ghost=local_cols+2;
    int local_grid_size_ghost=local_rows_ghost*local_cols_ghost;
    
    //get the processes that have neighbouring rows of current process
    int upper_rank=(rank==0)? size-1 : rank-1;
    int lower_rank=(rank==size-1)? 0 : rank+1;
    
    //memory allocations 
    int *local_grid_current=(int *) malloc(local_grid_size*sizeof(int));        //local grid
    int *local_grid_ghost=(int *) malloc(local_grid_size_ghost*sizeof(int));    //local grid whith ghosts
    int *local_grid_next=(int *) malloc(local_grid_size_ghost*sizeof(int));       //local grid after evolution

    //process 0 reads image, send local grids to other processes and keep one for itself
    if (rank==0){
      int *full_grid_current=(int*) malloc(grid_size*sizeof(int));
      readin_array(full_grid_current, file_name, rows, cols);
    
      for (int i=0; i<local_grid_size; i++){
        local_grid_current[i]=full_grid_current[i];   
      }

    //send local grids to other processes  
      for (int i=1; i<size; i++){     
        if (i<size-1){
          MPI_Send(&full_grid_current[i*local_grid_size], local_grid_size, MPI_INT, i, 0, MPI_COMM_WORLD);
        }else{
          MPI_Send(&full_grid_current[i*local_grid_size], (local_rows+offset)*local_cols, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
      }

      free(full_grid_current);
    }
    //receive the local grids from process 0
    if (rank !=0){
      MPI_Recv(&local_grid_current[0], local_grid_size, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    //copy the local grid into the local grid with ghost rows/cols
    for (int i=0; i<local_rows; i++){
      for (int j=0; j<local_cols; j++){
        local_grid_ghost[(i+1)*local_cols_ghost+(j+1)]=local_grid_current[i*local_cols+j];
      }
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
   
    //start the actual evolution
    double t_start=MPI_Wtime();     //start time
    for (int step=1; step<=n; step++){
      //exchange ghost rows
      exchange_ghost_rows(local_grid_ghost, local_rows_ghost, local_cols_ghost, upper_rank, lower_rank);

      MPI_Barrier(MPI_COMM_WORLD);
     
      //compute ghost cols
      compute_ghost_cols(local_grid_ghost, local_rows_ghost, local_cols_ghost);

      //apply static evolution algorithm
      static_evo(local_grid_ghost, local_grid_next, local_rows_ghost, local_cols_ghost);

      //copy new state array into the current array to start a new iteration
      memcpy(local_grid_ghost, local_grid_next, local_grid_size_ghost*sizeof(int));

      //save the image every s-th step or evey step, depending on command line argument 
      if ((s!=0 && step%s==0) || step==n){
        save_image(local_grid_ghost, local_rows, local_cols, rows, cols, rank, size, offset, step);  
      }
    }
    if(rank==0){
      printf("static evolution took %f using %d processes and %d threads \n", MPI_Wtime()-t_start, size, omp_get_max_threads());
    }    
    //free allocated memory
    free(local_grid_current);
    free(local_grid_ghost);
    free(local_grid_next);
  }  //end of if that start static run, row 98
    
    //ordered evolution, it's serial so all work will be done by master process 
    if (action==RUN && e==ORDERED && rank==0){
     
     int rows=read_rows(file_name);
     int cols=read_cols(file_name);  
    
     int grid_size=rows*cols;      //size of the grid of each process
    
     //sizes including ghost rows and columns
     int rows_ghost=rows+2;
     int cols_ghost=cols+2;
     int grid_size_ghost=rows_ghost*cols_ghost;     
     
     //allocate memory for grid and grid with ghosts
     int *grid=(int*)malloc(grid_size*sizeof(int));
     int *grid_ghost=(int*)malloc(grid_size_ghost*sizeof(int));
     
     //read grid from the file
     readin_array(grid, file_name, rows, cols);

     //copy the grid in the grid with ghosts
     for (int i=0; i<rows; i++){
       for (int j=0; j<cols; j++){
         grid_ghost[(i+1)*cols_ghost+(j+1)]=grid[i*cols+j];
       }
     }

     //start the actual evolution
     double t_start=MPI_Wtime();     //start time 
     for (int step=1; step<=n; step++){
     
       //compute ghost rows and columns
       compute_ghost_rows(grid_ghost, rows, cols, rows_ghost, cols_ghost);
       compute_ghost_cols(grid_ghost, rows_ghost, cols_ghost);   
    
       //apply ordered evolution algorithm
       ordered_evo(grid_ghost, rows_ghost, cols_ghost, rows, cols);    
    
       //save the image every s-th step or evey step, depending on command line argument 
       if ((s!=0 && step%s==0) || step==n){
         int id=0;
         //update grid from grid with ghost, on which the algorithm was applied
         for (int i=1; i<=rows; i++){
           for (int j=1; j<=cols; j++){
             grid[id]=grid_ghost[i*cols_ghost+j];
             id++;
           }  
         }
         save_name(grid, rows, cols, step); 
       }
     }
     printf("ordered evolution took %f", MPI_Wtime()-t_start);     
     //free allocated memory
     free(grid);
     free(grid_ghost);
   }  //end of if that starts the ordered evolution  
   
   MPI_Finalize();
   return 0; 
}    
    
    
    
    
