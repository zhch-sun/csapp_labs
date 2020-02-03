/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

#define min(a, b) (((a) > (b)) ? (b) : (a))

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i, ii, j, jj;
    // int i_size = 4, j_size = 4;
    int t0, t1, t2, t3;
    int t4, t5, t6, t7;
    for (ii = 0; ii < N; ii += 8) {
        for(jj = 0; jj < M; jj += 8) {
            for (i = ii; i < min(ii + 8, N); ++i) {
                j = jj;
                t0 = A[i][j  ], t1 = A[i][j+1];
                t2 = A[i][j+2], t3 = A[i][j+3];
                t4 = A[i][j+4], t5 = A[i][j+5];
                t6 = A[i][j+6], t7 = A[i][j+7];
                B[j  ][i] = t0, B[j+1][i] = t1;
                B[j+2][i] = t2, B[j+3][i] = t3;
                B[j+4][i] = t4, B[j+5][i] = t5;
                B[j+6][i] = t6, B[j+7][i] = t7;
                // for (j = jj; j < min(jj + 4, M); ++j) {
                //     B[j][i] = A[i][j];
                // }
            }
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 
char trans1_desc[] = "transpose trial1! ";
void trans1(int M, int N, int A[N][M], int B[M][N])
{
    int i, ii, j, jj;
    // int i_size = 4, j_size = 4;
    int t0, t1, t2, t3;
    int t4, t5, t6, t7;
    for (ii = 0; ii < N; ii += 8) {
        for(jj = 0; jj < M; jj += 8) {
            for (i = ii; i < min(ii + 8, N); ++i) {
                j = jj;
                t0 = A[i][j  ], t1 = A[i][j+1];
                t2 = A[i][j+2], t3 = A[i][j+3];
                t4 = A[i][j+4], t5 = A[i][j+5];
                t6 = A[i][j+6], t7 = A[i][j+7];
                B[j  ][i] = t0, B[j+1][i] = t1;
                B[j+2][i] = t2, B[j+3][i] = t3;
                B[j+4][i] = t4, B[j+5][i] = t5;
                B[j+6][i] = t6, B[j+7][i] = t7;
            }
        }
    }
}

char trans2_desc[] = "transpose trial2! ";
void trans2(int M, int N, int A[N][M], int B[M][N])
{
    int i, ii, j, jj;
    int i_size = 14, j_size = 6;
    for (ii = 0; ii < N; ii += i_size) {
        for(jj = 0; jj < M; jj += j_size) {
            for (i = ii; i < min(ii + i_size, N); ++i) {
                for (j = jj; j < min(jj + j_size, M); ++j) {
                    B[j][i] = A[i][j];
                }
            }
        }
    }
}

char trans3_desc[] = "transpose trial3! ";
void trans3(int M, int N, int A[N][M], int B[M][N])
{
    int i, ii, j, jj;
    int i_size = 13, j_size = 8;
    for (ii = 0; ii < N; ii += i_size) {
        for(jj = 0; jj < M; jj += j_size) {
            for (i = ii; i < min(ii + i_size, N); ++i) {
                for (j = jj; j < min(jj + j_size, M); ++j) {
                    B[j][i] = A[i][j];
                }
            }
        }
    }
}

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}


/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    // registerTransFunction(trans, trans_desc); 
    // registerTransFunction(trans1, trans1_desc); 
    // registerTransFunction(trans2, trans2_desc); 
    // registerTransFunction(trans3, trans3_desc);     
}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

