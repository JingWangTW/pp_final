/home/PP-f19/MPI/bin/mpicc -c main.c -o main.o
nvcc -arch sm_30 -c kernel.cu -o kernel.o
/home/PP-f19/MPI/bin/mpicc main.o kernel.o -lcudart -o n_queen -lstdc++
rm *.o
