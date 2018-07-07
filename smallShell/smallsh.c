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
#include "smallsh.h"


/*****************************************************************************
 MAIN
 ****************************************************************************/
int main()
{
	runCommandLine();
	return 0;
}


/*****************************************************************************
 Function Name: runCommandLine
 Description: This function loops until the shell is terminated by an exit
 	command. It uses several auxilliary functions to provide the shell
 	features.
 ****************************************************************************/
void runCommandLine()
{
	int closeShell = 0;          // Flag to control the duration of operation
	struct DynArr bgProcList;	 // Create dynamic array to track background processes
	initDynArr(&bgProcList, 10); // Initialize array with initial capacity of 10
	
	struct sigaction SIGTSTP_action;  			// Declare sigaction struct for SIGTSTP
	SIGTSTP_action.sa_handler = catchSIGTSTP;  	// Assign fx to be called when SIGTERM received
	SIGTSTP_action.sa_flags = 0;				// No flags set
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);	// Register to call signal handler
	
	struct sigaction SIGINT_action;				// Declare sigaction struct
	SIGINT_action.sa_handler = SIG_IGN;  		// Set to ignore SIGINT (modified in runInForeground())
	sigfillset(&SIGINT_action.sa_mask);		    // Block all signals while handler is executing
	sigaction(SIGINT, &SIGINT_action, NULL);    // Register to ignore signal 2

	// Shell runs until closeShell flag is set to one (true) by a call to exit
	do
	{
		char** cmdLineArgs = malloc(ARGS_MAX * sizeof(char*)); // Holds command line argument strings
		char* userInput; // To hold user input from getInput()
		int ignoreInput = 0; // To accomodate comments and blank lines
		struct specialFlags spFlags; // To denote the presence of any special characters <, >, and &
		
		
		userInput = getInput(); // GET USER INPUT
		
		if(userInput[0] == '#' || userInput[0] == '\n')
		{
			ignoreInput = 1; // IGNORE COMMENTS AND BLANK LINES
		}
		
		if(!ignoreInput)
		{
			if(strstr(userInput, "$$")) // EXPAND ANY INSTANCES OF "$$"
			{
				expand$$(userInput);
			}
			
			// Parse command line string
			parseCommand(userInput, cmdLineArgs, &spFlags);
			
			if(strcmp(cmdLineArgs[0], "exit") == 0) {	// BUILT-IN exit COMMAND
				closeShell = 1;
				// Clean up bg processes
				endBgProcesses(&bgProcList);
			} else if (strcmp(cmdLineArgs[0], "cd") == 0) {		// BUILT-IN cd COMMAND
				chgShDir(cmdLineArgs[1]); // Change directory
			} else if(strcmp(cmdLineArgs[0], "status") == 0) {		// BUILT-IN status COMMAND
				dispStatus();
			} else {
				if(spFlags.runInBg == 1 && !disableBackground) {      // RUN NON-BUILT-IN PROGRAM IN BACKGROUND
					runInBackground(cmdLineArgs, &spFlags, &bgProcList);
				}
				else {
					runInForeground(cmdLineArgs, &spFlags, &bgProcList); // RUN NON-BUILT-IN PROGRAM IN FOREGROUND
					sigaction(SIGINT, &SIGINT_action, NULL); // Reset to ignore SIGINT
				}
			}
		}
		// CHECK FOR COMPLETED BG PROCESSES B4 LOOPING BACK TO RETURN COMMAND LINE CONTROL TO USER
		if(sizeDynArr(&bgProcList) != 0) { chkBgProcCompl(&bgProcList); }
		free(cmdLineArgs); // FREE/RESET COMMAND LINE ARGUMENTS
		free(userInput);   // FREE MEM ALLOCATED BY getline() FOR LAST COMMAND
	} while(!closeShell);
	
	freeDynArr(&bgProcList); // Free the dynamic array of background processes
}


/*****************************************************************************
 Function Name: getInput
 Description: This function gets a line of input from the user. It
 returns the line entered. The calling function is responsible for
 freeing the memory allocated.
 Reference Citation: Lecture material: 3.3 Advanced User Input with
 getline() was explicitly referenced to address complications due to signal
 interruption.
 ****************************************************************************/
