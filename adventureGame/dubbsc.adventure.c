
/****************************************************************************
 *Program Name: adventure
 *Author: Christopher Dubbs
 *Date: January 30, 2018
 *Course: CS 344
 *Description: This program provides an interface for playing the "adventure"
	game uing the most recently generated rooms (i.e. by buildrooms
	program). A player will begin in a "starting room" (as dictated by
 	buildroom.c program) and will win the game upon entering the likewise
 	designated "end room." At any time the player can enter a command, "time,"
 	that will return the current time.
 *Note: A linked list was also implemented within this file
 	(partially for usage, as well as, for practice). The linked list
 	functions follow the program specific functions.
 *Reference Citation: All of CS344 Block 2 Materials
 *Reference Citation: http://www.csc.villanova.edu/~mdamian/threads/posixthreads.html
 ****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h> // For POSIX threads

#ifndef __TYPE  // Type specification for linked list
#define __TYPE
#define TYPE int
#define TYPE_SIZE sizeof(int)
#endif

#define MAX_CONX 6
#define ROOM_LIST_MAX 7

// Setup for the multi-thread/mutex portion of program
pthread_mutex_t timeFileLock; // Lock for mutually exclusive usage of time file
pthread_t timeThread;  // to hold thread identifier

// enum to represent room types
enum ROOM_TYPE { START_ROOM, MID_ROOM, END_ROOM };

//ROOM STRUCT
struct room
{
	int rmId; // i.e. index of name in r_names
	char name[20];  // (i.e. index of one of ten possible room names)
	enum ROOM_TYPE rType;  // (i.e. START_ROOM (0), MID_ROOM (1), or END_ROOM (2))
	int outboundConxs[MAX_CONX]; // Holds indexes of other rooms in roomList
	int cnxCount; // Tracks total # of outbound connections crrently assigned
};

// Linked List Node
struct link
{
	TYPE value;
	struct link *next;
	struct link *prev;
};

//Linked List
struct linkList
{
	int size;
	struct link* listFront; // Sentinel (Does not contain a value)
	struct link* listBack;  // Sentinel (Does not contain a value)
};

struct timeStruct {
	
	FILE* timeFile;
};

// FUNCTION PROTOTYPES
void* writeTime(void* stop);
void displayTime(void);
void printPath(struct room* rmArr, struct linkList* list);
int isGameOver(struct room rm);
int getStartLoc(struct room* rmArr);
char* getInput(void);
char* findNewDir(struct room* rmArr);
void readInData(struct room* rmArr);
void dispCurLoc(struct room* rmArr, int currLoc);
void initRoomList(struct room* rmArr);
void playGame(void);
struct linkList* createDeque(void);
int listIsEmpty(struct linkList* list);
void listAddBack(struct linkList * list, TYPE val);
void listAddFront(struct linkList *list, TYPE val);
void listRemoveBack(struct linkList* list);
void listRemoveFront(struct linkList* list);
TYPE listFront(struct linkList* list);
TYPE listBack(struct linkList* list);
void freeList(struct linkList* list);

/****************************************************************************
 MAIN
 ****************************************************************************/
int main() {
	
	// Create mutex for time file access
	if(pthread_mutex_init(&timeFileLock, NULL) != 0)
	{
		printf("\nMUTEX INIT ERROR.\n");
	}
	
	// Begin Game
	playGame();
	
	// Destroy mutex for time file access
	//pthread_mutex_destroy(&timeFileLock);
	return 0;
}


////// PROGRAM SPECIFIC FUNCTIONS /////////////////////////////////////////////

/****************************************************************************
 *Function Name: writeTime
 *Description: This function serves a time writing thread (Thread B).
 	The function prints the time to the text file "currentTime.txt" in the
 following format: 1:03pm: Tuesday, September 13, 2016
 *Reference Citation: http://man7.org/linux/man-pages/man3/strftime.3.html
 ****************************************************************************/
