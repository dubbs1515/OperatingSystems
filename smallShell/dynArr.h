/***********************************************************************
 * Dynamic Array Header File
 * OSU CS261 Coursework
 **********************************************************************/
#ifndef TYPE
#define TYPE pid_t
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

struct DynArr {
    TYPE *data;      // Data array pointer
    int size;        // Tracks current size (# of elements)
    int capacity;    // # of elements the array can currently hold
};

// Function Prototypes
void initDynArr(struct DynArr *dArr, int capacity);
void freeDynArr(struct DynArr *dArr);
int sizeDynArr(struct DynArr *dArr);
void addDynArr(struct DynArr *dArr, TYPE val);
void _setCapacityDynArr(struct DynArr *dArr, int newCap);
TYPE getDynArr(struct DynArr *dArr, int position);
void putDynArr(struct DynArr *dArr, int position, TYPE value);
void swapDynArr(struct DynArr *dArr, int i, int j);
void removeAtDynArr(struct DynArr *dArr, int index);