char* getInput()
{
	char* inputBuff = NULL; // Points to a buffer allocated by getline
	size_t buffSize = 0;    // Tracks size of the allocated buffer
	int charCount; 			// Holds return value from getline
	// Get a line of user input, loop until no error from signal interruption
	while(1)
	{
		printf(": ");       // DISPLAY COMMAND LINE PROMPT
		fflush(stdout);
		
		charCount = getline(&inputBuff, &buffSize, stdin);  // Read from stdin
		
		if(charCount == -1) 	// Check for getline() call error
		{
			clearerr(stdin);	// Remove error status from stdin before continuing
		}
		else
		{
			break;	// Valid input received, exit loop
		}
	}
	return inputBuff; // Return input
}


/*****************************************************************************
 Function Name: expand$$
 Description: This function takes an input string of characters and replaces
 all instances of "$$" in the string into the process ID of the shell
 itself.
 Reference Citation: https://www.tutorialspoint.com/c_standard_library/string_h.htm
 ****************************************************************************/
void expand$$ (char* inputString)
{
	int shProcId = getpid();		  // Get process id to replace $$
	char* stringToken;				  // Holds next token
	char expandedString[MAX_CHARS];   // Holds the expanded string
	memset(expandedString, '\0', sizeof(expandedString));
	
	stringToken = strtok(inputString, "$$"); // Get first token
	// Replace all instances of $$ in the given string with shell process ID
	while(stringToken != NULL)
	{
		strcat(expandedString, stringToken); // Add text prior to $$
		stringToken = strtok(NULL, "$$");
		// Add process id if not simply the end of string
		if(stringToken != NULL) {
			// Add process id in place of $$
			sprintf(&expandedString[strlen(expandedString)], "%d", shProcId);
		}
	}
	// Replace user input string with expanded string
	strcpy(inputString, expandedString);
}


/*******************************************************************************
 Function Name: parseCommand
 Description: This function uses strtok() to parse the command line input.
 It stores each token in the array passed to be used later as command
 arguments. It also checks for special characters (>, <, &) and uses the
 passed spFlags struct to flag the presence of standard input, standard
 output, and run-in background preferences. In the case that standard input
 and/or output are intended to be redirected, it also stores the filename
 provided by the user in the spFlags struct.
 ******************************************************************************/
void parseCommand(char* input, char** argTokens, struct specialFlags* spFlags)
{
	char *nextArg; // Holds next token
	int count = 0; // Tracks number of arguments
	// Get first argument
	nextArg = strtok(input, " \n");	// Get first token/argument
	clearSpecialFlags(spFlags);		// Clear the special flags struct for new command
	
	
	// Cycle through the input string adding each argument to argTokens array
	while(nextArg != NULL)
	{
		// Check for special characters and set any needed flags in struct
		if(strcmp(nextArg, "<") == 0)
		{
			spFlags->inputRedir = 1;  // Set flag to redirect input
			spFlags->inputfile = strtok(NULL, " \n"); // Store file to redirect to
		} else if(strcmp(nextArg, ">") == 0) {
			spFlags->outputRedir = 1;  // Set flag to redirect input
			spFlags->outputfile = strtok(NULL, " \n"); // Store file to redirect to
		} else {
			argTokens[count] = nextArg; // Add argument to commands array
			count++;					// Increment argument count
		}
		nextArg = strtok(NULL, " \n"); // Get next token
	}
	// Check for & as last argument to indicate run in background
	if(strcmp(argTokens[count-1], "&") == 0)
	{
		spFlags->runInBg = 1; // Set flag to run in background
		count--; // Decrement command argument count
		argTokens[count] = NULL; // null term in place of &
	} else {
		argTokens[count] = NULL; 	// null terminate array of arguments
	}
}


/*****************************************************************************
 Function Name: runInBackground
 Description: This function is used by runCommandLine() to run commands in
 	the background of the shell. Command line access and control is returned
 	immediately to the user.
 Reference Citation: https://linux.die.net/man/2/waitpid
 ****************************************************************************/
