/**********************************************************************************
 Program Name: otp_dec
 Author: Christopher Dubbs
 Class: CS344
 Date: March 1, 2018
 Description: This program connects to otp_dec_d and asks it to decrypt ciphertext
     using the passed ciphertext and key, and otherwise performs exactly like
     otp_enc. It is runnable in the same 3 manners as otp_enc. otp_dec
     is NOT able to connect to otp_enc_d. 
 *********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>


#define ASCII_CAP_MAX 90
#define ASCII_CAP_MIN 65

// Function Prototypes
void sendData(int sockfd, char* dataToSend, int size);
void validateArgs(int argCount);
int findFileSize(char* fileName);
void validateKeyLen(int fileLen, int keyLen);
char* readFileIn(char* fileName, int fSize);
int createSock(int servPort);
void recvAll(int sockfd, char* msgBuff, int len);


/***********************************************************************
 * MAIN
 **********************************************************************/
int main(int argc, const char * argv[]) {
    char* fileName;
    char* keyName;
    char* fileContents;
    char* keyContents;
    int fileLen;
    int keyLen;
    int servSockfd;
    int servPort;
    char cliType, permToConnect;
    
    // Save command line values
    fileName = (char *) argv[1];
    keyName = (char *) argv[2];
    servPort = atoi(argv[3]);
    
    // Validate arguments
    validateArgs(argc);
    
    // Determine size of each file
    fileLen = findFileSize(fileName);
    keyLen = findFileSize(keyName);
    
    // Ensure valid file sizes
    validateKeyLen(fileLen, keyLen);
    
    // Read in plaintext file
    fileContents = readFileIn(fileName, fileLen);
    
    // Read in key
    keyContents = readFileIn(keyName, keyLen);
    
    // Setup socket
    servSockfd = createSock(servPort);
    
    // Verify permission to connect
    cliType = 'D';          // Idenitify as decryption type 'D' (otp_dec)
    sendData(servSockfd, &cliType, sizeof(char));
    recv(servSockfd, &permToConnect, sizeof(char), 0);  // Receive notification of permission
    if(permToConnect == 'N') {  // Print notice of rejection
        close(servSockfd);
        fprintf(stderr, "Cannot connect to server (Not a decryption server)\n");
        exit(2);
    }
    
    // Send file length, data, and key to server
    send(servSockfd, &fileLen, sizeof(fileLen), 0); // Send inidication of file lenght
    sendData(servSockfd, fileContents, fileLen * sizeof(char)); // Send file
    sendData(servSockfd, keyContents, fileLen * sizeof(char));  // Send key (length of file)
    
    // Receive decrypted text
    char* decryptedText = malloc(fileLen * sizeof(char));  // Allocate memory for incoming text
    recvAll(servSockfd, decryptedText, fileLen * sizeof(char));
    
    // Output plaintext to stdout
    printf("%s\n", decryptedText);
    
    // Clean up
    close(servSockfd);
    free(decryptedText);
    free(fileContents);
    free(keyContents);
    close(servSockfd);

    return 0;
}



/**********************************************************************************
 * Function Name: validateArgs
 * Description: This function insures a valid number of arguments were received
     from the command line.
 *********************************************************************************/
void validateArgs(int argCount)
{
    if(argCount != 4)   // Must have all three needed parameters
    {
        fprintf(stderr, "Please, provide <file> <key> <port> arguments.\n");
        exit(1);
    }
}


/**********************************************************************************
 * Function Name: findFileSize
 * Description: This function is to determine the size in bytes of a given file.
 * Reference Citation: https://linux.die.net/man/2/stat
 *********************************************************************************/
int findFileSize(char* fileName)
{
    // Determine file size
    struct stat st;
    stat((char*) fileName, &st);
    return ((int) st.st_size - 1);  // Return size (stripping off newline)
}


/**********************************************************************************
 * Function Name: readFileIn
 * Description: This function is used to read in the contents of a specified file.
     It returns a character string containing the files contents. The calling
     function is responsible for freeing the memory allocated.
 * Reference Citation: Kernighan & Ritchie, "The C Programming Language", ISBN: 0131103628
 * Reference Citation: http://man7.org/linux/man-pages/man2/stat.2.html
 *********************************************************************************/
