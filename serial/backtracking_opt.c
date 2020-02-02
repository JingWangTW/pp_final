#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int n_queen_backtrack( int current_column, int * chess_board, const int n_queens, int * my, int * md1, int * md2 );
void print_board( int * chess_board, int n_queens );

int main( int argc, char * argv[] ) 
{ 
    int i, j;
    int num_tasks;    
    int n_queens;
    int count_iter;
    int * board;
    int * my, * md1, * md2;
    int ans;
    
    if ( argc != 2 )
    {
        printf("Pass Number of Queen in first argument.\n");
        return 1;
    }
    
    n_queens = atoi( argv[1] );
    
    if ( n_queens < 4 )
    {
        printf("There is no answer if number of queens is less than 4.\n");
        return 1;
    }
    
    board = ( int * ) malloc ( sizeof( int ) * n_queens );
    my = ( int * ) malloc ( sizeof( int ) * n_queens );
    md1 = ( int * ) malloc ( sizeof( int ) * n_queens * 2 );
    md2 = ( int * ) malloc ( sizeof( int ) * n_queens * 2 );
    
    memset( board, -1, sizeof(int) * n_queens );
    memset( my, 0, sizeof( int ) * n_queens );
    memset( md1, 0, sizeof( int ) * n_queens * 2 );
    memset( md2, 0, sizeof( int ) * n_queens * 2 );
    
    n_queen_backtrack ( 0, board, n_queens, my, md1, md2 );
    
    print_board( board, n_queens );

    free( my );
    free( md1 );
    free( md2 );
    free( board );
    
    return 0; 
}

int n_queen_backtrack( int current_column, int * chess_board, const int n_queens, int * my, int * md1, int * md2 )
{
    int i;
    int d1, d2;
    int ans;
    
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

void print_board( int * chess_board, int n_queens )
{
    int i ,j;
    
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
}