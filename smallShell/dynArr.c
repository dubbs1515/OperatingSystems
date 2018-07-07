/***********************************************************************
* Dynamic Array Source File
* OSU CS261 Coursework
 **********************************************************************/
#include "dynArr.h"

/***********************************************************************
 * Initialize Dynamic Array
 **********************************************************************/
void initDynArr(struct DynArr *dArr, int capacity)
{
    dArr->data = malloc(sizeof(TYPE) * capacity); // Allocate memory per the capacity param and data type
    assert(dArr->data != 0);   // Verify successful memory allocation
    dArr->size = 0;            // Initialize # of elements to zero
    dArr->capacity = capacity; // Set initial capacity per given parameter
}


/***********************************************************************
 * Free Dynamic Array
 **********************************************************************/
void freeDynArr(struct DynArr *dArr)
{
    if(dArr->data != 0) // Ensure array ptr is not null
    {
        free(dArr->data);  // Free memory allocated for the array
        dArr->data = 0;    // Set to null
    }
    dArr->size = 0;        // Update array size
    dArr->capacity = 0;    // Update array capacity
}


/***********************************************************************
 * Get Size of Dynamic Array
 **********************************************************************/
int sizeDynArr(struct DynArr *dArr)
{
    return dArr->size; // Return the current size of array (# of elements)
}


/***********************************************************************
 * Add Value to Dynamic Array
 **********************************************************************/
void addDynArr(struct DynArr *dArr, TYPE val)
{
    // Check to to determine if resize is necessary
    if(dArr->size >= dArr->capacity)
    {
        _setCapacityDynArr(dArr, 2 * dArr->capacity); // Double curr capacity
    }
    
    dArr->data[dArr->size] = val; // Fill next empty index location
    dArr->size++;                 // Update size value
}


/***********************************************************************
 * Set/Reset the Capacity of Dynamic Array
 **********************************************************************/
void _setCapacityDynArr(struct DynArr *dArr, int newCap)
{
    
    TYPE *oldArray = dArr->data;        // Save old array for value copying
    
    int oldSize = dArr->size;           // Save old array size for value copying
    
    int i;
    
    initDynArr(dArr, newCap);   // Initialize new array double capacity of old
    
    for(i = 0; i < oldSize; i++)  // Copy values from old array to new array
    {
        dArr->data[i] = oldArray[i];
    }
    
    dArr->size = oldSize;       // Set new array size (# of elements in old array)
    
    free(oldArray);             // Free memory allocated for old array
}


/***********************************************************************
 * Get Dynamic Array Value at Index
 **********************************************************************/
TYPE getDynArr(struct DynArr *dArr, int position)
{
    assert(position >= 0 && position < dArr->size); // Ensure valid index
    return dArr->data[position];        // Return value at index
}


/***********************************************************************
 * Insert Value into Dynamic Array at Index
 **********************************************************************/
void putDynArr(struct DynArr *dArr, int position, TYPE value)
{
    assert(position >= 0 && position < dArr->size); // Ensure valid index
    dArr->data[position] = value;       // Insert value at index given
}


/***********************************************************************
 * Swap values at two given Indicies
 **********************************************************************/
void swapDynArr(struct DynArr *dArr, int i, int j)
{
    assert(i >= 0 && i < dArr->size);   // Ensure index of value 1 is valid
    assert(i >= 0 && i < dArr->size);   // Ensure index of value 2 is valid
    // Swap values at the two indicies
    TYPE temp = getDynArr(dArr, i);
    putDynArr(dArr, i, getDynArr(dArr, j));
    putDynArr(dArr, j, temp);
}


/***********************************************************************
 * Remove Value from Dynamic Array at a Given Index
 **********************************************************************/
void removeAtDynArr(struct DynArr *dArr, int index)
{
    int i;
    
    assert(index >= 0 && index < dArr->size);   // Ensure index is valid 
    
    for(i = index; i < dArr->size-1; i++) // slide values left
    {
        dArr->data[i] = dArr->data[i + 1]; // move each value over to left one at a time
    }
    
    dArr->size--; // Reduce array size value
}


















