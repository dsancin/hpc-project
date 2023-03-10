SRC_DIR=./src

all:main.x

main.x: $(SRC_DIR)/main.c $(SRC_DIR)/evolution.c $(SRC_DIR)/read_write_pgm_image.c 
	mpicc -fopenmp $(SRC_DIR)/main.c $(SRC_DIR)/evolution.c $(SRC_DIR)/read_write_pgm_image.c -o main.x

clean: 
	rm -f main.x
