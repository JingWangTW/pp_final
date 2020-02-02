mpicc -c main.c -o main.o
nvcc -arch sm_30 -c kernel.cu -o kernel.o
mpicc main.o kernel.o -lcudart -o n_queen -lstdc++
rm *.o
