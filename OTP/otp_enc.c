/**********************************************************************************
 Program Name: opt_enc
 Author: Christopher Dubbs
 Class: CS344
 Date: March 1, 2018
 Description: This program connects to otp_enc_d, and asks it to perform a one-time
     pad style encryption. By itself, otp_enc does not perform encryption. The
     syntax of otp_enc is: otp_enc plaintext <key> <port>
     Plaintext is the name of a file in the current directory that contains the
     plaintext to be encrypted. Key contains the encryption key to be used to
     encrypt the text. Finally, port is the port that otp_enc should attempt to
     connect to otp_enc_d. When otp_enc receives the ciphertext back from
     otp_enc_d, it outputs it to stdout. If otp_enc receives any key or plaintext
     files with any bad characters, or if the key file is < than the plaintext,
     it will terminate. In such a case, in addition it sends appropriate error text
     to stderr, and sets the exit value to 1. otp_enc CANNOT connect to
     otp_dec_d. In the case of attempting to do so, otp_enc reports the rejection
     to stderr and terminates itself. Moreover, if otp_enc cannot connect to
     otp_enc_d for any reason (including the prev case) it reports the error to
     stderr with the attempted port, and sets the exit value to 2. Conversly, upon
     successfully running and terminating, otp_enc sets the exit value to 0.
 Reference Citation: Kernighan & Ritchie, "The C Programming Language", ISBN: 0131103628
 Reference Citation: "The Linux Programming Interface", Michael Kerrisk, ISBN: 9781593272203
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
void validateArgs(int argCount);
void validateKeyLen(int fileLen, int keyLen);
char* readFileIn(char* fileName, int fSize);
int findFileSize(char* fileName);
int createSock(int servPort);
void sendData(int sockfd, char* dataToSend, int size);
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
    
    // Validate number of arguments
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
    cliType = 'E';
    sendData(servSockfd, &cliType, sizeof(char));
    recv(servSockfd, &permToConnect, sizeof(char), 0);
    if(permToConnect == 'N') {      // Report error if connection denied
        close(servSockfd);
        fprintf(stderr, "Cannot connect to server (Not an encryption server)\n");
        exit(2);
    }
    
    // Send file length, data, and key to server
    send(servSockfd, &fileLen, sizeof(fileLen), 0);   // Send inication of file length
    sendData(servSockfd, fileContents, fileLen * sizeof(char)); // Send file
    sendData(servSockfd, keyContents, fileLen * sizeof(char));  // Send key
    
    
    // Receive encrypted text
    char* cipherContents = malloc(fileLen * sizeof(char));  // Allocate memory to hold text
    recvAll(servSockfd, cipherContents, fileLen * sizeof(char));
    
    // Output text to stdout
    printf("%s\n", cipherContents);
    
    // Clean up
    close(servSockfd);
    free(cipherContents);
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
    if(argCount != 4)
    {
        fprintf(stderr, "Please, provide <file> <key> <port> arguments.\n");
        exit(1);
    }
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
            fprintf(stderr, "otp_enc error: input contains bad characters.\n");
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
 * Function Name: validateKey Len
 * Description: This function is used to verify that key file is not shorter than the
    plaintext file.
 *********************************************************************************/
void validateKeyLen(int fileLen, int keyLen)
{
    if(keyLen < fileLen)    // key must be at least as long as file
    {
        fprintf(stderr, "Please, ensure that the key file is not shorter than the plaintext file.\n");
        exit(1);
    }
}


/**********************************************************************************
 * Function Name: findFileSize
 * Description: This function is used to determine the size in bytes of a given file.
     Reference Citation: https://linux.die.net/man/2/stat
 *********************************************************************************/
int findFileSize(char* fileName)
{
    // Determine file size
    struct stat st;
    stat((char*) fileName, &st);
    return ((int) st.st_size - 1);  // Return size (less newline)
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
        fprintf(stderr, "Error from otp_enc socket() system call");
        exit(2);
    }
    
    // Citation: "The Linux Programming Interface", Michael Kerrisk, ISBN: 9781593272203 pages 1278 - 1280
    // Set SO_REUSEADDR Socket Option to avoid "Address already in use" error
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);
    
    // Connect socket
    if(connect(sockfd, (struct sockaddr *) &servAddress, sizeof(servAddress)) == -1)
    {
        fprintf(stderr, "Error: could not contact otp_enc_d on port %d\n", servPort);
        exit(2);
    }
    
    return sockfd;
}


/****************************************************************************
 * Function Name: sendData
 * Description: This function is used to send data to peer via the given
     file descriptor argument.
 * Reference Citation: http://beej.us/guide/bgnet/html/multi/advanced.html#sendall
 * Reference Citation: https://oregonstate.instructure.com/courses/1662153/pages/4-dot-2-verified-sending
 ****************************************************************************/
void sendData(int sockfd, char* dataToSend, int size)
{
    int bytesSent = 0, chkSend = -7;
    
    bytesSent = send(sockfd, dataToSend, size, 0);    // Send message to client
    if(bytesSent < 0) { fprintf(stderr, "Error sending to server.\n"); }
    
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

