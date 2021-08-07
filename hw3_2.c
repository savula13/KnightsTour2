#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <math.h>


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

    int o_ids [10];
    int threads = 0;
    int t_squares = 0;
    
    #ifndef NO_PARALLEL
        //pthread_t * thread_ids = calloc(1, sizeof(pthread_t));
        pthread_t thread_ids [10];
        //int threads = 0;
    #endif
    

    //printf("Thread %i\n ",id);
    //print_board(m, n, board);


    int valid_moves [16] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
    /*
    int co_moves [16] = {c_r - 1, c_c + 2, c_r + 1, c_c + 2, c_r + 2, c_c -1 ,c_r + 2, c_c + 1,
    c_r -1, c_c - 2, c_r + 1, c_c - 2, c_r - 2, c_c -1 ,c_r - 2, c_c + 1};
    */
    int co_moves [16] = {c_r + 1, c_c + 2, c_r - 1, c_c + 2, c_r - 2, c_c + 1 ,c_r - 2, c_c - 1,
    c_r -1, c_c - 2, c_r + 1, c_c - 2, c_r + 2, c_c -1 ,c_r + 2, c_c + 1};
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
                //printf("THREAD %i: Sonny found a full knight's tour!\n", id);
                if(id == 0){
                    printf("MAIN: Sonny found a full knight's tour!\n");
                } else {
                    printf("THREAD %i: Sonny found a full knight's tour!\n", id);
                }
                tours += 1;
            } else if (squares < m*n){
                //printf("THREAD %i: Dead end at move #%i\n", id, squares);
                if(id == 0){
                    printf("MAIN: Dead end at move #%i\n", squares);
                } else {
                    printf("THREAD %i: Dead end at move #%i\n", id, squares);
                }
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
        if(id == 0){
            printf("MAIN: %i possible moves after move #1; creating %i child threads...\n",moves, moves);
        }else{
            printf("THREAD %i: %i possible moves after move #%i; creating %i child threads...\n", id, moves, squares, moves);
        }
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
            /*    
            if(id != 0){
                pthread_mutex_lock(&mutex_on_next_thread_id);
                {
                    next_thread_id += 1;
                }
            }
            */
            pthread_mutex_lock(&mutex_on_next_thread_id);
            {
                next_thread_id += 1;
            }
            pthread_mutex_unlock(&mutex_on_next_thread_id);
            init_args = create_args(c_board, valid_moves[j-2], valid_moves[j-1], m, n, squares + 1, x, next_thread_id);
            int rc;
            /*
            #ifndef NO_PARALLEL
                threads += 1;
            #endif
            */
            threads += 1;
            rc = pthread_create(&pid, NULL, find_moves, (void *) init_args);
            //thread_ids[threads-1] = pid;
            if ( rc != 0 ) { fprintf( stderr, "pthread_create() failed (%d)\n", rc );}
            o_ids[threads-1] = next_thread_id;
            //printf("before pid %li, thread %i\n", pid, id);
            #ifdef NO_PARALLEL
                int * retval;
                pthread_join(pid, (void **)&retval);
                if(id == 0){
                    //printf("MAIN: Thread %i joined (returned %i)\n",retval[0], retval[1]);
                    printf("MAIN: Thread %i joined (returned %i)\n",o_ids[threads-1], retval[1]);
                }else{
                    //printf("THREAD %i: Thread %i joined (returned %i)\n", id, , retval[1]);
                    printf("THREAD %i: Thread %i joined (returned %i)\n", id, o_ids[threads-1], retval[1]);
                }
                if(retval[1] > squares && retval[1] > t_squares){
                    t_squares = retval[1];
                }
                free(retval);
                if ( rc != 0 ) { fprintf( stderr, "pthread_join() failed (%d)\n", rc );}
            #endif
            #ifndef NO_PARALLEL
                thread_ids[threads-1] = pid;
                //printf("after pid %li, thread %i\n", pid, id);
                //int t_size = sizeof(thread_ids)/ sizeof(pthread_t);
                //thread_ids = realloc(thread_ids, (t_size + 3) * (sizeof(pthread_t)));
                //printf("size %i, thread: %i\n",t_size, id);
            #endif
            /*
            if(id == 0){
                next_thread_id += 1;
            }
            */
        } 
    }
    #ifndef NO_PARALLEL
        //int t_size = sizeof(thread_ids)/ sizeof(pthread_t);
        //printf("size %i, thread: %i\n",t_size, id);
        if(threads > 1){
            for(int i = 0; i < threads; i++){
                int rc;
                int * retval;
                rc = pthread_join(thread_ids[i], (void **)&retval);
                if ( rc != 0 ) { fprintf( stderr, "pthread_join() failed (%d)\n", rc );}
                if(id == 0){
                    printf("MAIN: Thread %i joined (returned %i)\n",o_ids[i], retval[1]);
                }else{
                    printf("THREAD %i: Thread %i joined (returned %i)\n", id, o_ids[i], retval[1]);
                }
                if(retval[1] > squares && retval[1] > t_squares){
                    t_squares = retval[1];
                }
                free(retval);
            }
        }
        //free(thread_ids);
    #endif
    for ( int i = 0 ; i < m ; i++ ) free( board[i] );
    free(board); 
    if(t_squares == 0){
        t_squares = squares;
    }
    if(id != 0){
        int * ret = calloc(2,sizeof(int));
        ret[0] = id;
        ret[1] = t_squares;
        //printf("id:, %i ret: %i\n", id, ret[0]);
        pthread_exit(ret);
    }
    return NULL;
}

