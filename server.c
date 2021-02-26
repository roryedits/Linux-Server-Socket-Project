#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#define MAX 80 
#define MAXPORT 6
#define SA struct sockaddr 
#define MAXCLIENTS 25
#define IPSIZE 16
char clients[MAXCLIENTS][16];
time_t clientJoinTime[MAXCLIENTS];
bool eFlag = true;
FILE *logF;


//Function for initializing the client list
void intializeList(){
    int i = 0;
    while(i<MAXCLIENTS){
        clients[i][0] = '\0';
        i++;
    }
}

//Function for checking if connection already exists
int isConnected(char * clientIP){
    int i = 0;
    while(i < MAXCLIENTS){
        int flag = 0, j = 0;
        while(j < IPSIZE){
            if(clients[i][j] != clientIP[j]){
                flag++;
            }
            j++;
        }
        if(flag == 0){
            return i;
        }
        i++;
    }
    return -1;
}
//Function for adding clients to client array
bool addToClients(char * clientIP){
    if(!(isConnected(clientIP) == -1)){   //checks if ip is already connected
        return false;
    }else{
        int slot = 0;
        int count = 0;
        while(slot < MAXCLIENTS){
            if(clients[slot][0] == '\0'){
                clientJoinTime[slot] = time(NULL);
                while(count < IPSIZE){      //add ip to slot
                    clients[slot][count] = clientIP[count];
                    count++;
                }
                break;
            }
            slot++;
        }
        return true;
    }
}
//Function to remove client
bool removeClient(char * clientIP){
    int slot = isConnected(clientIP);
    if(slot >= 0){
        clients[slot][0] = '\0';
        return true;
    }else{
        return false;
    }
}
//Function for listing Clients
void listClients(char * buff,int sockfd){
    int i = 0;
    while(i < MAXCLIENTS){
        if(!(clients[i][0] == '\0')){
            int t = timeOnline(i);
            bzero(buff, MAX);
            strcpy(buff,"<");
            strcat(buff,clients[i]);
            strcat(buff,", ");
            char * temp;
            sprintf(temp, "%i", t);
            strcat(buff, temp);
            strcat(buff,">");
            write(sockfd, buff, strlen(buff));
        }
        i++;
    }
}

//Function for sending the log
void sendLog(int sockfd){
    rewind(logF);
    char * line =  NULL;
	size_t len = 0;
	ssize_t read;
    while((read = getline(&line, &len, logF)) != -1){		//read file by line
		write(sockfd, line, strlen(line));
	}
}
//Function for making time eaiser to utilize
struct tm getTime(time_t x){
    struct tm *ptm = localtime(&x);
    return *ptm;
}
//Function to add the formatted time to the file, this is used before the rest of the logging
void printTime(time_t x){
    struct tm ptm = getTime(x);    
    fprintf(logF,"%02d:%02d:%02d", ptm.tm_hour, ptm.tm_min, ptm.tm_sec);
}
//this is used to find the time in seconds that a client has been online.
int timeOnline(int slot){
    time_t temp = time(NULL);
    double elapsed_seconds = difftime(temp, clientJoinTime[slot]);
    return elapsed_seconds;
}

