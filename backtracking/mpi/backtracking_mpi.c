#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <mpi.h>
#include <unistd.h>


#define MASTER_TASK 0

int FIND_ANS = -1;
int TASK_ID;
MPI_Request ANS_REQUEST;
struct timeval START_TIME;

int n_queen_with_1_col( int * chess_board, int n_queens );
int n_queen_backtrack( int current_column, int * chess_board, const int n_queens, int * my, int * md1, int * md2 );
void end_process ( int * chess_board, int * my, int * md1, int * md2, int n_queens );
  
int main( int argc, char * argv[] ) 
{ 
    gettimeofday(&START_TIME, NULL);
    
    int i, rc;
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
        MPI_Abort(MPI_COMM_WORLD, rc);
        return 1;
    }
    
    n_queens = atoi( argv[1] );
    
    if ( n_queens < 4 )
    {
        printf("There is no answer if number of queens is less than 4.\n");
        MPI_Abort(MPI_COMM_WORLD, rc);
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
        
        end_process( board, NULL, NULL, NULL, n_queens );
    }
    else
    {
        end_process( board, NULL, NULL, NULL, n_queens );
    }
    
    return 0; 
}

int n_queen_with_1_col( int * chess_board, int n_queens )
{
    int ans;
    
    int * my;
    int * md1;
    int * md2;
    
    my = ( int * ) malloc ( sizeof( int ) * n_queens );
    md1 = ( int * ) malloc ( sizeof( int ) * n_queens * 2 );
    md2 = ( int * ) malloc ( sizeof( int ) * n_queens * 2 );
    
    memset( my, 0, sizeof( int ) * n_queens );
    memset( md1, 0, sizeof( int ) * n_queens * 2 );
    memset( md2, 0, sizeof( int ) * n_queens * 2 );
    
    my[ chess_board[0] ] = 1;
    md1[ chess_board[0] ] = 1;
    md2[ ( 0 - chess_board[0] + n_queens - 1 ) % (n_queens * 2 - 1) ] = 1;
    
    ans = n_queen_backtrack ( 1, chess_board, n_queens, my, md1, md2 );
    
    free( my );
    free( md1 );
    free( md2 );
    
    return ans;
}

int n_queen_backtrack( int current_column, int * chess_board, const int n_queens, int * my, int * md1, int * md2 )
{
    int i;
    int d1, d2;
    int ans;
    
    int ans_status = 0;;
    
    MPI_Test( &ANS_REQUEST, &ans_status, MPI_STATUS_IGNORE);
    
    // find ans in somewhere
    if ( ans_status )
    {
        MPI_Wait(&ANS_REQUEST, MPI_STATUS_IGNORE);
        end_process( chess_board, my, md1, md2, n_queens );
    }
    
    if ( current_column == n_queens )
    {
        return 1;
    }
    
    for ( i = 0; i < n_queens; i++ )
    {
        d1 = ( current_column + i ) % ( n_queens * 2 - 1 );
        d2 = ( current_column - i + ( n_queens - 1) ) % ( n_queens * 2 - 1 );
        
        if ( !my[i] && !md1[d1] && !md2[d2] )
        {
            my[i] = md1[d1] = md2[d2] = 1;
            chess_board[current_column] = i;
            
            ans = n_queen_backtrack ( current_column + 1, chess_board, n_queens, my, md1, md2 );
            
            if ( ans )
                return ans;
 
            my[i] = md1[d1] = md2[d2] = 0;
            chess_board[current_column] = -1;
        }
    }
    
    return 0;
}

void end_process ( int * chess_board, int * my, int * md1, int * md2, int n_queens )
{
    int i, j;
    struct timeval end_time;
    
    if ( my ) free( my );
    if ( md1 ) free( md1 );
    if ( md2 ) free( md2 );
    
    // not found ans yet
    if ( FIND_ANS == TASK_ID )
    {
        for ( i = 0; i < n_queens; i++ )
        {
            for ( j = 0; j < n_queens; j++ )
            {
                if ( chess_board[j] == i )
                    printf("Q ");
                else
                    printf(". ");
                
            }
            printf("\n");
        }
                
        gettimeofday(&end_time, NULL);
        printf("Time Spend on Node %d: %ld(us)\n", TASK_ID, ( end_time.tv_sec - START_TIME.tv_sec ) * 1000000 + end_time.tv_usec - START_TIME.tv_usec );
    }
    
    free( chess_board );
    
    MPI_Finalize();
    
    exit(0);
}