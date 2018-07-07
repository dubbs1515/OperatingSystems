#ifndef smallsh_h
#define smallsh_h
/****************************************************************************
 Program Name: smallsh
 Author: Christopher Dubbs
 Date: February 20, 2018
 Class: CS 344
 Description: This program implements a simple shell.
 Reference Citation: "The Linux Programming Interface", Michael Kerrisk
    ISBN: 9781593272203
 Reference Citation: All of block 3 lecture material (OSU, CS 344)
 Reference Citation: https://www.gnu.org/software/libc/manual/html_node/Signal-Handling.html
 Reference Citation: http://man7.org/linux/man-pages/man2/sigaction.2.html
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "dynArr.h"

#define ARGS_MAX 512 // Specify max # of args to accept for a command
#define MAX_CHARS 2048

// Struct to denote presence of special arguments
struct specialFlags {
    int inputRedir;        // denotes input should be redirected
    char* inputfile;    // file name to redirect input
    int outputRedir;    // denotes output should be redirected
    char* outputfile;    // file name to redirect output
    int runInBg;         // denotes designation as background process
};

// Global
int prevStatus;   // For tracking exit status of last foreground process
int disableBackground;  // Global Background/Foreground Mode flag

// Function Prototypes
char* getInput(void);
void parseCommand(char* input, char** argTokens, struct specialFlags* spFlags);
void runCommandLine(void);
void runInForeground(char** cmdArgs, struct specialFlags* spFlags, struct DynArr* bgProcList);
void runInBackground(char** cmdArgs, struct specialFlags* spFlags, struct DynArr* bgProcList);
void chgShDir (char* dirpath);
void expand$$ (char* inputString);
void clearSpecialFlags(struct specialFlags* flagStruct);
void inputRedir(struct specialFlags* spFlags);
void outputRedir(struct specialFlags* spFlags);
void chkBgProcCompl(struct DynArr* bgProcList);
void catchSIGINT(int sigNum);
void catchSIGTSTP(int sigNum);
void endBgProcesses(struct DynArr* bgProcList);
void dispStatus(void);

#endif /* smallsh_h */