int simulate( int argc, char * argv[] ){
    int m = atoi( argv[1] );
    int n = atoi( argv[2] );
    if(m <= 2 || n <= 2){
        fprintf(stderr, "ERROR: Invalid argument(s)\n");
        fprintf(stderr, "USAGE: a.out <m> <n> <r> <c> <x>\n");
        return EXIT_FAILURE;
    }

    int r = atoi(argv[3]);
    int c = atoi(argv[4]);
    int x = atoi(argv[5]);

    if(r >= m || r < 0){
        fprintf(stderr, "ERROR: Invalid argument(s)\n");
        fprintf(stderr, "USAGE: a.out <m> <n> <r> <c> <x>\n");
        return EXIT_FAILURE;
    }

    if(c >= n || c < 0){
        fprintf(stderr, "ERROR: Invalid argument(s)\n");
        fprintf(stderr, "USAGE: a.out <m> <n> <r> <c> <x>\n");
        return EXIT_FAILURE;
    }
    if(x > m*n){
        fprintf(stderr, "ERROR: Invalid argument(s)\n");
        fprintf(stderr, "USAGE: a.out <m> <n> <r> <c> <x>\n");
        return EXIT_FAILURE;
    }
  
    char ** board = create_board(m,n);
    //#ifndef NO_PARALLEL
     //   pthread_t * thread_ids = calloc(1, sizeof(pthread_t));
    //#endif
    board[r][c] = 'S';
    printf("MAIN: Solving Sonny's knight's tour problem for a %ix%i board\n", m, n);
    printf("MAIN: Sonny starts at row %i and column %i (move #1)\n", r, c);

    struct args *init_args;
    next_thread_id = 0;
    init_args = create_args(board, r, c, m, n, 1, x, 0);
    find_moves(init_args);

    if(max_squares == m*n){
        printf("MAIN: All threads joined; found %i possible ways to achieve a full knight's tour", tours);
    } else{
        if(max_squares == 1){
            printf("MAIN: All threads joined; best solution(s) visited %i square out of %i\n", max_squares, m*n); 
        } else{
            printf("MAIN: All threads joined; best solution(s) visited %i squares out of %i\n", max_squares, m*n); 
        }
        if(dead_boards == 1){
            if(max_squares == 1){
                printf("MAIN: Dead end board covering at least %i square:\n", x);
            } else {
                printf("MAIN: Dead end board covering at least %i squares:\n", x);
            }
        } else {
            if(max_squares == 1){
                printf("MAIN: Dead end boards covering at least %i square:\n", x);
            } else {
                printf("MAIN: Dead end boards covering at least %i squares:\n", x);
            }
        }

        for(int i = 0; i < dead_boards; i++){
            char * prefix = "MAIN:";
            print_board(m, n, dead_end_boards[i], prefix);
        }
    }


    next_thread_id += 1;
    return dead_boards;
    //return EXIT_SUCCESS;

}
/*
int main( int argc, char * argv[] ){
    simulate(argc, argv);
}
*/
