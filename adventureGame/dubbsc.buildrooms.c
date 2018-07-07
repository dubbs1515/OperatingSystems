/****************************************************************************
 *Program Name: buildrooms
 *Author: Christopher Dubbs
 *Date: January 30, 2018
 *Course: CS 344
 *Description: This program creates a series of files that hold descriptions
	 of the in-game rooms and how the rooms are connected. The program first
	 generates 7 different room files, 1 room per file, in a directory
	 called dubbsc.rooms.<PROCESS ID>". Each room has a room name, 3 to 6
	 outgoing connections (where the # of outgoing connections is random) to
	 other rooms, and a room type. The connections among rooms are randomly
	 assigned (if room A connects to B, then B connects to A). Rooms do not
	 connect to themselves. Rooms do not connect to other rooms more than
	 once. The program randomly assings one of 10 specified room names to
	 each room generated (mutually exclusive).
 *Notes: Partially uses the outline provided in the course materials:
	 "2.2 Program Outlining in Program 2"
****************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>  // For mkdir()
#include <sys/types.h>  // For getpid()
#include <unistd.h>    // For getpid()

#define MIN_CONX 3
#define MAX_CONX 6
#define ROOM_LIST_MAX 7
#define NUM_ROOM_NAMES 10

// enum to represent room types
enum ROOM_TYPE { START_ROOM, MID_ROOM, END_ROOM };

//ROOM STRUCT
struct room
{
	int rmId; // i.e. index of name in r_names
	char name[9];  // (i.e. index of one of ten possible room names)
	enum ROOM_TYPE rType;  // (i.e. START_ROOM (0), MID_ROOM (1), or END_ROOM (2))
	int outboundConxs[MAX_CONX]; // Holds indexes of other rooms in roomList
	int cnxCount; // Tracks total # of outbound connections crrently assigned
};

/****************************************************************************
 *Function Name: isGraphFull
 *Description: This function returns true is all rooms have 3 to 6
	 outbound connections, and false otherwise (i.e 0 if false and 1 if
	 true).
****************************************************************************/
int isGraphFull(struct room* roomArr)
{
	int isFull = 1; // acts as bool to be returned
	int i; // counter
	for(i = 0; i < ROOM_LIST_MAX; i++) // Verify each room has >= 3 cnxs
	{
		if(roomArr[i].cnxCount < 3)
		{
			isFull = 0;      // Break if any room has < 3 cnxs
			break;
		}
	}
	return isFull;
}

/****************************************************************************
 *Function Name: randInRange
 *Description: returns a number within the range of passed upperbound (int)
 and 0.
 ****************************************************************************/
int randInRange(int upperBound)
{
	return (rand() % upperBound);
}

/****************************************************************************
 *Function Name: connectRoom
 *Description: This function connects the two rooms passed together without
 verifying the connection to be valid.
 ****************************************************************************/
void connectRoom(struct room* roomArr, int rm1, int  rm2)
{
	roomArr[rm1].outboundConxs[roomArr[rm1].cnxCount] = roomArr[rm2].rmId;
	roomArr[rm1].cnxCount++;
	roomArr[rm2].outboundConxs[roomArr[rm2].cnxCount] = roomArr[rm1].rmId;
	roomArr[rm2].cnxCount++;
}

/****************************************************************************
 *Function Name: conxExists
 *Description: This function returns true (1) if a connection from rm1 to
 rm2 already exists, and otherwise false (0).
 ****************************************************************************/
int conxExists(struct room* roomArr, int rm1, int rm2)
{
	int connected = 0; // acts as bool to be returned
	int i; // counter
	
	// Verifies no outbound conx from rm1 to rm2 already assigned
	for( i = 0; i < roomArr[rm1].cnxCount; i++)
	{
		if(roomArr[rm1].outboundConxs[i] == roomArr[rm2].rmId)
		{
			connected = 1;
			break;
		}
	}
	
	return connected;
}

/****************************************************************************
 *Function Name: isSameRoom
 *Description: This function returns 1 if the the rooms are the same.
 ****************************************************************************/
int isSameRoom(struct room* roomArr, int rm1, int rm2)
{
	return (roomArr[rm1].rmId == roomArr[rm2].rmId);
}

/****************************************************************************
 *Function Name: addRdmConx
 *Description: This function adds a random, valid outbound connection from
	 one room to another room.
****************************************************************************/
void addRdmConx(struct room* roomArr)
{
	
	int rm1;
	int rm2;
	
	while(1)
	{
		// Get random room
		rm1 = randInRange(ROOM_LIST_MAX);
		
		// Ensure connections not maxed out
		if( roomArr[rm1].cnxCount < 6 )
		{
			break;
		}
	}

	// Get 2nd random room to establish additional coonection
	do
	{
		rm2 = randInRange(ROOM_LIST_MAX);
	} while((roomArr[rm2].cnxCount > 5) || ((isSameRoom(roomArr, rm1, rm2)) == 1) || ((conxExists(roomArr, rm1, rm2)) == 1));
	
	// Connect the rooms
	connectRoom(roomArr, rm1, rm2);
}