void* writeTime(void* stop)
{
	// Lock down the time file
	pthread_mutex_lock(&timeFileLock);
	
	char formatedTime[64]; // To hold the formatted time.
	memset(formatedTime, '\0', sizeof(formatedTime));
	FILE* timeFileFP;
	timeFileFP = fopen("currentTime.txt", "w"); // Create or open the time file
	time_t tm;
	time(&tm); // Get current time & save to time_t var
	struct tm *time = localtime(&tm); // Fill the tm struct with curr time vals
	
    // Format time to be written to the file
	strftime(formatedTime, sizeof(formatedTime), "%I:%M%P, %A, %B %d, %Y\n", time);
		  
	// Print the current time to the time file in the req'd format
	fprintf(timeFileFP, "%s", formatedTime);
	
	fclose(timeFileFP);
	
	// Unlock the time file
	pthread_mutex_unlock(&timeFileLock);
	
	return NULL;
}

/****************************************************************************
 *Function Name: displayTime
 *Description: This function reads and displays the time from the currTime.txt
 	file.
 ****************************************************************************/
void displayTime()
{
	FILE* timeFileFP;
	timeFileFP = fopen("currentTime.txt", "r"); // Open to read
	char currChar = fgetc(timeFileFP);
	while(currChar != EOF)
	{
		printf("%c", currChar); // print current contents of file by char
		currChar = fgetc(timeFileFP);
	}
	printf("\n");
	fclose(timeFileFP); // Break connection b/n FP and time file
}

/****************************************************************************
 *Function Name: printPath
 *Description: This function prints the path taken to the end room.
 	It takes a linked list argument (holding the indices for each room
 	visited). The indicies are then used to reference specific room names
 	held in the room struct array.
****************************************************************************/
void printPath(struct room* rmArr, struct linkList* list)
{
	struct link* currLink = list->listFront->next;
	while(currLink != list->listBack)
	{
		printf("%s\n", rmArr[currLink->value].name);
		currLink = currLink->next;
	}
}

/****************************************************************************
 *Function Name: isGameOver
 *Description: This function returns true (1) if the game is over
 	(i.e. current room is END_ROOM) and false (0) otherwise.
 ****************************************************************************/
int isGameOver(struct room rm)
{
	return (rm.rType == END_ROOM);
}

/****************************************************************************
 *Function Name: getStartLoc
 *Description: This function returns an integer denoting the starting room.
 	(i.e. it's index in the room struct array.)
****************************************************************************/
int getStartLoc(struct room* rmArr)
{
	int startLoc = 0;
	int i;
	
	// Find starting location index
	for( i = 0; i < ROOM_LIST_MAX; i++)
	{
		if(rmArr[i].rType == START_ROOM)
		{
			startLoc = i;
		}
	}
	return startLoc;
}

/****************************************************************************
 *Function Name: getInput
 *Description: This function reads input from stdin. It returns the line
 	entered. The calling function is responsible for freeing the memory.
****************************************************************************/
char* getInput()
{
	char* inBuff = NULL; //Points toa buffer allocated by getline
	size_t buffSize = 0; //Tracks the size of the allocated buffer
	
	// Get user input
	printf("WHERE TO? >");
	fflush(stdout);
	getline(&inBuff, &buffSize, stdin); // Get a line from stdin
	printf("\n");
	return inBuff;
}

/****************************************************************************
 *Function Name: findNewDir
 *Description: This function finds and returns the most recently created
 	rooms directory. The calling function, using the directory name, is
 	responsible for freeing the memory allocated to hold the directory name.
 *Reference citation: CS344 lecture material 2.4 Manipulatin Directories
 ****************************************************************************/
char* findNewDir(struct room* rmArr)
{
	DIR* workingDir; // Holds current working directory (directory stream)
	struct dirent* dirEntry; // Holds a directory entry
	struct stat dirInfo;     // Holds info about subdir
	int latestDirTime = -1; // Modified timestamp of latest subdir analyzed
	char tarDirPrefix[32] = "dubbsc.rooms."; // Prefix of target directories
	char* latestDirName = calloc(128, sizeof(char)); // Holds name of latest subdir

	// 1. FIND MOST RECENT ROOMS SUB-DIRECTORY
	workingDir = opendir("."); // Open current directory
	if( workingDir > 0) // Ensure directory successfully opened
	{
		while((dirEntry = readdir(workingDir)) != NULL) // Loop through all entries
		{
			if(strstr(dirEntry->d_name, tarDirPrefix) != NULL) // Check entries with prefix
			{
				// Determine if latest time stamp is more recent than previously checked
				stat(dirEntry->d_name, &dirInfo);
				if((int)dirInfo.st_mtime > latestDirTime)
				{
					// update the most recent directory timestamp found
					latestDirTime = ((int)dirInfo.st_mtime);
					// update the most recent directory name var
					//memset(latestDirName, '\0', sizeof(latestDirName));
					strcpy(latestDirName, dirEntry->d_name);
				}
			}
		}
	}
	closedir(workingDir); // close current directory
	return latestDirName;
}

