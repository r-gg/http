/**
 * @author Rastko Gajanin, 11930500
 * @brief c File for the Assignment 3B http server
 * @date 17.12.2020
 */

#include "server.h"


static int quit = 0;


char* fileName;
/**
 * @remark No exit in myUsage(). Only print to stdout
 */
static void myUsage(){ 
	fprintf(stderr, "\nSYNOPSIS\n%s\t[USAGE]\t %s  [-p PORT]  [-i INDEX] DOC_ROOT\nEXAMPLE\n%s -p 1280 -i index.html ~/Documents/my_website/\n", fileName,fileName,fileName); 
} 

static void signalHandler(int sig){
    quit++;
}

static void printDate(FILE * sockFile){
    time_t rawtime;
    struct tm *info;
    char buffer[80];

    time(&rawtime); // fetches the time
    info = localtime(&rawtime); // saves it into the tm struct
    //Date: %a, %d %B %y %X %Z
    strftime(buffer,80,"Date: %a, %d %B %y %X %Z\r\n", info);
    if( 0 > fputs(buffer,sockFile)){
        fprintf(stderr,"error in putting date");
    }
}

/**
 * @brief searches the size of requested file in bytes and prints it in sockfile as Content-Length: size
 * @param requestedFile PRE: is not null
 */
static void printContentLength(FILE * sockFile, FILE * requestedFile){
    fseek(requestedFile,0L, SEEK_END); // put the stream pointer on the end of file

    long posOfFilepointer = ftell(requestedFile);
    char * buff = malloc(sizeof(char)*100);
    if(0>sprintf(buff,"Content-Length: %ld\r\n", posOfFilepointer)){
        fprintf(stderr,"err in printCL sprintf");
    }
    if(0>fputs(buff,sockFile)){
        fprintf(stderr,"err in printCL fputs");
    }
    free(buff);
}

static void printHeader(int command, FILE * sockFile, FILE * requestedFile){
    switch (command)
    {
    case 400:
        fputs("HTTP/1.1 400 Bad Request\r\n",sockFile);
        break;
    case 404:
        fputs("HTTP/1.1 404 Not Found\r\n",sockFile);
        break;
    case 501:
        fputs("HTTP/1.1 501 Not implemented\r\n",sockFile);
        break;
    case 200:
        fputs("HTTP/1.1 200 OK\r\n",sockFile);
        fputs("HTTP/1.1 200 OK\r\n",stdout); //FOR DEBUGGING
        printDate(sockFile);
        printDate(stdout); // FOR DEBUGGING 
        printContentLength(sockFile,requestedFile);
        printContentLength(stdout,requestedFile); // FOR DEBUGGING
        break;
    default:
        fprintf(stderr,"Invalid command");
        break;
    }
    fprintf(sockFile, "Connection: close\r\n");
	fprintf(sockFile, "\r\n");
    fprintf(stdout, "Connection: close\r\n"); // FOR DEBUGGING
	fprintf(stdout, "\r\n"); // FOR DEBUGGING
    // printDate
    // print content length
}
/**
 * writing binary objects
 */
static void writeToFile(FILE * in, FILE * out){
    uint8_t * binary_buffer = malloc(sizeof(uint8_t)*1024);

    while(!feof(in)){
	ssize_t n = fread(binary_buffer, sizeof(uint8_t), 1, in); // not eof, therefore n cant be -1,,, n must be 1 (or 0)
    fwrite(binary_buffer,sizeof(binary_buffer),n,out);
    }

    free(binary_buffer);
}


