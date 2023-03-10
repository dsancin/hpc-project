#ifndef EVOLUTION_DOT_H
#define EVOLUTION_DOT_H


//function to exchange ghost rows of the local grid with upper and lower ranks
void exchange_ghost_rows(int *local_grid_ghost, int local_rows_ghost, int local_cols_ghost, int upper_rank, int lower_rank);

//function to compute ghost columns
void compute_ghost_cols(int *local_grid_ghost, int local_rows_ghost, int local_cols_ghost);

//function to compute ghost rows
void compute_ghost_rows(int *grid, int rows, int cols,  int rows_ghost, int cols_ghost);

//function to count the number of alive neighbours
int alive_neigh(int *grid, int i, int j, int cols);

//function that apply static evolution algorithm              
void static_evo(int *grid, int *grid_next, int rows, int cols);

//function that apply ordered evolution algorithm              
void ordered_evo(int *grid, int rows_ghost, int cols_ghost, int rows, int cols);


#endif /* EVOLUTION_DOT_H */