void runInBackground(char** cmdArgs, struct specialFlags* spFlags, struct DynArr* bgProcList)
{
	// Change sigaction struct for SIGTSTP (To prevent current bg processes from stopping)
	struct sigaction SIGTSTP_action_bg;		 // Initialize
	SIGTSTP_action_bg.sa_handler = SIG_IGN;  // Bg processes ignore SIGTSTP, survives the exec() call
	SIGTSTP_action_bg.sa_flags = 0;			 // No flags
	
	pid_t spawnPid;
	
	// Spawn a new process
	spawnPid = fork();	// value returned will be pid of child(in parent) or 0(in child)

	switch (spawnPid)
	{
		// Check for error from fork() call
		case -1:
			perror("Error by fork() call.\n");
			break;
		// Case for child process (execute command)
		case 0:
			sigaction(SIGTSTP, &SIGTSTP_action_bg, NULL);		// Register a modified action for bg processes
			// Handle input redirection
			if(spFlags->inputRedir) {
				inputRedir(spFlags);          // Use command line arguments, if given
			} else {
				// Otherwise, redirect to avoid reading from standard input
				int FD;
				FD = open("/dev/null", O_RDONLY);
				dup2(FD, 0);
			}
			// Handle output redirection
			if(spFlags->outputRedir) { 		  // Use command line arguments, if given
				outputRedir(spFlags);
			} else {
				// Otherwise, redirect standard output to /dev/null
				int FD;
				FD = open("/dev/null", O_WRONLY);
				dup2(FD, 1);
			}
			
			// Execute command
			execvp(cmdArgs[0], cmdArgs);
			// The following will only execute if an error occurs in execvp() call
			perror("Command could not be executed.\n");
			exit(1);
			break;
			// Case for parent
		default:
			// Don't wait on process (run in background)
			printf("background pid is %d\n", spawnPid);		// Print process id to screen
			fflush(stdout);
			// Add bg process pid to list of currently running bg processes
			addDynArr(bgProcList, spawnPid);
	}
}


/*****************************************************************************
 Function Name: runInForeground
 Description: This function is used by runCommandLine() to run commands in
 	the foreground of the shell. The parent shell does not return command line
 	access and control to the user until the child terminates.
 Reference Citation: https://linux.die.net/man/2/waitpid
 ****************************************************************************/
void runInForeground(char** cmdArgs, struct specialFlags* spFlags, struct DynArr* bgProcList)
{
	pid_t spawnPid;
	
	// Change sigaction struct for SIGINT for foreground specifications (So SIGINT can kill fg process)
	struct sigaction SIGINT_action_fg;	   	 		// Initialize
	SIGINT_action_fg.sa_handler = catchSIGINT;  	// Assign custom SIGINT handler for foreground processes
	sigfillset(&SIGINT_action_fg.sa_mask);	     	// Block all signals while handler is executing
	SIGINT_action_fg.sa_flags = SA_RESTART;			// To handle interupted sys calls
	sigaction(SIGINT, &SIGINT_action_fg, NULL);		// Register the modified action for bg processes
	
	// Spawn a new process
	spawnPid = fork();	// value returned will be pid of child(in parent) or 0(in child)
	
	switch (spawnPid)
	{
		// Check for error from fork() call
		case -1:
			perror("Error by fork() call.\n");
			break;
		// Case for child process (execute command)
		case 0:
			// Handle possible input redirection
			if(spFlags->inputRedir) { inputRedir(spFlags); }
			// Handle possible output redirection
			if(spFlags->outputRedir) { outputRedir(spFlags); }
			// Execute command
			execvp(cmdArgs[0], cmdArgs);
			// The following will only execute if an error occurs in execvp call
			perror("Command could not be executed.\n");
			exit(1);
			break;
		// Case for parent
		default:
			// Run in foreground (i.e. wait until process is finished)
			waitpid(spawnPid, &prevStatus, 0);
	}
}