int main (int argc, char *argv[]){
    fileName = argv[0];

    struct sigaction sigint,sigterm;
	memset(&sigint, 0, sizeof sigint);
	memset(&sigterm, 0, sizeof sigterm);

	sigint.sa_handler = signalHandler;
	sigterm.sa_handler = signalHandler;

    sigaction(SIGINT, &sigint, NULL);
    sigaction(SIGTERM, &sigterm, NULL);
    char o;
    char* port = "8080";
    char* indexName = "index.html";
    int nrOfPorts = 0;
    int nrOfIndexArgs = 0;
    while((o = getopt(argc,argv,"p:i:")) != -1){
        switch (o)
        {
        case 'p':
            port = optarg;
            nrOfPorts++;
            break;
        case 'i':
            indexName = optarg;
            nrOfIndexArgs++;
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
    if(nrOfPorts>1 || nrOfIndexArgs>1){
        fprintf(stderr,"Too many ports or index args given");
        myUsage();
        exit(EXIT_FAILURE);
    }
    
    if(argc - optind != 1){
        fprintf(stderr,"Amount of positional arguments not = 1\n");
        myUsage();
        exit(EXIT_FAILURE);
    }
    char * doc_root = argv[optind];
    fprintf(stdout, "Port: %s\nIndex: %s\ndoc_root = %s\n",port,indexName,doc_root);

    struct addrinfo hints, *ai;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int res = getaddrinfo( NULL , port, &hints, &ai);
    if (res != 0) {
        const char * errDesc = gai_strerror(res);
        fprintf(stderr,"Error in getaddrinfo with error code = %d\nError Description: %s\n",res, errDesc);
        myUsage();
        exit(EXIT_FAILURE);    
    }
    int sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sockfd < 0) {
        fprintf(stderr,"Error in socket");
        myUsage();
        exit(EXIT_FAILURE);
    }
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval,sizeof(optval));
    errno = 0;
    if ( bind(sockfd, ai->ai_addr, ai->ai_addrlen) < 0) {
        fprintf(stderr,"Error in bind, errno = %d\n",errno);
        myUsage();
        exit(EXIT_FAILURE);
    }
    
    if(listen(sockfd,1) < 0){
        fprintf(stderr,"Error in listen");
        exit(EXIT_FAILURE);
    }
    while (quit == 0){
    
        int connfd = accept(sockfd,NULL,NULL);
        if(connfd < 0){
            fprintf(stderr,"Error in accept");
            close(connfd);
            break;
        }
        FILE * sockFile = fdopen(connfd,"r+");
        char* fstLine = malloc(sizeof(char)*MAX_SIZE_OF_FST_LINE);
        if(NULL == fgets(fstLine,sizeof(fstLine),sockFile)){
            fprintf(stderr,"Error in fgets");
            free(fstLine);
            fflush(sockFile);
            fclose(sockFile);
            close(connfd);
            break;
        }
        char* reqMethod = malloc(sizeof(char)*MAX_SIZE_OF_FST_LINE);
        char* path1 = malloc(sizeof(char)*MAX_SIZE_OF_FST_LINE);
        /*
        char * token;
        token = strtok(fstLine," ");
        if(token != NULL){
            reqMethod = token;
        }
        {
        
            token = strtok(NULL," ");
        }
        */
        int numOfFilledVariables = sscanf(fstLine,"%[^ ] /%[^ ] HTTP/1.1\r\n",reqMethod,path1); // %[^ ] everything up until ' '
        fprintf(stderr,"path1 = %s\n",path1);
    
        if(numOfFilledVariables < 1){
        fprintf(stderr,"Bad request");
            printHeader(400,sockFile,NULL);
            free(path1);
            free(reqMethod);
            free(fstLine);
            fflush(sockFile);
            fclose(sockFile);
            close(connfd);
            continue;
        }
        if(strcmp(reqMethod,"GET") != 0){
            fprintf(stdout,"Not implemented yet");
            printHeader(501,sockFile,NULL);
            free(path1);
            free(reqMethod);
            free(fstLine);
            fflush(sockFile);
            fclose(sockFile);
            close(connfd);
            continue;
        }
        char *fullPath = malloc(sizeof(char)*MAX_SIZE_OF_FST_LINE);
        fullPath[0] = '\0';
        strcat(fullPath,doc_root);
        strcat(fullPath,path1); // root/path
        if(fullPath[strlen(fullPath)-1] == '/'){ 
            //fprintf(stdout,"fullPath does end with '/'\n");
            strcat(fullPath,indexName); // root/path/index.html
        }


        fprintf(stderr, "GET: %s PATH: %s\n", reqMethod, fullPath);
        errno = 0;
        FILE * requestedFile = fopen(fullPath,"r+");
        if(requestedFile == NULL){
            fprintf(stderr,"error in opening requested file, errno = %d\n",errno);
            if(errno == 2){
                printHeader(404,sockFile,NULL);
            }
            fprintf(stdout,"Not implemented yet");
            printHeader(501,sockFile,NULL);
            free(fullPath);
            fclose(requestedFile);
            free(path1);
            free(reqMethod);
            free(fstLine);
            fflush(sockFile);
            fclose(sockFile);
            close(connfd);
            continue;
        }
        printHeader(200,sockFile,requestedFile);
        writeToFile(requestedFile,sockFile);
        rewind(requestedFile);
        writeToFile(requestedFile,stdout); //FOR DEBUGGING
        fflush(sockFile);
        fflush(stdout);// FOR DEBUGGING
        fclose(requestedFile);
        free(fullPath);
        free(reqMethod);
        free(path1);
        fclose(sockFile);
        free(fstLine);
        //shutdown(connfd,SHUT_WR);
        close(connfd);
        //connfd = -1;
    }
    /*
    char * ln;
    size_t n = 0;
    getline(&ln,&n,sockFile);
    fprintf(stdout,"First line: %s\n,getline() = %s\n",fstLine,ln);
    fflush(stdout);*/
    /*
    fflush(sockFile);
    fflush(stdout);// FOR DEBUGGING
    fclose(requestedFile);
    free(fullPath);
    free(reqMethod);
    free(path1);
    fclose(sockFile);
    free(fstLine);*/
    freeaddrinfo(ai);  
    //close(connfd);
    close(sockfd);





    return 0;
}