#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <errno.h>

#define DEST_PORT       2000
#define SERVER_PORT     2000
#define SERVER_IP_ADDRESS   "192.168.1.11"

char data_buffer[1024];

void setup_tcp_communication() {
    int sockfd = 0, 
        sent_recv_bytes = 0;

    int addr_len = 0;

    addr_len = sizeof(struct sockaddr);

    struct sockaddr_in dest;

    dest.sin_family = AF_INET;

    dest.sin_port = DEST_PORT;
    struct hostent *host = (struct hostent *)gethostbyname(SERVER_IP_ADDRESS);
    dest.sin_addr = *((struct in_addr *)host->h_addr);
    
    int master_sock_tcp_fd = 0,
            opt = 1;
    int comm_socket_fd = 0;
    fd_set readfds;
    struct sockaddr_in server_addr, client_addr;

    if ((master_sock_tcp_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)  {
        printf("socket creation failed\n");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = SERVER_PORT;
    server_addr.sin_addr.s_addr = INADDR_ANY;

    addr_len = sizeof(struct sockaddr);

    if (bind(master_sock_tcp_fd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) == -1) {
        printf("socket bind failed\n");
        return;
    }

    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(master_sock_tcp_fd, (struct sockaddr *)&sin, &len) == -1)
        perror("getsockname");
    else
        printf("port number %d\n", ntohs(sin.sin_port));

    if (listen(master_sock_tcp_fd, 5) < 0) {
        printf("listen failed\n");
        return;
    }

    while(1) {
    printf("Trying to connect\n");
    //Client part
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int response;
    response = connect(sockfd, (struct sockaddr *)&dest,sizeof(struct sockaddr));
    if (response >= 0) {

    printf("Your message: ");
    memset(data_buffer, 0, sizeof(data_buffer));
    scanf("%s", data_buffer);
    printf("%s\n", data_buffer);

    sent_recv_bytes = sendto(sockfd, 
	   (char*) data_buffer,
	   sizeof(data_buffer), 
	   0, 
	   (struct sockaddr *)&dest, 
	   sizeof(struct sockaddr));
    }

	//Server part
        FD_ZERO(&readfds);   
        FD_SET(master_sock_tcp_fd, &readfds);
	
	printf("Waiting for connection\n");
        select(master_sock_tcp_fd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(master_sock_tcp_fd, &readfds)) {
            printf("New connection recieved recvd, accept the connection. Client and Server completes TCP-3 way handshake at this point\n");

            comm_socket_fd = accept(master_sock_tcp_fd, (struct sockaddr *) &client_addr, &addr_len);
            if (comm_socket_fd < 0) {
                printf("accept error : errno = %d\n", errno);
                exit(0);
            }

            printf("Connection accepted from client : %s:%u\n",
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

	    printf("Server ready to service client msgs.\n");
	    memset(data_buffer, 0, sizeof(data_buffer));

            sent_recv_bytes = recvfrom(comm_socket_fd, (char *) data_buffer, sizeof(data_buffer), 0, (struct sockaddr *) &client_addr, &addr_len);

            printf("Server recvd %d bytes from client %s:%u\n", sent_recv_bytes,
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            if (data_buffer == 0) {

            	close(comm_socket_fd);
                printf("Server closes connection with client : %s:%u\n", inet_ntoa(client_addr.sin_addr),
                           ntohs(client_addr.sin_port));
                    break;
            }

	    printf("Interlocutor's message: %s\n", data_buffer);
            usleep(500000);
	}
    }	
}
    

int main(int argc, char **argv) {
    setup_tcp_communication();
    printf("application quits\n");
    return 0;
}

