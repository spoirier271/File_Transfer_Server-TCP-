

#include "myunp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>


#define ARRAY_SIZE_MAX 1000
#define DELIM "|"

int fsize(FILE*);
int parse_client_filename_header(char *, char *);
int make_server_file_size_header(int, int, char *);
int parse_client_chunk_header(char *, int *, int *, char *);
int make_server_chunk_header(char *, int);
void get_message_type(char *, char *);

int main(int argc, char **argv)
{
	//initializations
	int     listenfd, connfd, i, chunk_count, chunk_number, chunk_size, bytes_to_send, offset;
	bool uneven;
	struct  sockaddr_in servaddr;
	char buff[MAXLINE];
	int file_size;
	char server_header[2];
	char header_from_client[ARRAY_SIZE_MAX];
	char filename[100];
	char file_portion[ARRAY_SIZE_MAX / 3];
	char chunk_header[ARRAY_SIZE_MAX];
	char msg_type[100];
	FILE *fp;
   listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    
	//initialize socket
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1])); /* daytime server */

	//bind socket
    Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
	
	while(1) {
	
		//set to listening
		 Listen(listenfd, LISTENQ);
	
		//accept connection from client
		connfd = Accept(listenfd, (SA *) NULL, NULL);
		bzero(&header_from_client, sizeof(header_from_client));
		Read(connfd, header_from_client, ARRAY_SIZE_MAX);
		strncpy(msg_type, header_from_client, 4);
		if(strcmp(msg_type, "FILE") == 0) {
			parse_client_filename_header(header_from_client, filename);
			fp = fopen(filename,"r");
			if(fp == NULL) {
				make_server_file_error_header(filename, 1, server_header, "file not found");
			} else {
				file_size = fsize(fp);
				fclose(fp);
				
				//create header with size of file
				make_server_file_size_header(file_size, 0, server_header);
			}
			
			//send size of file to client
			Write(connfd, server_header, strlen(server_header));
		}
		if(strcmp(msg_type, "CHUN") == 0) {
			parse_client_chunk_header(header_from_client, &chunk_number, &chunk_size, &filename[0]);
	
			//read file chunk
			char * data = (char*)malloc(chunk_size);
			i = 0;
			fp = fopen(filename,"r");
			if(fp == NULL) {
				make_server_chunk_error_header(chunk_header, 1, "bad file name");
				Write(connfd, chunk_header, strlen(chunk_header));
			} else {
				offset = chunk_number * chunk_size;
				fseek(fp, offset, SEEK_SET);
				for(i = 0; i < chunk_size; i++) {
				
					data[i] = fgetc(fp);
					if( feof(fp) ) {
						break ;
					}
				}
				data[i] = 0;
				fclose(fp);
		
				//Send file chunk to client
				make_server_chunk_header(chunk_header, 0);
				Write(connfd, chunk_header, strlen(chunk_header));
				Write(connfd, data, strlen(data));
				free(data);
				printf("sending chunk %d of %s to client\n", chunk_number, filename);
			}
		}
	
		//close connection with client
		Close(connfd);
	}
}

int fsize(FILE *fp){
    int p = ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int s = ftell(fp);
    fseek(fp, p,SEEK_SET);
    return s;
}

int make_server_file_error_header(char * file_name, int return_code, char *server_header, char * error_msg) {

	//fill header with type, return code and error message
	char temp[100];
	strcpy(server_header,"SIZE");
	strcat(server_header, DELIM);
	sprintf(temp, "%d", return_code);
	strcat(server_header, temp);
	strcat(server_header, DELIM);
	strcat(server_header, error_msg);
}


int parse_client_filename_header(char * client_header, char * filename) {

	//store name of file
	char junk[ARRAY_SIZE_MAX];
	strcpy(junk, strtok(client_header, DELIM));
	strcpy(filename, strtok(NULL, DELIM));
}

int make_server_file_size_header(int file_size, int return_code, char *server_header) {

	//fill header with type, return code and size data
	char temp[100];
	strcpy(server_header,"SIZE");
	strcat(server_header, DELIM);
	sprintf(temp, "%d", return_code);
	strcat(server_header, temp);
	strcat(server_header, DELIM);
	sprintf(temp, "%d", file_size);
	strcat(server_header, temp);
}

int parse_client_chunk_header(char * header, int * chunk_number, int * chunk_size, char * file_name) {

	//extract chunk number, size and name of file from client's header
	char junk[ARRAY_SIZE_MAX];
	strcpy(junk, strtok(header, DELIM));
	strcpy(file_name, strtok(NULL, DELIM));
	strcpy(junk, strtok(NULL, DELIM));
	*chunk_number = atoi(junk);
	strcpy(junk, strtok(NULL, DELIM));
	strcpy(junk, strtok(NULL, DELIM));
	*chunk_size = atoi(junk);
}

int make_server_chunk_error_header(char *header, int return_code, char *error_msg) {

	//make header containing type, return code and error message
	char temp[100];
	strcpy(header,"CHUNKDATA");
	strcat(header, DELIM);
	sprintf(temp, "%d", return_code);
	strcat(header, temp);
	strcat(header, DELIM);
	strcat(header, error_msg);
}

int make_server_chunk_header(char *header, int return_code) {

	//make header with chunk data
	char temp[100];
	strcpy(header,"CHUNKDATA");
	strcat(header, DELIM);
	sprintf(temp, "%d", return_code);
	strcat(header, temp);
	strcat(header, DELIM);
}

void get_message_type(char *header, char *msg_type) {
	strcpy(msg_type, strtok(header, DELIM));
}
	