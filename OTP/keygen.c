/**********************************************************************************
 Program Name: genkey
 Author: Christopher Dubbs
 Class: CS344
 Date: March 1, 2018
 Description: This program creates a key file of a specified length. The characters
     in the file generated are any of the permitted 27 (i.e. all 26 capital
     alphabet letters plus space), as generated using the standard UNIX randomization
     methods. The last character the program outputs is a newline. All error text,
     if any, is output to stderr.
     The syntax for keygen is: keygen <keylength>
     Where keylength is the length of the key file in characters. keygen outputs
     to stdout.
 *********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


// Function Prototypes
char* produceKey(int keyLen);
char genRandChar(void);

#define ASCII_MIN 64    // @ NOTE: @ will be replaced with space if encountered
#define ASCII_MAX 90    // Z


/****************************************************************************
 MAIN
 ****************************************************************************/
int main(int argc, const char * argv[]) {
    int keyLen;
    char* encryptionKey;
    
    // Seed rand
    unsigned int seed = (unsigned int) (time(0));
    srand(seed);
    
    keyLen = atoi(argv[1]); // Store requested length
    // Ensure a key length recvd from cmd line
    if(argc < 2)
    {
        fprintf(stderr, "Expected a key length argument.\n");
        exit(1);
    }
    
    encryptionKey = produceKey(keyLen); // Generate key
    
    // Print key
    printf("%s", encryptionKey);
    
    free(encryptionKey); // Deallocate memory
    return 0;
}


/****************************************************************************
 * Function Name: genRandChar
 * Description: This function generates and returns a random caplital alphabet
    character or a space.
 ****************************************************************************/
char genRandChar()
{
    char nextChar;
    // Generate and return a random cap letter
    nextChar = rand() % ((ASCII_MAX + 1) - ASCII_MIN) + ASCII_MIN;
    
    // Replace '@' with a space
    if(nextChar == 64)  // i.e. ASCII '@'
    {
        nextChar = 32;  // i.e. ASCII ' '
    }
    
    return nextChar;
}


/****************************************************************************
 * Function Name: produceKey
 * Description: This function is used to produce the random string of
    characters. 
 ****************************************************************************/
char* produceKey(int keyLen)
{
    int i;
    char* newKey = malloc((keyLen + 1) * sizeof(char));   // Allocate memory
    
    for(i=0; i < keyLen; i++)   // Generate requested number of characters
    {
        newKey[i] = genRandChar(); // Otherwise, add cap letter
    }
    newKey[i] = '\n'; // Add newline to end
    return newKey;
}