char* readFileIn(char* fileName, int fSize)
{
    char* fileContents;     // Holds file contents
    char nextChar;
    int i;
    //int fileSize;
    int stringPostion = 0;
    
    
    fileContents = calloc(fSize, sizeof(char));  // Allocate memory to hold file contents
    
    FILE* fd = fopen(fileName, "r");  // Open file for reading
    
    
    // Copy characters to string
    for(i = 0; i < fSize; i++)
    {
        nextChar = getc(fd);    // Get next character
        
        // Check for bad character
        if(!(nextChar == 32 || (nextChar >= ASCII_CAP_MIN && nextChar <= ASCII_CAP_MAX)))
        {
            fprintf(stderr, "Sorry, the file provided contains an invalid character.\n");
            exit(1);
        }
        else    // Add next character to fileContents
        {
            fileContents[stringPostion] = nextChar;
        }
        stringPostion++;    // Increment position
    }
    
    return fileContents;
}


/**********************************************************************************
 * Function Name: createSock
 * Description: This function is used to setup a client socket on the
    port given by the command line argument to communicate with a server.
 * Reference Citation: http://beej.us/guide/bgnet/html/single/bgnet.html
    and proivided class code and notes.
 *********************************************************************************/
int createSock(int servPort)
{
    int sockfd;
    int optval = 1;
    socklen_t optlen = sizeof(optval);
    struct hostent* servInfo;
    struct sockaddr_in servAddress;
    
    // Setup server info
    servAddress.sin_family = AF_INET;   // IPv4
    servAddress.sin_port = htons(servPort);     // Store port (converting to big endian)
    servInfo = gethostbyname("localhost");      // local host target
    memcpy((char *) &servAddress.sin_addr.s_addr, (char *) servInfo->h_addr_list[0], servInfo->h_length);
    
    // Create a new socket (IPv4, connection-oriented)
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "Error from otp_dec socket() system call");
        exit(2);
    }
    
    // Citation: "The Linux Programming Interface", Michael Kerrisk, ISBN: 9781593272203 pages 1278 - 1280
    // Set SO_REUSEADDR Socket Option to avoid "Address already in use" error
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);
    
    // Connect socket
    if(connect(sockfd, (struct sockaddr *) &servAddress, sizeof(servAddress)) == -1)
    {
        fprintf(stderr, "Error: cannot connect to otp_dec_d server on port %d", servPort);
        exit(2);
    }
    
    return sockfd;
}


/**********************************************************************************
 * Function Name: validateKeyLen
 * Description: This function is used to verify that key file is not shorter than the
     plaintext file.
 *********************************************************************************/
void validateKeyLen(int fileLen, int keyLen)
{
    if(keyLen < fileLen)    // Key must be at least as long as file
    {
        fprintf(stderr, "Please, ensure that the key file is not shorter than the plaintext file.\n");
        exit(1);
    }
}


/****************************************************************************
 * Function Name: sendData
 * Description: This function is used to send data to peer via the given
     file descriptor argument.
 *Reference Citation: http://beej.us/guide/bgnet/html/multi/advanced.html#sendall
 *Reference Citation: https://oregonstate.instructure.com/courses/1662153/pages/4-dot-2-verified-sending
 ****************************************************************************/
void sendData(int sockfd, char* dataToSend, int size)
{
    int bytesSent = 0, chkSend = -7;
    
    bytesSent = send(sockfd, dataToSend, size, 0);    // Send message to client
    if(bytesSent < 0) { fprintf(stderr, "Error sending to client.\n"); }
    
    do
    {
        ioctl(sockfd, TIOCOUTQ, &chkSend);  // Check socket send buffer
    } while(chkSend > 0);
    // Check for error
    if(chkSend < 0) { fprintf(stderr, "Error from ioctl.\n"); }
}


/***********************************************************************
 * Function Name: recvAll
 * Description: This function is used to ensure that all expected
     bytes are received. The function loops to until the required number
     recv() calls receives the cumulative total of expected bytes.
 * Reference Citation: http://man7.org/linux/man-pages/man2/recv.2.html
 **********************************************************************/
void recvAll(int sockfd, char* msgBuff, int len)
{
    int bytesToRecv = len;      // Expected  bytes to be received
    int bytesRcvd;              // Bytes read by last recv() call
    
    // Loop until all bytes are received or an error occurs
    while(bytesToRecv > 0 && (bytesRcvd = recv(sockfd, msgBuff, len, 0)) > 0) {
        msgBuff += bytesRcvd;       // Add bytes to cumulative buffer
        bytesToRecv -= bytesRcvd;   // Adjust the # of bytes still expected
    }
}










