#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

extern int next_thread_id;
extern int max_squares;
extern char *** dead_end_boards;
int tours = 0;
int dead_boards = 0;
pthread_mutex_t mutex_on_next_thread_id = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_on_max_squares = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_on_dead_end_boards = PTHREAD_MUTEX_INITIALIZER;

struct args {
    char** board;
    int c_r;
    int c_c;
    int m;
    int n;
    int squares;
    int x;
    int id;
};

char ** create_board(int m, int n){
    char ** board = calloc(m, sizeof(char*));
    for(int i = 0; i < m; i++){
        board[i] = calloc(n, sizeof(char));
        for(int j = 0; j < n; j ++){
            board[i][j] = '.';
        }
    }
    return board;
}

char ** copy_board(int m, int n, char **board){
    char ** c_board = calloc(m, sizeof(char*));
        for(int i = 0; i < m; i++){
        c_board[i] = calloc(n, sizeof(char));
        for(int j = 0; j < n; j ++){
            c_board[i][j] = board[i][j];
        }
    }
    return c_board;
}

struct args * create_args(char ** board, int c_r, int c_c, int m, int n, int squares, int x, int id){
            struct args *init_args = calloc(1,sizeof(struct args));
            init_args -> board = board;
            init_args -> c_c = c_c;
            init_args -> c_r = c_r;
            init_args -> m = m;
            init_args -> n = n;
            init_args -> x = x;
            init_args -> squares = squares;
            init_args -> id = id;
            init_args = (void *)init_args;
            return init_args;
}

void print_board(int m,int n, char ** board, char * prefix){
    for(int i = 0; i < m; i++){
        printf("%s ", prefix);
        if(i == 0){
            printf(">>");
        } else {
            printf("  ");
        }
        for(int j = 0; j < n; j++){
            printf("%c", board[i][j]);
        }
        if(i == m-1){
            printf("<<");
        }
        printf("\n");
    }
}

void * find_moves(void * args){
    char ** board = ((struct args*)args)-> board;
    int c_r = ((struct args*)args)-> c_r;
    int c_c = ((struct args*)args)-> c_c;
    int m = ((struct args*)args)-> m;
    int n = ((struct args*)args)-> n;
    int squares = ((struct args*)args)-> squares;
    int x = ((struct args*)args)-> x;
    int id = ((struct args*)args)-> id;
    free(args);

    //printf("Thread %i\n ",id);
    //print_board(m, n, board);


    int valid_moves [16] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
    int co_moves [16] = {c_r - 1, c_c + 2, c_r + 1, c_c + 2, c_r + 2, c_c -1 ,c_r + 2, c_c + 1,
    c_r -1, c_c - 2, c_r + 1, c_c - 2, c_r - 2, c_c -1 ,c_r - 2, c_c + 1};
    int moves = 0;
    for(int i = 0; i < 16; i += 2){
        if(co_moves[i] < m && co_moves[i]  >= 0 && co_moves[i+1] < n && co_moves[i+1] >= 0){
            if(board[co_moves[i]][co_moves[i+1]] == '.'){
                moves += 1;
                //printf("r: %i, c: %i \n", co_moves[i], co_moves[i+1]);
                valid_moves[i] = co_moves[i];
                valid_moves[i+1] = co_moves[i+1];
            }
        }
    }
    //squares += moves;
    //printf("moves: %i\n", moves);
    //printf("squares: %i\n", squares);
    if(moves == 1 || moves == 0){
        int i = 0;
        for(; i < 16; i+=2){
            if(valid_moves[i] != -1){
                board[valid_moves[i]][valid_moves[i+1]] = 'S';
                break;
            }
        }
        if(moves == 1){
            //continue if only 1 move
            //free(args);
            struct args * a = create_args(board, valid_moves[i], valid_moves[i+1], m, n, squares + 1, x, id);
            find_moves((void *) a);
        } else {
            if(squares == m*n){
                printf("THREAD %i: Sonny found a full knight's tour!\n", id);
                tours += 1;
            } else if (squares < m*n){
                printf("THREAD %i: Dead end at move #%i\n", id, squares);
                if(squares >= x){
                    //might take too long
                    dead_boards += 1;
                    //dead_end_boards = realloc(dead_end_boards, (sizeof(char) * m * n));
                    if(sizeof(dead_end_boards)/(sizeof(char) * m * n) <= dead_boards){
                        long new_size = dead_boards * (sizeof(char) * m * n);
                        //printf("s: %li\n", new_size);
                        dead_end_boards = realloc(dead_end_boards, new_size);
                        //printf("size new : %li\n", sizeof(dead_end_boards));
                    }
                    dead_end_boards[dead_boards-1] = copy_board(m, n, board);
                    //print_board(m, n, dead_end_boards[dead_boards-1]);
                    // add to dead boards
                }
            }
            if(squares > max_squares){
                pthread_mutex_lock(&mutex_on_max_squares);
                {
                    max_squares = squares;
                }
                pthread_mutex_unlock(&mutex_on_max_squares);
            }
            //free(args); 
        }
    } else if (moves > 1){
        printf("THREAD %i: %i possible moves after move #%i; creating %i child threads...\n", id, moves, squares, moves);
        //int rc;
        struct args *init_args;
        //change c_board based on move  
        int j = 0;
        for(int i = 0; i < moves; i++){
            pthread_t pid;
            char ** c_board = copy_board(m, n, board); 
            //might take too long
            while(j < 15){
                if(valid_moves[j] != -1){
                    c_board[valid_moves[j]][valid_moves[j+1]] = 'S';
                    j += 2;
                    break;
                }
                j+=2;
            }      
            pthread_mutex_lock(&mutex_on_next_thread_id);
            {
                next_thread_id += 1;
                //printf("nti %i\n",next_thread_id);
            }
            pthread_mutex_unlock(&mutex_on_next_thread_id);
            //free(args);
            init_args = create_args(c_board, valid_moves[j-2], valid_moves[j-1], m, n, squares + 1, x, next_thread_id);
            pthread_create(&pid, NULL, find_moves, (void *) init_args);
            //if ( rc != 0 ) { fprintf( stderr, "pthread_create() failed (%d)\n", rc ); return EXIT_FAILURE; }
            #ifdef NO_PARALLEL
                int * retval;
                pthread_join(pid, (void **)&retval);
                printf("MAIN: Thread %i joined (returned %i)\n", retval[0], retval[1]);
                free(retval);
                //if ( rc != 0 ) { fprintf( stderr, "pthread_join() failed (%d)\n", rc ); return EXIT_FAILURE; }
            #endif
        } 
        /*
        for ( int i = 0 ; i < m ; i++ ) free( board[i] );
        free(board);  
        */
    }
    //int ret [2] = {id, squares};
    int * ret = calloc(2,sizeof(int));
    for ( int i = 0 ; i < m ; i++ ) free( board[i] );
    free(board); 
    ret[0] = id;
    ret[1] = squares;
    pthread_exit(ret);
    return NULL;
}

