#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <arpa/inet.h>
#define true 1

int main(){
	/* Open an IPV4, send and receive socket on machine */
	/* It returns socket descriptor or -1 on failure, so I save it in variable 's' to manage */
	printf("Creating server endpoint (socket) ...\n");
	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	/* Error handling */
	if (socket_fd < 0){
		printf("Create endpoint failed. Exiting ...\n");
		return 0;
	}
	else{
		printf("Socket created.\n");
	}

	/* Inittalize a sockaddr_in object to store data stream */
	const struct sockaddr_in addr = {
		.sin_family = AF_INET, /* Refers to IPV4 protocol */
		.sin_port = htons(8080), /* Refers to 8080. Port 8080 is 0x1f90 in hex,
				       but because of LIFO reason, we need to reverse it */
		.sin_addr = 0 /* Use default setting */
	};

	/* Once socket opened successfully, bind it with a port */
	/* It returns 0 on success and -1 on failure */
	printf("Binding port ...\n");
	int b = bind(socket_fd, &addr, sizeof(addr));
	if (b){
		printf("Bind port failed. Exiting ...\n");
		return 0;
	}
	else{
		printf("Port binded.\n");
	}

	/* Once port binded successfully, listen to incoming connections */
	/* On success, 0 is returned. Otherwise, return -1 */
	printf("Initialize listening channel ...\n");
	int l = listen(socket_fd, 10); // 10 connections at a time is enough for POC
	if (l){
		printf("Listening failed. I'm deaf now\n");
		return 0;
	}
	else{
		printf("Listening ...\n");
	}

	/* Accept for incoming connection request on a listening socket */
	/* Return -1 on failure, otherwise return client's socket file descriptor */
	printf("Accepting request(s) ...\n");
	/* accept() function has three params:
	   -	int sockfd: socket file descriptor of opening socket on the server.
			    This socket is initialized with socket() function, bind to a specific port with bind()
			    and ready to listen with listen().
	   -	struct sockaddr *addr: a pointer to the socket which belong to the incoming connection (a.k.a client).
				       If 0 is assigned, it means that the server will accept the connection but don't know
				       any information about it, like IP address. If you want to collect information,
				       you have to init a sockaddr object for client and pass the pointer as parameter.
				       After that, you can extract infor from sockaddr's properties.
	   -	socklen_t *addrlen: this is a value-result argument, which is a declared term: "Value-result argument is an
				    argument that its value can be changed when being passed to a function". You can initialize
				    it like this: socklen_t sockSize = sizeof(client_socket). This argument is used to tell the
				    accept() function the actual size of client socket address. addrlen's value and the actual size
				    of client socket address can be different, potentially result to data loss when transfering data.
				    So, it's neccessary to pass a pointer *addrlen instead of only pass its value. The accept() function
				    can compare the actual size of sockaddr with addrlen and update addrlen value if those two are
				    different, avoid losing data.

		If sockaddr is NULL, it means the server only accept connection from clients but don't need to know anything about it.
		In this situation, addrlen also left NULL */
	int client_fd = accept(socket_fd, 0, 0);
	if (client_fd < 0){
		printf("Request declined due to error(s). Exiting ...\n");
		return 0;
	}
	else{
		printf("Request accepted!\n");
	}

	/* When request is accepted, receive data from it */
	/* Return -1 when error(s) occur, otherwise return the number of byte received. When end-of-file reached, return 0 */
	/* Only use this function with connected socket */
	char data_buffer[256]; // Buffer to store received data
	int rec = recv(client_fd, data_buffer, 256, 0);
	if (rec < 0){
		printf("Bad data. Exiting ...\n");
		return 0;
	}
	else{
		printf("Bytes received from request: %ld byte(s).\n", strlen(data_buffer));
	}

	/* HTTP request will look like this: GET /abc.html ... , but we only want the file name abc.html */
	char *fn = data_buffer + 5; // Because 'GET /' took 5 bytes
	if (!*strchr(fn, ' ')){ // strchr() return a pointer to needed character. If not found, return NULL.
		printf("Character ' ' not found, found '%c' instead. Maybe HTTP request was broken. Exiting ...\n", *strchr(fn, ' '));
		return 0;
	}
	else{
		*strchr(fn, ' ') = '\0'; // Null-terminated character in C-style string.
	}

	/* Open file, return -1 on error or file descriptor */
	int server_fd = open(fn, O_RDONLY); // Read-only
	if (server_fd < 0){
		printf("Error in opening file. Exiting ...\n");
		return 0;
	}
	else{
		printf("File opened.\n");
	}

	/* Transfer data from client's file descriptor to server's file descriptor */
	while (true){
		int sent = sendfile(client_fd, server_fd, NULL, 256); /* client_fd is the file to writem server_fd is the file to read,
								 	 NULL pass to off_t *offset (a pointer to the position that this
								 	 function should read in server_fd) means just read from the start of file.*/
		if (sent < 0){
			printf("Cannot send file due to unknown error(s). Exiting ...\n");
			return 0;
		}
		else if (sent > 0){
			printf("Sent %d byte(s).\n");
		}
		else{
			printf("File has been sent!\n");
			break;
		}
	}

	/* Close all files and socket descriptors */
	close(client_fd);
	close(server_fd);
	close(socket_fd);
}
