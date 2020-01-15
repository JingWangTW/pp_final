#include <cuda.h>
#include <cuda_runtime.h>

#include <stdio.h>

__global__ 
void n_queen_in_gpu( const short n_queens, const short first, const short second, int * global_ans, int * find_ans_local )
{
    short third = blockIdx.x;
    short forth = threadIdx.x;
    
    short * my;
    short * md1;
    short * md2;
    int * local_ans;
    
    short d1, d2;
    short o_d1, o_d2, o_p;
    short current_column;
    
    // allocate memory
    my = ( short * ) malloc ( sizeof ( short ) * n_queens ); 
    md1 = ( short * ) malloc ( sizeof ( short ) * n_queens * 2 );
    md2 = ( short * ) malloc ( sizeof ( short ) * n_queens * 2 );
    local_ans = ( int * ) malloc ( sizeof ( int ) * n_queens );
    
    // set the default value
    current_column = 4;
    memset(my, 0, sizeof(short) * n_queens);
    memset(md1, 0, sizeof(short) * n_queens * 2);
    memset(md2, 0, sizeof(short) * n_queens * 2);
    memset(local_ans, -1, sizeof(int) * n_queens);
    
    // filled the status finish in cpu
    my[first] = 1;
    my[second] = 1;
    
    md1[first] = 1;
    md1[( 1 + second ) % ( n_queens * 2 - 1 )] = 1;

    md2[ ( 0 - first + ( n_queens - 1) ) % ( n_queens * 2 - 1 ) ] = 1;
    md2[ ( 1 - second + ( n_queens - 1) ) % ( n_queens * 2 - 1 ) ] = 1;
    
    local_ans[0] = first;
    local_ans[1] = second;
        
    // test for chance of third place
    d1 = ( 2 + third ) % ( n_queens * 2 - 1 );
    d2 = ( 2 - third + ( n_queens - 1) ) % ( n_queens * 2 - 1 );
    
    if ( my[third] || md1[d1] || md2[d2] )
    {
        free( my );
        free( md1 );
        free( md2 );
        return;
    }
    
    my[third] = md1[d1] = md2[d2] = 1;
    local_ans[2] = third;
    
    // test for chance of forth place
    d1 = ( 3 + forth ) % ( n_queens * 2 - 1 );
    d2 = ( 3 - forth + ( n_queens - 1) ) % ( n_queens * 2 - 1 );
    
    if ( my[forth] || md1[d1] || md2[d2] )
    {
        free( my );
        free( md1 );
        free( md2 );
        return;
    }
    
    my[forth] = md1[d1] = md2[d2] = 1;
    local_ans[3] = forth;
    
    // check if somebody yet found the ans.
    while ( !( *find_ans_local ) )
    {
        // FIND ANS!!!!!
        if ( current_column == n_queens ) 
        {
            // notify to all gpu thread that I found the ANS!!!!!
            *find_ans_local = 1;
            
            memcpy( global_ans, local_ans, sizeof(int) * n_queens );
        }
        
        // first come into this column
        // there is no valid o_d1 and o_d2
        if ( local_ans[current_column] == -1 )
        {
            o_d1 = -1;
            o_d2 = -1;
            o_p = -1;
        }
        else
        {
            o_d1 = ( current_column + local_ans[current_column] ) % ( n_queens * 2 - 1 );
            o_d2 = ( current_column - local_ans[current_column] + ( n_queens - 1) ) % ( n_queens * 2 - 1 );
            o_p = local_ans[current_column];
        }
        
        // try the next possible ans
        do
        {
            local_ans[current_column] ++;
        
            if ( local_ans[current_column] < n_queens )
            {
                d1 = ( current_column + local_ans[current_column] ) % ( n_queens * 2 - 1 );
                d2 = ( current_column - local_ans[current_column] + ( n_queens - 1) ) % ( n_queens * 2 - 1 );    
            }
            else
            {
                break;
            }
            
        } while ( ( my[ local_ans[ current_column ] ] || md1[ d1 ] || md2[ d2 ] ) );
        
        // remove the mark of old position
        if ( o_d1 != -1 && o_d2 != -1 && o_p != -1 )
            my[ o_p ] = md1[ o_d1 ] = md2[ o_d2 ] = 0;
        
        // find the possible ans
        if ( local_ans[current_column] < n_queens )
        {
            // mark the board according to current position
            my[ local_ans[ current_column ] ] = md1[ d1 ] = md2[ d2 ] = 1;
            
            // go to next column
            current_column ++ ;
        }
        else
        {   
            local_ans[ current_column ] = -1;
            
            if ( current_column == 4 )
                break;
            else
                current_column --;
        }
    }
    
    free( my ); 
    free( md1 );
    free( md2 );
}

extern "C" int find_ans_in_gpu ( int * board, int n_queens )
{
    int * global_gpu_ans;
    int * find_ans_in_local_thread;
    
    cudaMallocManaged( ( void** )&global_gpu_ans, sizeof( int ) * n_queens );
    cudaMallocManaged( ( void** )&find_ans_in_local_thread, sizeof( int ) );
    
    ( *find_ans_in_local_thread ) = 0;
    
    n_queen_in_gpu <<< n_queens, n_queens >>> ( n_queens, board[0], board[1], global_gpu_ans, find_ans_in_local_thread );
    cudaDeviceSynchronize();
    
    if ( *find_ans_in_local_thread )
    {        
        memcpy( board, global_gpu_ans, sizeof( int ) * n_queens );
        
        return 1;
    }
    else
    {
        return 0;
    }
}