/****************************************************************************
 *Function Name: readInData
 *Description: This function reads the data from the most recent rooms
 	directory and saves the data to representative room structs for
 	use throughout the duration of the game.
 *Reference citation: CS344 lecture material 2.4 Manipulatin Directories
 ****************************************************************************/
void readInData(struct room* rmArr)
{
	char* latestDir = findNewDir(rmArr); // Find the newest directory
	
	
	// FILL IN THE ROOM NAMES OF ALL ROOM STRUCTS IN ROOM ARRAY
	
	int rmIndex = 0; // index for rmArr
	// OPEN THE LATEST BUILDROOMS DIRECTORY AND FILL ROOMS ARRAY
	DIR* workingDir = opendir(latestDir); // Open the latest directory
	struct dirent* dirEntry; // Holds a directory entry
	if( workingDir > 0) // Ensure directory successfully opened
	{
		// FOR EACH FILE IN DIRECTORY
		while((dirEntry = readdir(workingDir)) != 0)
		{
			if(strlen(dirEntry->d_name) > 2) // Copy the name of each file/room
			{
				strcpy(rmArr[rmIndex].name, dirEntry->d_name);
				rmIndex++;
			}
		}
	}

	// OPEN EACH FILE BY NAME AND POPULATE REMAINING DATA
	FILE* currFile; // File stream
	currFile = 0;
	char lineBuff[256]; // Buffer for each line of file
	memset(lineBuff, '\0', sizeof(lineBuff));
	char filePathBuff[128]; // Buffer for file path
	memset(lineBuff, '\0', sizeof(lineBuff));
	int i;
	for(i = 0; i < ROOM_LIST_MAX; i++)
	{

		memset(filePathBuff, '\0', sizeof(filePathBuff));
		sprintf(filePathBuff, "./%s/%s", latestDir, rmArr[i].name);
		currFile = fopen(filePathBuff, "r"); // Open each file for reading
		//int conCount = 0; // Count number of connections
		//Read each line in file
		while(fgets(lineBuff, 256, currFile) != NULL)
		{
			//char* tok1;  // Holds string to left of :
			//char* tok2;  // Holds :
			char* key;	 // Holds string key (left of :)
			char* val; // Holds cleaned string value (right of :)
			
			// Use strtok to parse the room information
			key = strtok(lineBuff, ":"); // Store key
			val = strtok(NULL, " \n"); // Store value
			int j;
			
			// Record connections
			if(strncmp(key, "CONNECTION", strlen("CONNECTION")) == 0)
			{
				for(j = 0; j < ROOM_LIST_MAX; j++)
				{
					if(strncmp(val, rmArr[j].name, strlen(rmArr[i].name)) == 0)
					{
						rmArr[i].outboundConxs[rmArr[i].cnxCount] = j; // save index of new outbound conx
						rmArr[i].cnxCount++; // increment cnx count
					}
				}
			}  // Record Room Type as an ENUM
			else if(strncmp(key, "ROOM TYPE", strlen("ROOM TYPE")) == 0)
			{
				if(strncmp(val, "MID_ROOM", strlen("MID_ROOM")) == 0)
				{
					rmArr[i].rType = MID_ROOM;
				}
				else if (strncmp(val, "START_ROOM", strlen("START_ROOM")) == 0)
				{
					rmArr[i].rType = START_ROOM;
				}
				else if(strncmp(val, "END_ROOM", strlen("END_ROOM")) == 0)
				{
					rmArr[i].rType = END_ROOM;
				}
			}
		}
		fclose(currFile); // Close file
	}
    closedir(workingDir); // Close directory
	free(latestDir);      // Free space allocated by findNewDir()
}

