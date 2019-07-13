/* Dichiarazioni relative al BitonicSort. */
#ifndef BITONIC_SORT_H
#define BITONIC_SORT_H

#include <stdbool.h>

#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif

/* Swaps elements in position i and j of array a */
void swap(int a[], int i, int j);

/* Modified binary search: returns the index of the nearest lower value
if the value is not found, otherwise it returns the index as usual */
int binSearch(int value, int a[], int dim);

/* Merge and Split operation as described in the paper */
void mergeAndSplit(int in[], int rank, int r_min, int r_max, int num_keys);

int* bitonicSortManager(int array[], int arraySize, int rank, int size);


#endif