int simulate( int argc, char * argv[] ){
    int m = atoi( argv[1] );
    int n = atoi( argv[2] );
    if(m <= 2 || n <= 2){
        fprintf(stderr, "Invalid board size argument: \n");
        return EXIT_FAILURE;
    }

    int r = atoi(argv[3]);
    int c = atoi(argv[4]);
    int x = atoi(argv[5]);

    if(r >= m || r < 0){
        fprintf(stderr, "Invalid starting row: %i \n", r);
        return EXIT_FAILURE;
    }

    if(c >= n || c < 0){
        fprintf(stderr, "Invalid starting column: %i \n", r);
        return EXIT_FAILURE;
    }
  
    char ** board = create_board(m,n);
    #ifndef NO_PARALLEL
        pthread_t * ids = calloc(1, sizeof(pthread_t));
    #endif
    board[r][c] = 'S';
    printf("MAIN: Solving Sonny's knight's tour problem for a %ix%i board\n", m, n);
    printf("MAIN: Sonny starts at row %i and column %i (move #1)\n", r, c);
    struct args *init_args;

    int valid_moves [16] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
    int co_moves [16] = {r -1, c + 2, r + 1, c + 2, r + 2, c -1 ,r + 2, c + 1,
    r -1, c - 2, r + 1, c - 2, r - 2, c -1 ,r - 2, c + 1};
    int moves = 0;
    for(int i = 0; i < 16; i += 2){
        if(co_moves[i] < m && co_moves[i]  > 0 && co_moves[i+1] < n && co_moves[i+1] > 0){
            if(board[co_moves[i]][co_moves[i+1]] == '.'){
                moves += 1;
                valid_moves[i] = co_moves[i];
                valid_moves[i+1] = co_moves[i+1];
            }
        }
    }
    if(moves == 1 || moves == 0){
        printf("uhhhh");
    } else if (moves > 1){
        int rc;
        print_board(m, n, board, "MAIN:");
        printf("MAIN: %i possible moves after move #1; creating %i child threads...\n", moves, moves);
        //change c_board based on move  
        int j = 0;
        for(int i = 0; i < moves; i++){
            pthread_t pid;
            char ** c_board = copy_board(m, n, board); 
            //might take too long
            while(j < 15){
                if(valid_moves[j] != -1){
                    c_board[valid_moves[j]][valid_moves[j+1]] = 'S';
                    j += 2;
                    break;
                }
                j+=2;
            }     
            init_args = create_args(c_board, valid_moves[j-2], valid_moves[j-1], m, n, 2, x, next_thread_id);
            rc = pthread_create(&pid, NULL, find_moves, (void *) init_args);
            if ( rc != 0 ) { fprintf( stderr, "pthread_create() failed (%d)\n", rc ); return EXIT_FAILURE; }

            #ifdef NO_PARALLEL
                int * retval;
                rc = pthread_join(pid, (void **)&retval);
                printf("MAIN: Thread %i joined (returned %i)\n", retval[0], retval[1]);
                free(retval);
                if ( rc != 0 ) { fprintf( stderr, "pthread_join() failed (%d)\n", rc ); return EXIT_FAILURE; }
            #endif
            next_thread_id += 1;
            #ifndef NO_PARALLEL
               ids = realloc(ids, (next_thread_id+1) * (sizeof(pthread_t)));
               ids[next_thread_id-2] = pid;
            #endif

        }
    }
    #ifndef NO_PARALLEL
        for(int i = 0; i < next_thread_id-1; i++){
            int rc;
            int * retval;
            rc = pthread_join(ids[i], (void **)&retval);
            printf("MAIN: Thread %i joined (returned %i)\n", i+1, retval[1]);
            free(retval);
            if ( rc != 0 ) { fprintf( stderr, "pthread_join() failed (%d)\n", rc ); return EXIT_FAILURE; }
        }
        free(ids);
    #endif
    
    for ( int i = 0 ; i < m ; i++ ) free( board[i] );
    free(board);  

    printf("MAIN: All threads joined; best solution(s) visited %i squares out of %i\n", max_squares, m*n);  

    printf("MAIN: Dead end boards covering at least %i squares:\n", x);

    for(int i = 0; i < dead_boards; i++){
        char * prefix = "MAIN";
        print_board(m, n, dead_end_boards[i], prefix);
    }

    return dead_boards;
    //return EXIT_SUCCESS;

}
/*
int main( int argc, char * argv[] ){
    simulate(argc, argv);
}
*/