/****************************************************************************
 *Function Name: dispCurLoc
 *Description: This function displays the current room, as well as,
 	adjacent rooms available to travel to from the current room.
****************************************************************************/
void dispCurLoc(struct room* rmArr, int currLoc)
{
	int i;
	
	printf("CURRENT LOCATION: %s\n", rmArr[currLoc].name);
	fflush(NULL);
	printf("POSSIBLE CONNECTIONS: ");
	fflush(NULL);
	
	// Print connections
	for(i = 0; i < rmArr[currLoc].cnxCount; i++)
	{
		if(i != rmArr[currLoc].cnxCount -1)
		{
			printf("%s, ", rmArr[rmArr[currLoc].outboundConxs[i]].name);
			fflush(NULL);
		}
		else // Print last connection followed by period
		{
			printf("%s.\n", rmArr[rmArr[currLoc].outboundConxs[i]].name);
			fflush(NULL);
		}
	}
}

/*******************************************************************************
 *Function Name: initRoomList
 *Description: This function initializes the array of room structs prior to
 	populating the room array with data read in from the most recent rooms
 	directory.
***************************sss*************************************************/
void initRoomList(struct room* rmArr)
{
	int i, j;
	
	for(i = 0; i < ROOM_LIST_MAX; i++)
	{
		struct room newRoom;
		newRoom.rmId = i;
		memset(newRoom.name, '\0', sizeof(newRoom.name));
		newRoom.rType = START_ROOM;
		newRoom.cnxCount = 0;
		for(j = 0; j < MAX_CONX; j++)
		{
			newRoom.outboundConxs[j] = 0;
		}
		rmArr[i] = newRoom;
	}
}

