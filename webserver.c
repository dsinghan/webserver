#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MYPORT 4000
#define READ_BUF_SIZE 4096
#define WRITE_BUF_SIZE 2097152
#define CODE_405 "HTTP/1.0 405 Method Not Allowed\r\n\r\n"
#define CODE_404 "HTTP/1.0 404 Not Found\r\n\r\n"
#define CODE_200 "HTTP/1.0 200 OK\r\n"
#define BIN_CONT "Content-Type: application/octet-stream\r\n"
#define HTML_CONT "Content-Type: text/html; charset=utf-8\r\n"
#define TXT_CONT "Content-Type: text/plain; charset=utf-8\r\n"
#define JPG_CONT "Content-Type: image/jpeg\r\n"
#define PNG_CONT "Content-Type: image/png\r\n"
#define GIF_CONT "Content-Type: image/gif\r\n"

int main() {
	int sockfd, new_fd; //socket to listen on, new connection
	struct sockaddr_in my_addr; //server address info
	struct sockaddr_in their_addr; //client address info
	unsigned int sin_size; //sockaddr struct size
	char read_buf[READ_BUF_SIZE]; //buffer for read
	char write_buf[WRITE_BUF_SIZE]; //buffer for write
	int write_offset; //write buffer offset
	char cont_buf[WRITE_BUF_SIZE]; //buffer for content
	size_t cont_buf_length; //length of content buffer
	char cont_length[128]; //string for content length

	//create socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("Error creating socket");
		exit(1);
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
    	perror("setsockopt(SO_REUSEADDR) failed");
	}
	//fprintf(stderr, "Created socket.\n");

	//initialize sockaddr struct
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(MYPORT);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(&(my_addr.sin_zero), '\0', 8);

	//bind socket
	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
		perror("Error binding socket");
		exit(1);
	}
	//fprintf(stderr, "Bound socket.\n");

	//listen on socket
	if (listen(sockfd, 5) == -1) {
		perror("Error listening on socket");
		exit(1);
	}
	//fprintf(stderr, "Started listening on socket.\n");

	while (1) {
		//reset buffer
		memset(read_buf, '\0', READ_BUF_SIZE);
		memset(write_buf, '\0', WRITE_BUF_SIZE);
		memset(cont_buf, '\0', WRITE_BUF_SIZE);
		//fprintf(stderr, "Reset buffer.\n");

		//accept new socket
		sin_size = sizeof(struct sockaddr_in);
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("Error accepting new socket");
			continue;
		}
		//fprintf(stderr, "Accepted new socket.\n");

		//read from new socket
		if (read(new_fd, read_buf, READ_BUF_SIZE) <= 0) {
			perror("Error reading from new socket");
			close(new_fd);
			continue;
		}
		fprintf(stdout, "%s", read_buf);

		//parse request for method and path
		const char *end_method = strchr(read_buf, ' ');
		const char *end_path = strchr(end_method + 1, ' ');
		char method[end_method - read_buf + 1];
		char path[end_path - end_method - 1];
		strncpy(method, read_buf, sizeof(method) - 1);
		strncpy(path, end_method + 2, sizeof(path) - 1);
		method[sizeof(method) - 1] = '\0';
        path[sizeof(path) - 1] = '\0';
		//fprintf(stderr, "Identified method and path.\n");

		//check method is GET
		if (strcmp(method, "GET") != 0) {
			fprintf(stderr, "%s", CODE_405);
			if (write(new_fd, CODE_405, strlen(CODE_405)) == -1) {
				perror("Error writing to new socket");
			}
			close(new_fd);
			continue;
		}
		//fprintf(stderr, "Checked for GET.\n");

		//open file at path
		FILE *fp = fopen(path, "r");
		if (fp == NULL || strcmp(path, "") == 0) {
			perror("Error opening file");
			fprintf(stderr, "%s", CODE_404);
			if (write(new_fd, CODE_404, strlen(CODE_404)) == -1) {
				perror("Error writing to new socket");
			}
			close(new_fd);
			continue;
		}
		//fprintf(stderr, "Opened file.\n");

		//read file into buffer
		cont_buf_length = fread(cont_buf, sizeof(char), WRITE_BUF_SIZE, fp);
		if (ferror(fp) != 0) {
			perror("Error reading file");
			fclose(fp);
			close(new_fd);
			continue;
		}
		fclose(fp);
		//fprintf(stderr, "Read file into buffer.\n");
		
		//add status line to response
		write_offset = 0;
		strcpy(write_buf, CODE_200);
		write_offset += strlen(CODE_200);
		//fprintf(stderr, "Added status line to response.\n");

		//add content type header to response
		const char *ext = strchr(path, '.');
		if (ext == NULL) {
			strcpy(write_buf + write_offset, BIN_CONT);
			write_offset += strlen(BIN_CONT);
		}
		else if (strcmp(ext, ".html") == 0) {
			strcpy(write_buf + write_offset, HTML_CONT);
			write_offset += strlen(HTML_CONT);
		}
		else if (strcmp(ext, ".txt") == 0) {
			strcpy(write_buf + write_offset, TXT_CONT);
			write_offset += strlen(TXT_CONT);
		}
		else if (strcmp(ext, ".jpg") == 0) {
			strcpy(write_buf + write_offset, JPG_CONT);
			write_offset += strlen(JPG_CONT);
		}
		else if (strcmp(ext, ".png") == 0) {
			strcpy(write_buf + write_offset, PNG_CONT);
			write_offset += strlen(PNG_CONT);
		}
		else if (strcmp(ext, ".gif") == 0) {
			strcpy(write_buf + write_offset, GIF_CONT);
			write_offset += strlen(GIF_CONT);
		}
		else {
			strcpy(write_buf + write_offset, BIN_CONT);
			write_offset += strlen(BIN_CONT);
		}
		//fprintf(stderr, "Added content type to response.\n");

		//add content length header to response
		sprintf(cont_length, "Content-Length: %lu\r\n\r\n", cont_buf_length);
		strcpy(write_buf + write_offset, cont_length);
		write_offset += strlen(cont_length);
		//fprintf(stderr, "Added content length to response.\n");

		//add content to response
		memcpy(write_buf + write_offset, cont_buf, cont_buf_length);
		write_offset += cont_buf_length;
		//fprintf(stderr, "Added content to response.\n");

		//send response to new socket
		if (write(new_fd, write_buf, write_offset) == -1) {
			perror("Error writing to new socket");
		}
		fprintf(stdout, "%s\n\n", write_buf);
		close(new_fd);
	}
	close(sockfd);
	exit(0);
}