#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <signal.h>
#include <pthread.h>
#include <mpi.h>

#define MASTER_TASK 0

int FIND_ANS = -1;
int TASK_ID;
MPI_Request ANS_REQUEST;
struct timeval START_TIME;

int n_queen_with_1_col( int * chess_board, int n_queens );
void * n_queen_with_2_col( void * argv );
void end_process ( int * chess_board, int n_queens );
void print_board( int * chess_board, int n_queens );

// define in other file
int find_ans_in_gpu ( int * board, int n_queens, int * global_thread_find_ans );

int main( int argc, char * argv[] ) 
{ 
    gettimeofday(&START_TIME, NULL);
    
    int i;
    int num_tasks;    
    int n_queens;
    int local_start, local_end;
    int count_iter;
    int * board;
    int ans;
   
    MPI_Request send_request;
    
    MPI_Init (&argc, &argv);
    
    MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &TASK_ID);
    
    // pre post recv
    MPI_Irecv( &FIND_ANS, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &ANS_REQUEST );
    
    if ( argc != 2 )
    {
        printf("Pass Number of Queen in first argument.\n");
        MPI_Abort(MPI_COMM_WORLD, 0);
        return 1;
    }
    
    n_queens = atoi( argv[1] );
    
    if ( n_queens < 4 )
    {
        printf("There is no answer if number of queens is less than 4.\n");
        MPI_Abort(MPI_COMM_WORLD, 0);
        return 1;
    }
    
    board = ( int * ) malloc ( sizeof( int ) * n_queens );
    memset ( board, -1, sizeof(int) * n_queens );
    
    local_start = n_queens / num_tasks * TASK_ID;
    local_end = n_queens / num_tasks * (TASK_ID + 1);
    
    if (local_end > n_queens)
        local_end = n_queens;
    
    for ( count_iter = local_start; count_iter < local_end; count_iter ++ ) 
    {
        board[0] = count_iter;
        ans = n_queen_with_1_col ( board, n_queens );

        if ( ans )
            break;
    }
    
    // if find ans
    if ( ans && FIND_ANS == -1 )
    {
        MPI_Cancel ( &ANS_REQUEST );
        FIND_ANS = TASK_ID;
        
        for ( i = 0; i < num_tasks; i ++ )
        {
            if ( i != TASK_ID )
                MPI_Isend( &FIND_ANS, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &send_request );
        }
        
        end_process( board, n_queens );
    }
    else
    {
        end_process( board, n_queens );
    }
    
    return 0; 
}

int n_queen_with_1_col( int * chess_board, int n_queens )
{
    int ans = -1;
    int *** argv;
    int ** board;
    int global_ans_status;
    
    int number_of_cpu;
    int thread_count;
    pthread_t * thread_pool;
    
    number_of_cpu = get_nprocs();
    
    thread_pool = (pthread_t *) malloc (sizeof(pthread_t) * number_of_cpu);
    
    board = ( int ** )malloc( sizeof( int * ) * number_of_cpu);
    argv = ( int *** )malloc( sizeof( int ** ) * number_of_cpu );
    
    for( thread_count = 0; thread_count < number_of_cpu; thread_count++ )
    {
        board[thread_count] = ( int * )malloc( sizeof( int ) * n_queens );
        argv[thread_count] = ( int ** )malloc( sizeof( int * ) * 7 );
        
        memcpy( board[thread_count], chess_board, sizeof( int ) * n_queens );
        
        argv[thread_count][0] = board[thread_count];
        argv[thread_count][1] = &n_queens;
        argv[thread_count][2] = ( int * ) malloc ( sizeof( int ) );
        argv[thread_count][3] = ( int * ) malloc ( sizeof( int ) );
        argv[thread_count][4] = ( int * ) malloc ( sizeof( int ) );
        argv[thread_count][5] = &ans;
        argv[thread_count][6] = ( int * ) thread_pool;
        
        *(argv[thread_count][2])  = n_queens / number_of_cpu * thread_count;        // start
        *(argv[thread_count][3])  = n_queens / number_of_cpu * (thread_count + 1);  // end
        *(argv[thread_count][4])  = thread_count;  // rank
        
        if ( *(argv[thread_count][3]) > n_queens )
            *(argv[thread_count][3]) = n_queens;
    
        pthread_create( &thread_pool[thread_count], NULL, n_queen_with_2_col, (void *)argv[thread_count]);
    }
    
    for( thread_count = 0; thread_count < number_of_cpu; thread_count++ )
    {
        pthread_join(thread_pool[thread_count], NULL);
    }
    
    // if find ans in current node
    if ( ans != -1 )
    {
        memcpy( chess_board, board[ans], sizeof( int ) * n_queens );
        
        for( thread_count = 0; thread_count < number_of_cpu; thread_count++ )
        {
            free ( board[thread_count] );
            free ( argv[thread_count][2] );
            free ( argv[thread_count][3] );
            free ( argv[thread_count] );
        }
        
        free ( argv );
        free ( board );
        free ( thread_pool );
        
        return 1;
    }
    else 
    {
        // check if find ans in other nodes
        MPI_Test( &ANS_REQUEST, &global_ans_status, MPI_STATUS_IGNORE);
        
        // if find ans in other nodes
        if ( global_ans_status )
        {
            MPI_Wait(&ANS_REQUEST, MPI_STATUS_IGNORE);
            
            for( thread_count = 0; thread_count < number_of_cpu; thread_count++ )
            {
                free ( board[thread_count] );
                free ( argv[thread_count][2] );
                free ( argv[thread_count][3] );
                free ( argv[thread_count] );
            }
            
            free ( argv );
            free ( board );
            free ( thread_pool );
            
            end_process( chess_board, n_queens );
        }
        
        // not found ans yet
        return 0;
    }
}