/*****************************************************************************
 Function Name: chgShDir
 Description: This function is used to change the working directory of the
 	shell. If the argument given is NULL, the working directory is changed to
 	the directory specified in the HOME environment variable. If an argument
 	is not NULL (assuming it is valid) the working directory is changed as
 	specified by the path given in the argument. Both absolute and relative
 	paths are supported.
 ****************************************************************************/
void chgShDir (char* dirpath)
{
	// If no path, change to directory specified in the HOME env var
	if(dirpath == NULL) {
		char *homeDir = getenv("HOME");
		chdir(homeDir);
	} else {
		// Change directory based on given path
		if(chdir(dirpath) == - 1) {
			//Print error is the directory given is invalid
			printf("The directory path given is invalid.\n");
			fflush(stdout);
		}
	}
}


/*****************************************************************************
 Function Name: clearSpecialFlags
 Description: This function resets all variables in a specialFlags struct
 	in order to prepare for the next command entered.
 ****************************************************************************/
void clearSpecialFlags(struct specialFlags* flagStruct)
{
	flagStruct->inputRedir = 0;
	flagStruct->inputfile = NULL;
	flagStruct->outputRedir = 0;
	flagStruct->outputfile = NULL;
	flagStruct->runInBg = 0;
}


/*****************************************************************************
 Function Name: inputRedir
 Description: This function redirects stdin (FD = 0) to the a different
 	file descriptor based on a given file provided by the user and stored in
 	the specialFlags struct.
  Reference Citation: http://man7.org/linux/man-pages/man2/dup.2.html
 ****************************************************************************/
void inputRedir(struct specialFlags* spFlags)
{
	int inputFD;
	
	if((inputFD = open(spFlags->inputfile, O_RDONLY)) == -1)
	{
		// Print error and set exit status to 1, if unable to open
		printf("cannot open %s for input\n", spFlags->inputfile);
		fflush(stdout);
		exit(1);
	}
	else // Redirect stdin to file descriptor based on user input
	{
		if((dup2(inputFD, 0)) == -1)
		{
			// Print error if dup2() call fails
			perror("dup2() error redirecting input\n");
		}
	}
}


/*****************************************************************************
 Function Name: outputRedir
 Description: This function redirects stdout (FD = 1) to the a different
 file descriptor based on a given file provided by the user and stored in
 the specialFlags struct.
 Reference Citation: http://man7.org/linux/man-pages/man2/dup.2.html
 ****************************************************************************/
void outputRedir(struct specialFlags* spFlags)
{
	int outputFD;
	
	// Open file to use for redirection of standard output (i.e. FD 1)
	if((outputFD = open(spFlags->outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0664)) == - 1)
	{
		// Print error and set exit status to 1, if unable to open
		printf("cannot open %s for output\n", spFlags->outputfile);
		fflush(stdout);
		exit(1);
	}
	else // Redirect stdout to file descriptor based on user input
	{
		if((dup2(outputFD, 1)) == -1)
		{
			// Print error if dup2() call fails
			perror("dup2() error redirecting output\n");
		}
	}
}


/*****************************************************************************
 Function Name: chkBgProcCompl
 Description: This function is used to check for completed background
 	process completion. It takes a list of currently running background
 	processes and uses waitpid() with "WNOHANG" argument to check the
 	completion status of each background process. If a background is found
 	to have been completed or terminated, then the exit status or the
 	terminating signal is printed to notify the user.
 Reference Citation: https://linux.die.net/man/2/waitpid
 ****************************************************************************/