/****************************************************************************
 *Function Name: playGame
 *Description: This function serves as the foundation of the game. It calls
 	on numerous auxiallary functions to flesh out the whole of the game. The
 	game concludes when the player reaches the end room.
****************************************************************************/
void playGame()
{
	// Lock mutex for time file
	pthread_mutex_lock(&timeFileLock);
	
	// Array of rooms to hold data from files
	struct room roomList[ROOM_LIST_MAX];
	// Initialize roomList
	initRoomList(roomList);

	// Convert File Data to roomList
	readInData(roomList);
	
	int stepCount = 0; // Counter to record the number of steps taken
	int i;
	int currLoc = getStartLoc(roomList);; // Tracks the current location of the player
	// Create linked list stack to track steps
	struct linkList* pathTaken = createDeque();
	//listAddBack(pathTaken, currLoc); // Omitted (in the example given, the start room was NOT included)
	
	// Continue game till over
	while(!isGameOver(roomList[currLoc]))
	{
		// Display current room
		dispCurLoc(roomList, currLoc);
		
		// Get user input
		int cnxIndexToCheck = 0;
		char* userInput = getInput();
		
		int validInput = 0; // acts as flag for valid input
		
		// Check to see if time was requested
		while(strncmp(userInput, "time", strlen("time")) == 0)
		{
			pthread_mutex_unlock(&timeFileLock); // main thread calls unlock
			// Create a time thread (i.e. write the new time to time file)
			pthread_create(&timeThread, NULL, writeTime, NULL);
			//Block main thread until done
			pthread_join(timeThread, NULL);
			pthread_mutex_lock(&timeFileLock); // lock back down to Main Thread
			displayTime(); // Print current time to user
			free (userInput); // Free memory allocated for previous input
			userInput = getInput();
		}

		// Find the room to connect move to (destination room)
		for(i = 0; i < roomList[currLoc].cnxCount; i++)
		{
			cnxIndexToCheck = roomList[currLoc].outboundConxs[i];
			if(strncmp(userInput, roomList[cnxIndexToCheck].name, strlen(roomList[cnxIndexToCheck].name)) == 0)
			{
				currLoc = roomList[cnxIndexToCheck].rmId; // index of destination room
				stepCount++; // Increment step count
				validInput = 1; // No need to print error message
				listAddBack(pathTaken, roomList[cnxIndexToCheck].rmId); // add index of room to path
			}
		}
		
		if(!validInput)
		{
			printf("HUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
			fflush(stdout);
		}
		
		// Free user input
		free (userInput);
	}
	
	// Print congratulatory message when loop breaks
	printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
	printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", stepCount);
	printPath(roomList, pathTaken); // Print path taken
	
	// Unlock mutex for time file
	pthread_mutex_unlock(&timeFileLock);
	
	// Free deque
	freeList(pathTaken);
}

/////// START: LINKED LIST FUNCTIONS ////////////////////////////////////////
/****************************************************************************
 *Function Name: createList
 *Description: This function creates and returns a linked list deque data
 structure. The calling function is responsible for freeing the memory.
 ****************************************************************************/
struct linkList* createDeque()
{
	// Dynamically allocate memory
	struct linkList* list = malloc(sizeof(struct linkList));
	list->listFront = malloc(sizeof(struct link));
	list->listBack = malloc(sizeof(struct link));
	
	// Initialize Sentinels
	list->listFront->next = list->listBack;
	list->listBack->prev = list->listFront;
	
	//Initialize list Size (i.e. # of links holding values (not sentinels))
	list->size = 0;
	
	return list;
}

/****************************************************************************
 *Function Name: listIsEmpty
 *Description: returns true (1) if list is empty or false (0) otherwise.
 ****************************************************************************/
int listIsEmpty(struct linkList* list)
{
	return (list->size == 0);
}

/****************************************************************************
 *Function Name: listAddBack
 *Description: This function takes a pointer to a linked list and a value
 to be added to the back of the linked list.
 ****************************************************************************/
void listAddBack(struct linkList * list, TYPE val)
{
	struct link* newLink = malloc(sizeof(struct link));
	newLink->value = val;
	newLink->next = list->listBack; // point to back sentinel
	newLink->prev = list->listBack->prev; // point to penultimate link
	list->listBack->prev->next = newLink; // former last link points to new link
	list->listBack->prev = newLink; // back sentinel points to new link
	list->size++; // increment list size
}

/****************************************************************************
 *Function Name: listAddFront
 *Description: This function takes a pointer to a linked list and a value
 to be added to the front of the linked list.
 ****************************************************************************/
void listAddFront(struct linkList *list, TYPE val)
{
	struct link* newLink = malloc(sizeof(struct link));
	newLink->value = val;
	newLink->next = list->listFront->next; // point to former first link
	newLink->prev = list->listFront; // point to front sentinel
	list->listFront->next->prev = newLink; // point fomer first link prev to new link
	list->listFront->next = newLink; // point front sentinel to new link
	list->size++; // increment size
}

/****************************************************************************
 *Function Name: listRemoveBack
 *Description: This function removes the last value holding link in the
 linked list.
 ****************************************************************************/
void listRemoveBack(struct linkList* list)
{
	if(!listIsEmpty(list)) // Ensure list is not empty
	{
		struct link* garbage = list->listBack->prev; // select node to be removed
		list->listBack->prev = garbage->prev; // point back sentinel to new last link
		garbage->prev->next = list->listBack; // point new last link to back sentinel
		free(garbage); // free allocated memory
		list->size--; // decrement list size
	}
}

/****************************************************************************
 *Function Name: listRemoveFront
 *Description: This function removes the first value holding link in the
 linked list.
 ****************************************************************************/
void listRemoveFront(struct linkList* list)
{
	struct link* garbage = list->listFront->next; // select node to be removed
	list->listFront->next = garbage->next; // point front sentinel to new front link
	garbage->next->prev = list->listFront; // point new front link to front sentinel
	free(garbage); // free allocated memory
	list->size--; // decrement list size
}

/****************************************************************************
 *Function Name: listFront
 *Description: Returns the value held by the first link in the list.
 ****************************************************************************/
TYPE listFront(struct linkList* list)
{
	return list->listFront->next->value;
}

/****************************************************************************
 *Function Name: listBack
 *Description: Returns the value held by the last link in the list.
 ****************************************************************************/
TYPE listBack(struct linkList* list)
{
	return list->listBack->prev->value;
}

/****************************************************************************
 *Function Name: freeList
 *Description: This function frees the memory allocated for the list and all
 of the links therein.
 ****************************************************************************/
void freeList(struct linkList* list)
{
	if((list != NULL) && (!listIsEmpty(list)))
	{
		while (list->size > 0)
		{
			listRemoveFront(list);
		}
		free(list->listBack);
		free(list->listFront);
		free(list);
	}
}
/////// END: LINKED LIST FUNCTIONS ////////////////////////////////////////

