/**********************************************************************************
 Program Name: otp_dec_d
 Author: Christopher Dubbs
 Class: CS344
 Date: March 1, 2018
 Description: This program performs exactly like otp_enc_d in syntax and usage.
     Except it serves to decrypt the given ciphertext, using the passed ciphertext
     and key. Thus, it returns plaintext to otp_dec.
 *********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <netinet/in.h>


#define ASCII_SPACE 32
#define ASCII_MIN   64  // '@' to represent space
#define ASCII_MAX   90  // 'Z'


// Function Prototypes
void runServ(int portNum);
int createServSock(int servPort);
void decryptData(char* data, char* key, char* decryptBuff);
void sendData(int sockfd, char* dataToSend, int len);
void recvAll(int sockfd, char* msgBuff, int len);
void handleSIGCHLD(int signal);


/***********************************************************************
 * MAIN
 **********************************************************************/
int main(int argc, const char * argv[]) {
    
    // Ensure valid number of command line arguments
    if(argc != 2)
    {
        fprintf(stderr, "Please include a port number argument.\n");
        exit(1);
    }
    
    runServ(atoi(argv[1]));
    
    return 0;
}


/***********************************************************************
 * Function Name: runServ()
 * Description: This function implements a server to handle
    decryption requests from otp_dec clients. It first verifies a
    connecting clinet's type (must be otp_dec). Assuming the client
    is of the valid type, the function spawns a new child to handle
    the client's decryption request.
 **********************************************************************/
void runServ(int portNum)
{
    char fileBuff[80000];
    char keyBuff[80000];
    int servSock, newClient, fileLen;
    char cliType, permToConnect;
    char* decryptText;
    struct sockaddr_in cliAddress;
    socklen_t cliAddressSize = sizeof(cliAddress);
    
    // Setup server socket
    servSock = createServSock(portNum);
    
    // Setup SIGCHLD handler to handle zombies
    struct sigaction SIGCHLD_action;    // Declare sigaction struct for SIGCHLD
    SIGCHLD_action.sa_handler = handleSIGCHLD;  // Assign fx to be called when SIGCHLD received
    SIGCHLD_action.sa_flags = SA_RESTART;       // To handle interrupted system calls
    sigaction(SIGCHLD, &SIGCHLD_action, NULL);   // Register signal handler
    
    while(1)
    {
        // Accept next client
        newClient = accept(servSock, (struct sockaddr*) &cliAddress, &cliAddressSize);
        
        pid_t spawnPid = fork();    // Spawn process to handle client request
        
        switch(spawnPid)
        {
            case -1:                // ERROR CASE
                fprintf(stderr, "Error from fork() call.\n");
                exit(1);
                break;
            case 0:                 // CHILD
                // Verify valid client type (must be otp_dec)
                recv(newClient, &cliType, sizeof(char), 0);  // Receive's char identifying client type
                if(cliType != 'D') {            // If not otp_enc ('D'), cannot connect
                    permToConnect = 'N';        // Send notice of decline
                    send(newClient, &permToConnect, sizeof(char), 0);
                    //fprintf(stderr, "Invalid client type (otp_dec only).\n");
                }
                else    // Client is allowed to connect
                {
                    permToConnect = 'Y';    // Send notice of permission
                    send(newClient, &permToConnect, sizeof(char), 0);
                }
                
                // Get indication of data size to be transferred
                recv(newClient, &fileLen, sizeof(fileLen), 0);
                
                // Reset data buffers
                memset(fileBuff, 0, sizeof(fileBuff));
                memset(keyBuff, 0, sizeof(keyBuff));
                
                // Receive file and key per size indicated above
                recvAll(newClient, fileBuff, fileLen * sizeof(char));
                recvAll(newClient, keyBuff, fileLen * sizeof(char));
                
                // Allocate memory and decrypt file
                decryptText = calloc(sizeof(fileBuff), sizeof(char));
                decryptData(fileBuff, keyBuff, decryptText);
                
                // Send plaintext back to client
                sendData(newClient, decryptText, fileLen * sizeof(char));
                
                free(decryptText);  // Free allocated memory
                exit(0);
                break;
            default:        //PARENT (Let's child do the work, parent listens for next conx)
            
                break;
        }
        close(newClient);   // Close connection
    }
}


