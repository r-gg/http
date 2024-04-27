/**
 * @author Rastko Gajanin, 11930500
 * @brief c File for the Assignment 3A http client
 * @date 17.12.2020
 */

#include "client.h"

char* fileName;
/**
 * @remark No exit in myUsage(). Only print to stdout
 */
static void myUsage(){ 
	fprintf(stderr, "\nSYNOPSIS\n%s\t[USAGE]\t %s  [-p PORT] [ -o FILE | -d DIR ] URL\nEXAMPLE\n%s http://pan.vmars.tuwien.ac.at/osue/\n", fileName,fileName,fileName); 
} 

int main (int argc, char *argv[]){
    fileName = argv[0];
    char o;
    int fileOrDir = 0; // 0 when neither, 1 when at least one already occured
    char* port = "80";
    int nrOfPorts = 0;
    char* output;
    while((o = getopt(argc,argv,"p:o:d:")) != -1){
        switch (o)
        {
        case 'p':
            port = optarg;
            nrOfPorts++;
            break;
        case 'o':
            if(fileOrDir == 0){
                output = optarg;
                fileOrDir++;
            }else
            {
                fprintf(stderr,"Only one file or one directory possible.\n");
                myUsage();
                exit(EXIT_FAILURE);
            }
            
            break;
        case 'd':
            if(fileOrDir == 0){
                output = optarg;
                fileOrDir++;
            }else
            {
                fprintf(stderr,"Only one file or one directory possible.\n");
                myUsage();
                exit(EXIT_FAILURE);
            }
            break;
        case '?':
            fprintf(stderr,"Error in options.");
            myUsage();
            break;
        
        default:
            myUsage();
            exit(EXIT_FAILURE);
            break;
        }
    }
    if(nrOfPorts > 1){
        fprintf(stderr,"Number of given ports too large");
        myUsage();
        exit(EXIT_FAILURE);
    }
    FILE * outputFile;
    if(fileOrDir == 0){
        outputFile = stdout;
    }
    else
    {   
        errno = 0;
        outputFile = fopen(output,"r+");
        if(outputFile == NULL){
            fprintf(stderr,"Error in fopen: %d\n",errno);
            if(errno == 2 ){
                fprintf(stderr,"File or directory does not exist ! \n");
            }
            myUsage();
            exit(EXIT_FAILURE);
        }
    }
    
    if(argc - optind != 1){
        fprintf(stderr,"Amount of positional arguments not = 1\n");
        myUsage();
        exit(EXIT_FAILURE);
    }
    char *url = argv[optind];
    
    /*
    if(url[strlen(url)-1] == '/'){
        strcat(url,"index.html");
    }*/

    char hostnameUntrimmed[100];
    int charCountInHostName = 0;
    for (size_t i = 7; i < strlen(url); i++) // assumed that the url starts with "http://" -> 7 characters -> next index in url is 7, therefore i = 7
    {   
        if(url[i] == '/' || url[i] == '?' || url[i] == ':' || url[i] == ';' || url[i] == '@' || url[i] == '=' || url[i] == '&'){
            hostnameUntrimmed[i-7] = '\0';
            break;
        }
        hostnameUntrimmed[i-7] = url[i];
        charCountInHostName++;
    }
    char hostName[charCountInHostName];
    strncpy(hostName,hostnameUntrimmed,charCountInHostName);

    char restOfTheUrl[1+ strlen(url) - charCountInHostName-7];
    for (size_t i = charCountInHostName+7; i < strlen(url); i++)
    {
        restOfTheUrl[i-charCountInHostName-7] = url[i];
    }
    
    
    //fprintf(stdout,"port = %s\noutput = %s\nurl = %s\nhostName = %s\nrest of the url = %s\nstrlen(url) = %d\n",port,outputFile,url,hostName, restOfTheUrl,strlen(url));
    
    struct addrinfo hints, *ai;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    int res = getaddrinfo(hostName, port, &hints, &ai);
    if (res != 0) {
        const char * errDesc = gai_strerror(res);
        fprintf(stderr,"Error in getaddrinfo with error code = %d\nError Description: %s\n",res, errDesc);
        myUsage();
        exit(EXIT_FAILURE);
    } 
    int sockfd = socket(ai->ai_family, ai->ai_socktype,
    ai->ai_protocol);
    if (sockfd < 0) {
        fprintf(stderr,"Error in socket");
        myUsage();
        exit(EXIT_FAILURE);
    }
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval,sizeof(optval));
    if (connect(sockfd, ai->ai_addr, ai->ai_addrlen) < 0) {
        fprintf(stderr,"Error in connect");
        myUsage();
        exit(EXIT_FAILURE);
    }

    char request[44+strlen(url)-7];
    if( 0 > sprintf(request,"GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",restOfTheUrl,hostName)){
        fprintf(stderr,"Error in sprintf");
    }
    FILE * sockFile = fdopen(sockfd,"r+");
    if(fputs(request,sockFile) == EOF){
        fprintf(stderr,"error in request to sockfile fputs");
        exit(EXIT_FAILURE);
    }
    if(fflush(sockFile) == EOF){
        fprintf(stderr,"error in request to sockfile fflush");
        exit(EXIT_FAILURE);
    }
    //fprintf(stdout,"Request: \n%s\nstrlen(request) = %d\n",request,strlen(request));
    char *responseBuffer = malloc(sizeof(char) * MAX_SIZE_OF_RESPONSE_BUFFER);
    int statusCode;
    if( NULL == fgets(responseBuffer,sizeof(char)*MAX_SIZE_OF_RESPONSE_BUFFER,sockFile)){
        fprintf(stderr,"Error in fgets\n");
        exit(EXIT_FAILURE);
    }

    int amountOfReadValues = sscanf(responseBuffer,"HTTP/1.1 %d",&statusCode);
    if(amountOfReadValues != 1){
        fprintf(stderr,"Protocol error!\n");
        exit(2);
    }
    if(statusCode != 200){ // 200 is status code for OKs
        fprintf(stderr, "%s ERROR: %s\n", fileName, responseBuffer);
		exit(3);
    }
    int shouldWriteToOut = 0; // 1 if the program should write to out
    while( NULL != fgets(responseBuffer,sizeof(char)*MAX_SIZE_OF_RESPONSE_BUFFER,sockFile)){
        if(shouldWriteToOut == 0){
            //fprintf(stdout,"Response buffer: %s\n",responseBuffer);
            if(strcmp(responseBuffer,"\r\n") == 0){ //check for empty line
                shouldWriteToOut++;
                //fputs(responseBuffer,outputFile);
            }
        }
        else
        {
            //fputs(responseBuffer,outputFile);
            if(fputs(responseBuffer,outputFile) == EOF){
                fprintf(stderr,"error in request to sockfile fputs");
                exit(EXIT_FAILURE);
            }
        }
        
    }

    if(fflush(outputFile) == EOF){
        fprintf(stderr,"error in request to sockfile fflush");
        exit(EXIT_FAILURE);
    }


    /*
    errno = 0;
    if(0 > send(sockfd,request,sizeof(request), MSG_WAITALL)){
        fprintf(stderr,"error in send\n errno = %d",errno);
        exit(EXIT_FAILURE);
    }
    char response[10000];
    if(0 > recv(sockfd,response,sizeof(response), MSG_WAITALL)){
        fprintf(stderr,"error in recv\n errno = %d",errno);
        exit(EXIT_FAILURE);
    }*/
    //fprintf(stdout,"ResponseBuffer:\n%s\nstrlen(responseBuffer) = %d\n",responseBuffer,strlen(responseBuffer));
    
    free(responseBuffer);
    freeaddrinfo(ai);
    fclose(sockFile);
    close(sockfd);

    if(fileOrDir != 0){
        fclose(outputFile);
    }
    
}


















