# N-queen backtracking

## Serial

* content
    ```
    > serial
        > backtrakcing.c        
        > backtracking_opt.c    # optimize method
    ```
    
* Serial version to solve N-queen problem using backtracking.

* Compile: 
    ```
    gcc backtraking_opt.c -o backtracking_opt
    ```
* Run: 
    ```
    ./backtracking_opt 8    # pass the number of queen as first argument
    ```
    
## MPI

* content
    ```
    > mpi
        > backtrakcing_mpi.c    # Code          
        > hostfile              # declare the host in cluster    
    ```
    
* Solving N-queen problem in backtracking with MPI, which can distribute the computing in multiple host.

* Compile: 
    ```
    mpicc backtraking_mpi.c -o backtracking_mpi
    ```
* Run: 
    ```
    mpiexec --hostfile hostfile backtracking_mpi 8  # pass the number of queen as first argument
    ```
    
## MPI_Pthread

* content
    ```
    > mpi_pthread
        > backtrakcing_mpi_ptherad.c    # Code          
        > hostfile                      # declare the host in cluster    
    ```
    
* Solving N-queen problem in backtracking with MPI, which can distribute the computing in multiple host. In each host, it will also use Pthread to launch as many thread as each host can to assist computing.

* Compile: 
    ```
    mpicc backtraking_mpi.c -o backtracking_mpi
    ```
* Run: 
    ```
    mpiexec --hostfile hostfile backtracking_mpi 8  # pass the number of queen as first argument
    ```

## MPI_Pthread_Cuda

* content
    ```
    > mpi_pthread_cuda
        > compile.sh    # Compuile command
        > hostfile      # declare the host in cluster    
        > main.c        # Code launch in host
        > kernel.cu     # Code launch in kernel
    ```
    
* Solving N-queen problem in backtracking with MPI, which can distribute the computing in multiple host. In each host, it will also use Pthread to launch as many thread as each host can. As in each thread, it will launch CUDA code to assist computing.

* Compile: 
    ```
    ./compile
    ```
    
* Run: 
    ```
    ./n_queen 8 # pass the number of queen as first argument
    ```