// Over all function handler for when client calls functions
void func(int sockfd, char * clientIP){ 
    char buff[MAX]; 
    int n; 
    bzero(buff, MAX); 

    read(sockfd, buff, sizeof(buff));
    ///////////////////////////////
    //  JOIN    ///////////////////
    ///////////////////////////////
    if(!strncmp(buff, "#JOIN", 5)){
    printTime(time(NULL));
    fprintf(logF,": Received a \"#JOIN\" action from Agent \"%s\"\n",clientIP);
        if(addToClients(clientIP)){
            printTime(time(NULL));
            fprintf(logF,": Responded to agent \"%s\" with \"$OK\"\n",clientIP);
            bzero(buff, MAX);
            strcpy(buff, "$OK");           
        }else{
            printTime(time(NULL));
            fprintf(logF,": Responded to agent \"%s\" with \"$ALREADY MEMBER\"\n",clientIP);
            bzero(buff, MAX);
            strcpy(buff, "$ALREADY MEMBER");
        }
        
        write(sockfd, buff, sizeof(buff));
    ///////////////////////////////
    //  LEAVE   ///////////////////
    ///////////////////////////////
    }else if(!strncmp(buff, "#LEAVE", 6)){
        printTime(time(NULL));
        fprintf(logF,": Received a \"#LEAVE\" action from Agent \"%s\"\n",clientIP);
        if(removeClient(clientIP)){
            printTime(time(NULL));
            fprintf(logF,": Responded to agent \"%s\" with \"$OK\"\n",clientIP);
            bzero(buff, MAX);
            strcpy(buff, "$OK");
       }else{
            printTime(time(NULL));
            fprintf(logF,": Responded to agent \"%s\" with \"$NOT MEMBER\"\n",clientIP);
            bzero(buff, MAX);
            strcpy(buff, "$NOT MEMBER");
       }

       write(sockfd, buff, sizeof(buff));

    ///////////////////////////////
    //  LIST    ///////////////////
    ///////////////////////////////
    }else if(!strncmp(buff, "#LIST", 5)){
        printTime(time(NULL));
        fprintf(logF,": Received a \"#LIST\" action from Agent \"%s\"\n",clientIP);
        if(isConnected(clientIP) == -1){
            printTime(time(NULL));
            fprintf(logF,": No response is supplied to Agent \"%s\" (agent not active)\n",clientIP);
            return;
        }else{
            printTime(time(NULL));
            fprintf(logF,": Responded with Agent list to Agent \"%s\"\n",clientIP);
            listClients(buff, sockfd);
        }

    ///////////////////////////////
    //  LOG     ///////////////////
    ///////////////////////////////
    }else if(!strncmp(buff, "#LOG", 4)){
        printTime(time(NULL));
        fprintf(logF,": Received an \"#LOG\" action from Agent \"%s\"\n", clientIP);
        if(isConnected(clientIP) == -1){
            printTime(time(NULL));
            fprintf(logF,": No response is supplied to Agent \"%s\" (agent not active)\n", clientIP);
            return;
        }else{
            printTime(time(NULL));
            fprintf(logF,": Responded to agent \"%s\" with Log.txt\n",clientIP);
            sendLog(sockfd);
        }

    ///////////////////////////////
    //  SHUTDOWN  /////////////////
    ///////////////////////////////
    }else if(!strncmp(buff, "#SHUTDOWN", 9)){
        printTime(time(NULL));
        fprintf(logF,": Received a \"#SHUTDOWN\" action from Agent \"%s\"\n",clientIP);
        eFlag = false;

    ///////////////////////////////
    //  DEFAULT     ///////////////
    ///////////////////////////////
    }else{
        printTime(time(NULL));
        fprintf(logF,": Received an Invalid action from Agent \"%s\"\n", clientIP);
        if(isConnected(clientIP) == -1){
            printTime(time(NULL));
            fprintf(logF,": No response is supplied to Agent \"%s\" (agent not active)\n", clientIP);
            return;
        }else{
            printTime(time(NULL));
            fprintf(logF,": Responded to agent \"%s\" with \"$INVALID ACTION\"\n",clientIP);
            bzero(buff, MAX);
            strcpy(buff, "$INVALID ACTION");
            write(sockfd, buff, sizeof(buff));
        }
    }
} 
  
// Driver function 
int main(int argc,char* argv[]){ 
    if (argc < 2){
        printf ("Usage: server server_port \r\n");
      return(0);
    }
    char cPort[MAXPORT];
    memset(cPort, 0, MAXPORT);
    sprintf(cPort,"%s",argv[1]);
    int PORT = atoi(cPort);


    int sockfd, connfd, len; 
    struct sockaddr_in servaddr, cli;
    char ip[IPSIZE];
    intializeList();

    logF = fopen("./log.txt", "w+");
    fclose(logF);
  
    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 
    bzero(&servaddr, sizeof(servaddr)); 
  
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        printf("socket bind failed...\n"); 
        exit(0); 
    }
    
    while(eFlag){
        logF = fopen("./log.txt","a+");
        // Now server is ready to listen and verification 
        if ((listen(sockfd, 5)) != 0) { 
            printf("Listen failed...\n"); 
            exit(0); 
        } 
        len = sizeof(cli); 
    
        // Accept the data packet from client and verification 
        connfd = accept(sockfd, (SA*)&cli, &len); 
        if (connfd < 0) { 
            printf("server acccept failed...\n"); 
            exit(0); 
        } 
        else{
            inet_ntop(AF_INET, &cli.sin_addr.s_addr, ip, sizeof(ip));
        }
    
        //Call functions
        func(connfd, ip);
        close(connfd); 
        fclose(logF);
    }
    // Shutdown server
    close(sockfd); 
} 