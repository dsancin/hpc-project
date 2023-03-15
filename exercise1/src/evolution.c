#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <mpi.h>
#include <string.h>

#define ALIVE 0
#define DEAD 255

void exchange_ghost_rows(int *local_grid_ghost, int local_rows_ghost, int local_cols_ghost, int upper_rank, int lower_rank){
  MPI_Request request1, request2;
  MPI_Isend(&local_grid_ghost[local_cols_ghost], local_cols_ghost, MPI_INT, upper_rank, 0, MPI_COMM_WORLD, &request1);
  MPI_Irecv(&local_grid_ghost[(local_rows_ghost-1)*local_cols_ghost], local_cols_ghost, MPI_INT, lower_rank, 0, MPI_COMM_WORLD, &request1);
  MPI_Isend(&local_grid_ghost[(local_rows_ghost-2)*local_cols_ghost], local_cols_ghost, MPI_INT, lower_rank, 1, MPI_COMM_WORLD, &request2);  
  MPI_Irecv(&local_grid_ghost[0], local_cols_ghost, MPI_INT, upper_rank, 1, MPI_COMM_WORLD, &request2);
  MPI_Wait(&request1, MPI_STATUS_IGNORE);
  MPI_Wait(&request2, MPI_STATUS_IGNORE);
}

//compute ghost columns
void compute_ghost_cols(int *local_grid_ghost, int local_rows_ghost, int local_cols_ghost){
  for (int i=0; i<local_rows_ghost; i++){
    local_grid_ghost[i*local_cols_ghost]=local_grid_ghost[(i+1)*local_cols_ghost-2];
    local_grid_ghost[(i+1)*local_cols_ghost-1]=local_grid_ghost[i*local_cols_ghost+1];
  }
}

//compute ghost rows
void compute_ghost_rows(int *grid, int rows, int cols,  int rows_ghost, int cols_ghost){
  for (int i=1; i<=cols; i++){
    grid[i]=grid[cols_ghost*rows+i];
    grid[cols_ghost*(rows_ghost-1)+i]=grid[cols_ghost+i];
  }
}


//count the number of alive neighbours
int alive_neigh(int *grid, int i, int j, int cols){
  //this compute the sum of the elements of all neighbouring cells
  int neighbours=grid[(i-1)*cols+(j-1)] + grid[(i-1)*cols+j] + grid[(i-1)*cols+(j+1)]+       
                 grid[i*cols+(j-1)]                          + grid[i*cols+(j+1)]+                 
                 grid[(i+1)*cols+(j-1)] + grid[(i+1)*cols+j] + grid[(i+1)*cols+(j+1)];          
                        //i,j row/cols index  cols is length of row 
                
                     
                
                
  return (8-neighbours/255);   //since dead=255 and there are 8 neigbours, the number of alive neighbours will be 8-neighbours/255                   
}                     
              
//apply static evolution algorithm              
void static_evo(int *grid, int *grid_next, int rows, int cols){
  #pragma omp parallel for schedule(static) 
  for (int i=1; i<rows-1; i++){                   
    for (int j=1; j<cols-1; j++){ 
      int count=alive_neigh(grid, i, j, cols);
      if (grid[i*cols+j]==ALIVE && (count==2 || count==3)){
        grid_next[i*cols+j]=ALIVE;                
      }else if(grid[i*cols+j]==DEAD && count==3){           
        grid_next[i*cols+j]=ALIVE;
      }else{
        grid_next[i*cols+j]=DEAD;
      }
    }
  }          
}         


//apply ordered evolution algorithm              
void ordered_evo(int *grid_ghost, int rows_ghost, int cols_ghost, int rows, int cols){
  for (int i=1; i<rows_ghost-1; i++){                   
    for (int j=1; j<cols_ghost-1; j++){ 
      int count=alive_neigh(grid_ghost, i, j, cols_ghost);
      if (grid_ghost[i*cols_ghost+j]==ALIVE && (count==2 || count==3)){
        grid_ghost[i*cols_ghost+j]=ALIVE;
        compute_ghost_rows(grid_ghost, rows, cols, rows_ghost, cols_ghost);
        compute_ghost_cols(grid_ghost, rows_ghost, cols_ghost);                                
      }else if (grid_ghost[i*cols_ghost+j]==DEAD && count==3){           
        grid_ghost[i*cols_ghost+j]=ALIVE;
        compute_ghost_rows(grid_ghost, rows, cols, rows_ghost, cols_ghost);
        compute_ghost_cols(grid_ghost, rows_ghost, cols_ghost);                              
      }else{
        grid_ghost[i*cols_ghost+j]=DEAD;
        compute_ghost_rows(grid_ghost, rows, cols, rows_ghost, cols_ghost);
        compute_ghost_cols(grid_ghost, rows_ghost, cols_ghost);                             
      }
    }
  }           
}    













          
                     
                     
                     

