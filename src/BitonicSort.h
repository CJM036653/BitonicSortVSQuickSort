/* Dichiarazioni relative al BitonicSort. */
#ifndef BITONIC_SORT_H
#define BITONIC_SORT_H

#include <stdbool.h>

#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif

/*Compare two values and swap base on direction:
true -> swap if a > b
false -> swap if a <= b*/
void compare(int *a, int *b, bool dir);

/*Sort a sequence in ascending order if dir = 1 or descending if dir = 0*/
void bitonicMerge(int a[], int low, int dim, bool dir);

void bitonicSort(int a[], int low, int dim, bool dir);

void sort(int a[], int dim);

void mergeAndSplit(int in[], int rank, int r_min, int r_max, int num_keys);


#endif
