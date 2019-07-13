#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"
#include "BitonicSort.h"

#define MASTER 0        /* Master process ID */

/* Swaps elements in position i and j of array a */
void swap(int a[], int i, int j)
{
  int t = a[i];
  a[i] = a[j];
  a[j] = t;
}

/* Modified binary search: returns the index of the nearest lower value
if the value is not found, otherwise it returns the index as usual */
int binSearch(int value, int a[], int dim)
{
  if(value < a[0])
    return 0;
  if(value > a[dim - 1])
    return dim;

  int lo = 0;
  int hi = dim - 1;

  while (lo <= hi)
  {
    int mid = (hi + lo) / 2;

    if (value < a[mid])
      hi = mid - 1;
    else if (value > a[mid])
      lo = mid + 1;
    else
      return mid;
  }
  return lo;
}

/* Merge and Split operation as described in the paper */
void mergeAndSplit(int in[], int rank, int r_min, int r_max, int num_keys)
{
  int val;
  MPI_Status status;

  if(rank == r_min)
  {
    /* Min polarity */
    MPI_Send(&in[num_keys - 1], 1, MPI_INT, r_max, 0, MPI_COMM_WORLD);
    MPI_Recv(&val, 1, MPI_INT, r_max, 0, MPI_COMM_WORLD, &status);

    /* Local Detection */
    int c = binSearch(val, in, num_keys);

    int buf_size = num_keys - c;
    int buf[buf_size];
    memcpy(buf, &in[c], (buf_size) * sizeof(*in));

    /* Split and Partial Exchange */
    int ex[num_keys];
    MPI_Send(buf, buf_size, MPI_INT, r_max, 0, MPI_COMM_WORLD);
    MPI_Recv(ex, num_keys, MPI_INT, r_max, 0, MPI_COMM_WORLD, &status);

    int ex_size;
    MPI_Get_count(&status, MPI_INT, &ex_size);

    int i, j = 0, k = 0;
    /* Merge: Keep lowest values */
    for(i = num_keys - buf_size; i < num_keys && j < buf_size && k < ex_size; i++)
    {
      if(buf[j] < ex[k])
        in[i] = buf[j++];
      else
        in[i] = ex[k++];
    }
    for(; i < num_keys && j < buf_size; i++)
      in[i] = buf[j++];
    for(; i < num_keys && k < ex_size; i++)
      in[i] = ex[k++];
  }
  else if(rank == r_max)
  {
    /* Max polarity */
    MPI_Recv(&val, 1, MPI_INT, r_min, 0, MPI_COMM_WORLD, &status);
    MPI_Send(&in[0], 1, MPI_INT, r_min, 0, MPI_COMM_WORLD);

    /* Local Detection */
    int c = binSearch(val, in, num_keys);

    int buf_size = c;
    int buf[buf_size];
    memcpy(buf, &in[0], (c) * sizeof(*in));

    /* Split and Partial Exchange */
    int ex[num_keys];
    MPI_Recv(ex, num_keys, MPI_INT, r_min, 0, MPI_COMM_WORLD, &status);
    MPI_Send(buf, buf_size, MPI_INT, r_min, 0, MPI_COMM_WORLD);

    int ex_size;
    MPI_Get_count(&status, MPI_INT, &ex_size);

    int i, j = buf_size - 1, k = ex_size - 1;
    /* Merge: Keep highest values */
    for(i = buf_size - 1; i >= 0 && j >= 0 && k >= 0; i--)
    {
      if(buf[j] > ex[k])
        in[i] = buf[j--];
      else
        in[i] = ex[k--];
    }
    for(; i >= 0 && j >= 0; i--)
      in[i] = buf[j--];
    for(; i >= 0 && k >= 0; i--)
      in[i] = ex[k--];
  }
  else
  {
    printf("Input rank error!");
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
}

int* bitonicSortManager(int array[], int arraySize, int rank, int size)
{
    /* Divide the values equally between the processors */
    int num_keys = arraySize / size;

    int in[num_keys];

    /* Send to each process its input for Bitonic Sorting */
    MPI_Scatter(
      array,
      num_keys,
      MPI_INT,
      &in,
      num_keys,
      MPI_INT,
      MASTER,
      MPI_COMM_WORLD);

    /* Now each processor has its input data, the simulation can begin */

    /* Each processor sequentially sort its input array using
    iterative bitonic sorting */

    for (int k = 2; k <= num_keys; k = 2 * k)
    {
      for (int j = k / 2; j > 0; j /= 2)
      {
        for (int i = 0; i < num_keys; i++)
        {
          int ixj = i ^ j;
          if ((ixj) > i)
          {
            if ((i&k)==0 && in[i] > in[ixj])
              swap(in, i, ixj);
            if ((i&k)!=0 && in[i] < in[ixj])
              swap(in, i, ixj);
          }
        }
      }
    }

    /* Parallel iterative bitonic sorting */

    for (int k = 2; k <= size; k *= 2)
    {
      for (int j = k / 2; j > 0; j /= 2)
      {
        for (int i = 0; i < size; i++)
        {
          int rankxj = rank ^ j;
          if ((rankxj) > rank)
          {
            if ((rank&k)==0)
              mergeAndSplit(in, rank, rank, rankxj, num_keys);
            if ((rank&k)!=0)
              mergeAndSplit(in, rank, rankxj, rank, num_keys);
          }
          else
          {
            if ((rank&k)==0)
              mergeAndSplit(in, rank, rankxj, rank, num_keys);
            if ((rank&k)!=0)
              mergeAndSplit(in, rank, rank, rankxj, num_keys);
          }
        }
      }
    }

    MPI_Gather(
      in,
      num_keys,
      MPI_INT,
      array,
      num_keys,
      MPI_INT,
      MASTER,
      MPI_COMM_WORLD);

      return array;
}
