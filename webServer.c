#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_DATA 8096
#define MAX_CLIENT 100

int contains(char* w1, char* w2){
    int i=0;
    int j=0;


    while(w1[i]!='\0'){
        if(w1[i] == w2[j])
        {
            int init = i;
            while (w1[i] == w2[j] && w2[j]!='\0')
            {
                j++;
                i++;
            }
            if(w2[j]=='\0'){
                return 1;
            }
            j=0;
        }
        i++;
    }
    return 0;
}


sub(int connfd)
{

	int i, file_fd, data_len;
    long file_len;
	char buffer[MAX_DATA+1]; 
    char* keepAlive = "keep-alive";
    int keep = 0;

    do{
        bzero(buffer,MAX_DATA+1);
        data_len = read(connfd,buffer,MAX_DATA); /* Read Incoming Message */
        
        if(contains(buffer,keepAlive)==1){
            keep = 1;
        }else{
            keep = 0;
        }

        /* Get filename from the request ignoring 'GET ' eg. GET image.jpg*/
        for(i=4;i<MAX_DATA;i++) { 
            if(buffer[i] == ' ') {
                buffer[i] = '\0';
                break;
            }
        }

        printf("Proccessing %s\n",buffer);

        if((file_fd = open(&buffer[5],O_RDONLY)) == -1) {  /* Open Requested File */
            printf("File not found\n");
            exit(1);
        }
        file_len = (long)lseek(file_fd, (off_t)0, SEEK_END); /* lseek to the file end to find the length */
        lseek(file_fd, (off_t)0, SEEK_SET); /* lseek back to the file start ready for reading */
        
        sprintf(buffer,"HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n",file_len); /*Append HTTP Header */

        write(connfd,buffer,strlen(buffer)); /* Write HTTP Headers */

        /* Write File */
        while ((data_len = read(file_fd, buffer, MAX_DATA)) > 0 ) {
            write(connfd,buffer,data_len);
        }
        
        printf("Complete Writing\n");
        
    }while(keep);

}

int main(int argc, char **argv){

    int pid, listenfd, connfd;
	struct sockaddr_in client, server; 
    socklen_t sockaddr_len = sizeof(client);

    if(argc!=2){
        printf("Too few arguments\n");
        printf("Usage: %s <PORT>\n",argv[0]);
        exit(1);
    }

	printf("HTTP Server Starting on port %s\n",argv[1]);

	if((listenfd = socket(AF_INET, SOCK_STREAM,0)) == -1){
		perror("socket");
        exit(1);
    }

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(atoi(argv[1]));

	if(bind(listenfd, (struct sockaddr *)&server,sizeof(server)) == -1){
		perror("bind");
        exit(1);
    }

	if( listen(listenfd,MAX_CLIENT) <0){
        perror("listen");
        exit(1);
    }

	while(1) {

		if((connfd = accept(listenfd, (struct sockaddr *)&client, &sockaddr_len)) == -1){
            perror("accept");
            exit(1);
        }

        printf("New Client Connected from port %d and IP %s\n",
        htons(client.sin_port),inet_ntoa(client.sin_addr));

		
        if((pid = fork()) == 0) { 
            close(listenfd);
            sub(connfd);
            close(connfd);
            printf("Closing Connection\n");
            exit(0);
        }
        
        close(connfd);	
        
	}
}