void * n_queen_with_2_col( void * argv )
{
    int * my;
    int * md1;
    int * md2;
    int * chess_board;
    int * ans;
    pthread_t   * thread_pool;
    int local_ans;
    int local_start;
    int local_end;
    int n_queens;
    int self_thread_rank;
    int count_iter;
    int number_of_cpu;
    int thread_count;
    int d1, d2;
    
    chess_board         = ((int **)argv)[0];
    n_queens            = *(((int **)argv)[1]);
    local_start         = *(((int **)argv)[2]);
    local_end           = *(((int **)argv)[3]);
    self_thread_rank    = *(((int **)argv)[4]);
    ans                 = ((int **)argv)[5];
    thread_pool         = (pthread_t *)(((int **)argv)[6]);
    number_of_cpu       = get_nprocs();
    
    my = ( int * ) malloc ( sizeof( int ) * n_queens );
    md1 = ( int * ) malloc ( sizeof( int ) * n_queens * 2 );
    md2 = ( int * ) malloc ( sizeof( int ) * n_queens * 2 );
    
    memset( my, 0, sizeof( int ) * n_queens );
    memset( md1, 0, sizeof( int ) * n_queens * 2 );
    memset( md2, 0, sizeof( int ) * n_queens * 2 );
    
    my[ chess_board[0] ] = 1;
    md1[ chess_board[0] ] = 1;
    md2[ ( 0 - chess_board[0] + n_queens - 1 ) % (n_queens * 2 - 1) ] = 1;
        
    for ( count_iter = local_start; count_iter < local_end && (*ans) == -1; count_iter ++  )
    {
        d1 = ( 1 + count_iter ) % ( n_queens * 2 - 1 );
        d2 = ( 1 - count_iter + ( n_queens - 1) ) % ( n_queens * 2 - 1 );
        
        if ( !my[count_iter] && !md1[d1] && !md2[d2] )
        {
            my[count_iter] = md1[d1] = md2[d2] = 1;
            chess_board[1] = count_iter;
            
            local_ans = find_ans_in_gpu ( chess_board, n_queens, ans );
            
            if ( local_ans )
            {
                if ( *ans  == -1 )
                {
                    print_board ( chess_board, n_queens );
                    
                    *ans = self_thread_rank;
                    
                    for ( thread_count = 0; thread_count < number_of_cpu; thread_count ++ )
                    {
                        if ( thread_count != self_thread_rank )
                        {
                            pthread_cancel( thread_pool[ thread_count ] );    
                        }
                    }
                }

                break;
            }
 
            my[count_iter] = md1[d1] = md2[d2] = 0;
            chess_board[1] = -1;
        }
    }
    
    free(my);
    free(md1);
    free(md2);
    
    return NULL;
}

void end_process ( int * chess_board, int n_queens )
{    
    // not found ans yet
    if ( FIND_ANS == TASK_ID )
    {
        print_board( chess_board, n_queens );
    }
    
    free( chess_board );
    MPI_Finalize();
    
    exit(0);
}

void print_board( int * chess_board, int n_queens )
{
    int i ,j;
    struct timeval end_time;
    
    for ( i = 0; i < n_queens; i++ )
    {
        for ( j = 0; j < n_queens; j++ )
        {
            if ( chess_board[j] == i )
                printf( "Q " );
            else
                printf( ". " );
            
        }
        printf( "\n" );
    }
    
    gettimeofday(&end_time, NULL);
    printf("Time Spend on Node %d: %ld(us)\n", TASK_ID, ( end_time.tv_sec - START_TIME.tv_sec ) * 1000000 + end_time.tv_usec - START_TIME.tv_usec );
}