/****************************************************************************
 *Function Name: initRoomList
 *Description: This function populates a new list of rooms to be used. It
	does not assign the connections, which are subsequently randomly
	assgined. It does assign unique room names. It also assigns the room
 	of each room.
****************************************************************************/
void initRoomList(struct room* roomArr, char rm_names[][9])
{
	
	int i; // counter variable

	int idxNamesUsed[ROOM_LIST_MAX] = { -1 };   // Holds indices of names already assigned
	int nameIdx; // Holds a randomly generated index to map to a name in r_names array
	int numNamesUsedIdx = 0; // Tracks the number of r_names indices already used
	
	// Create 7 rooms with random names and of type mid room
	for( i = 0; i < ROOM_LIST_MAX; i++)
	{
		numNamesUsedIdx++;  // Increment the count of used indices of r_names array
		int nameTaken = 1;          // Flag for unique name assignment
		struct room newRoom;        // Declare new room
		
		while(nameTaken) //Ensure an unused name index is generated
		{
			nameIdx = randInRange(NUM_ROOM_NAMES); // Generate a random index within the range
			int currIdx = 0; // Track position in array holding previously used values
			
			// Iterate through all values currently in used index array
			while(currIdx < numNamesUsedIdx)
			{
				if(nameIdx != idxNamesUsed[currIdx])
				{
					nameTaken = 0; // So far name is avaiable
				} else {
					nameTaken = 1; // Name taken try again
					break;
				}
				currIdx++;
			}
		}
		
		// Create additional room
		memset(newRoom.name, '\0', 9); // Intialize new room name with nulls
		strcpy(newRoom.name, rm_names[nameIdx]);	    // Assign name
		newRoom.rmId = nameIdx;
		memset(newRoom.outboundConxs, '\0', 6); // Intialize outbound conx
		newRoom.rType = MID_ROOM;   // Assign type
		
		newRoom.cnxCount = 0;		// Set count to 0
		roomArr[i] = newRoom;      // Add new room to list of rooms
		
		idxNamesUsed[numNamesUsedIdx - 1] = nameIdx; // Add index used to used index array
	}
	
	// Randomly assign one room as the start room
	int startRm = randInRange(ROOM_LIST_MAX);
	roomArr[startRm].rType = START_ROOM;
	
	// Randomly assign one room as the end room
	int isStartRoom = 1; // Can't make start room end room (bool)
	int endRm;
	
	while(isStartRoom) // Find a room aside from the of the start room
	{
		endRm = randInRange(ROOM_LIST_MAX);
		if( endRm != startRm)
		{
			isStartRoom = 0;
		}
	}
	roomArr[endRm].rType = END_ROOM;
}

/****************************************************************************
 *Function Name: createRmDir
 *Description: This function creates a room directory.
 ****************************************************************************/
void createRmDir(struct room* rmArr, char rm_names[][9])
{
	FILE *rmFile;       // For generating room files, to hold file pointers
	char dirName[256];  // For creating a directory name
	int dirPerm = 0755; // Directory permissions
	int i, j;           // Counters

	// Create directory name using getpid to append unique identifier
	sprintf(dirName, "dubbsc.rooms.%d", getpid());
	//printf("\n\ndirectory name: %s\n\n", dirName);
	
	// Create directory and check for error
	if((mkdir(dirName, dirPerm)) != 0)
	{
		perror("\nDirectory Creation Unsuccessful\n\n");
	}
	
	// Create a file for each room in the new directory
	for(i = 0; i < ROOM_LIST_MAX; i++)
	{
		char charBuffer[256] = { '\0' }; // To format file location
		sprintf(charBuffer, "%s/%s", dirName, rmArr[i].name);
		
		// Open a new file for writing
		rmFile = fopen(charBuffer, "w");
		
		// Print room name to file
		fprintf(rmFile, "ROOM NAME: %s\n", rmArr[i].name);
		
		// Print room outbound connections to file
		for(j = 0; j < rmArr[i].cnxCount; j++)
		{
			fprintf(rmFile, "CONNECTION %d: %s\n", j + 1, rm_names[rmArr[i].outboundConxs[j]]);
		}
		
		// Print room type to file
		switch (rmArr[i].rType)
		{
			case START_ROOM:
				fprintf(rmFile, "ROOM TYPE: %s\n", "START_ROOM");
				break;
			case MID_ROOM:
				fprintf(rmFile, "ROOM TYPE: %s\n", "MID_ROOM");
				break;
			case END_ROOM:
				fprintf(rmFile, "ROOM TYPE: %s\n", "END_ROOM");
				break;
			default:
				fprintf(rmFile, "INVALID");
				break;
		}
		fclose(rmFile); // Close the file}
	}
}

/******************s**********************************************************
 MAIN
****************************************************************************/

int main()
{
	// Possible room names
	char r_names[10][9] = { "MainLab", "Airlock", "Quarters", "MiniSub", "FaunaLab", "Galley", "Library", "Head", "FaunaLab", "RecRoom" };
	
	// Array of rooms to be used for file generation
	struct room roomList[ROOM_LIST_MAX];
	
	// Seed for srand function
	srand(time(0));
	
	// Generate Rooms
	initRoomList(roomList, r_names);
	
	// Create Graph (add connections)
	while (!isGraphFull(roomList))
	{
		addRdmConx(roomList);
	}

	createRmDir(roomList, r_names);
	
	return 0;
}