void chkBgProcCompl(struct DynArr* bgProcList)
{
	int procExitMethod = -7, i;
	pid_t pidDone;
	// Reap any finished bg processes
	for(i = 0; i < sizeDynArr(bgProcList); i++)
	{
		// Check each bg process in list to determine if finished, and if so to reap
		pidDone = waitpid(bgProcList->data[i], &procExitMethod, WNOHANG);
		if(pidDone > 0)	// If no error and a child has completed print exit/termination value (mutually exclusive)
		{
			if(WIFEXITED(procExitMethod))
			{
				// Notify user of process completion and exit value
				printf("background pid %d is done: exit value %d\n", bgProcList->data[i], WEXITSTATUS(procExitMethod));
				fflush(stdout);
			}
			else
			{
				// Notify user of process completion and termination signal number
				printf("background pid %d is done: terminated by signal %d\n", bgProcList->data[i], WTERMSIG(procExitMethod));
				fflush(stdout);
			}
			// Remove process from background process array
			removeAtDynArr(bgProcList, i);
			i--; // Account for dyn arr shifting values 
		}
	}
}


/*****************************************************************************
 Function Name: catchSIGINT
 Description: This is the signal handler for SIGINT. It prevents the default
 	action of process termination of the parent process (shell), because only
 	SIG_DFL and SIG_IGN are preserved through exec(). Foreground child
 	processes are terminated, as a result of an addtional call
    to sigaction() with a custom signal handler assigned. A message is printed
    to indicate that the foreground process was terminated by siganl 2 (the
    value for SIGINT). Background processes are not terminated, because the
    they inherit the SIG_IGN assigned handler, which is always set outside of
    runInForeground().
 ****************************************************************************/
void catchSIGINT(int sigNum)
{
	char* termMessage = "terminated by signal 2\n";  // Print notice of term signal
	write(STDOUT_FILENO, termMessage, 23);
}


/*****************************************************************************
 Function Name: catchSIGTSTP
 Description: This function is used as the signal handler for SIGTSTP. It
 	prevents the default action of stopping execution. Currently running
 	background processes are not affected given an additional call to
 	sigaction(), which registers the SIG_IGN handler for the background
 	process. However, this handler diplays an informative message immediately
 	any currently running foreground process has terminated, and causes the
 	shell to enter a state where subsequent programs can no longer be run
 	in the background, until a SIGTSTP signal is sent again. Upon receipt of
 	a subsequent SIGTSTP another message is displayed, and background
 	processes are again allowed.
 ****************************************************************************/
void catchSIGTSTP(int sigNum)
{
	// If background processes have not been disabled by a previous signal
	if(!disableBackground) {
		char* enterMessage = "\nEntering foreground-only mode (& is now ignored)\n";
		write(STDOUT_FILENO, enterMessage, 50);
		disableBackground = 1; // Background processes disabled
	} else {
		char* exitMessage = "\nExiting foreground-only mode\n";
		write(STDOUT_FILENO, exitMessage, 30);
		disableBackground = 0; // Background processes enabled
	}
}


/*****************************************************************************
 Function Name: endBgProcesses
 Description: This function is used upon a call to exit in runCommandLin()
 	to cleanup all currently running background processes.
 ****************************************************************************/
void endBgProcesses(struct DynArr* bgProcList)
{
	int i = 0, numProcesses = sizeDynArr(bgProcList), exitStat = 0;
	
	// Clean up background processes with SIGTERM (signal 15)
	for(i = 0; i < numProcesses; i++)
	{
		kill(bgProcList->data[i], SIGTERM);
		waitpid(bgProcList->data[i], &exitStat, 0); // Reap to prevent zombies
		// Note: the values are freed from the background process list in
		 	// the runCommandLine() function prior to ending the shell process.
	}
}


/*****************************************************************************
 Function Name: displayStatus
 Description: This function displays either the exit status or the
 	terminating signal of the last foreground process ran by the shell.
 	The global variable prevStatus is used to look up last exit/term status.
 ****************************************************************************/
void dispStatus()
{
	// Check to determin if previous foreground process was exited or terminated (mutually exclusive)
	if(WIFSIGNALED(prevStatus))
	{
		int termSig = WTERMSIG(prevStatus);
		// Notify user of process completion and termination signal number
		printf("Last foreground process terminated by signal %d\n", termSig);
		fflush(stdout);
	}
	else
	{
		int exitStatus = WEXITSTATUS(prevStatus);
		// Notify user of process completion and exit value
		printf("Last foreground process exit value %d\n", exitStatus);
		fflush(stdout);
	}
}