/***********************************************************************
 * Function Name: decryptData
 * Description: This function takes a message to decrypt and a key, and
    returns the decrypted version of the message. The calling function
    is responsible for freeing the allocated memory.
 **********************************************************************/
void decryptData(char* data, char* key, char* decryptBuff)
{
    //char* decryptedData = calloc(sizeof(data), sizeof(char));
    char decryptedChar;
    int i;
    
    for(i = 0; i < strlen(data); i++)   // For each character
    {
        
        if(data[i] == ASCII_SPACE) {
            data[i] = ASCII_MIN;   // Set space to '@' for easier manipulation
        }
        if(key[i] == ASCII_SPACE) {
            key[i] = ASCII_MIN;    // Set space to '@' for easier manipulation
        }
        // Manipulate ASCII values for calculation (therefore, vals b/n 0 and 26)
        int nxtDataChar = (int) data[i] - ASCII_MIN;
        int nxtKeyChar = (int) key[i] - ASCII_MIN;

        // Perform decryption calculation (adding 27 to handle possible negative)
        if((decryptedChar = (nxtDataChar - nxtKeyChar)) % 27 < 0){
            decryptedChar += 27;
        }
        
        decryptedChar += 64;    // Convert back to corresponding ASCII value
        decryptBuff[i] = (char) decryptedChar;    // Cast and add to decrypted string
        
        // Convert '@' back to ' ' if applicable
        if(decryptBuff[i] == '@') {
            decryptBuff[i] = ASCII_SPACE;
        }
    }
}


/***********************************************************************
 * Function Name: createServSock
 * Description: This function is used to setup a server socket on the
    port given by the command line argument.
 * Reference Citation: http://beej.us/guide/bgnet/html/single/bgnet.html
    and proivided class code and notes.
 **********************************************************************/
int createServSock(int servPort)
{
    int servSock;
    struct sockaddr_in servAddress;
    int optval = 1;
    socklen_t optlen = sizeof(optval);
    
    // Fill Socket Address Struct
    servAddress.sin_family = AF_INET;               // Address family for IPv4
    servAddress.sin_port = htons(servPort);         // host to network conversion
    servAddress.sin_addr.s_addr = INADDR_ANY;       // Any address allowed to connect
    
    // Create a new socket (passive) (IPv4 and connection-oriented)
    if((servSock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Error from otp_dec_d socket() system call.\n");
        exit(1);
    }
    
    // Citation: "The Linux Programming Interface", Michael Kerrisk, ISBN: 9781593272203 pages 1278 - 1280
    // Set SO_REUSEADDR Socket Option to avoid "Address already in use" error
    setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);
    
    // Bind socket to address
    if(bind(servSock, (struct sockaddr*) &servAddress, sizeof(servAddress)) == -1) {
        fprintf(stderr, "Error from otp_dec_d bind() system call.\n");
        exit(1);
    }
    
    // Allow control socket to accept incoming conxs (conx P) (backlog of 5)
    if((listen(servSock, 5)) == -1) {
        fprintf(stderr, "Error from otp_dec_d listen() system call.\n");
        exit(1);
    }
    
    return servSock;    // Return socket file descriptor
}


/****************************************************************************
 * Function Name: sendData
 * Description: This function is used to send data to peer via the given
    file descriptor argument.
 * Reference Citation: https://oregonstate.instructure.com/courses/1662153/pages/4-dot-2-verified-sending
 ****************************************************************************/
void sendData(int sockfd, char* dataToSend, int len)
{
    int bytesSent = 0, chkSend = -7;
    
    bytesSent = send(sockfd, dataToSend, len, 0);    // Send message to client
    if(bytesSent < 0) { fprintf(stderr, "Error sending to client.\n"); }
    
    // Ensure all bytes sent
    do
    {
        ioctl(sockfd, TIOCOUTQ, &chkSend);  // Check socket send buffer
    } while(chkSend > 0);
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


/***********************************************************************
 * Function Name: handleSIGCHLD
 * Description: This signal handler is used to reap zombies.
 * Reference Citation: https://linux.die.net/man/2/waitpid
 **********************************************************************/
void handleSIGCHLD(int signal)
{
    // using -1 as first parameter to indicate wait for any child process
    // the loop performs no purpose except to reap zombies
    while( waitpid(-1, 0, WNOHANG) > 0) {}
}

