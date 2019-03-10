#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <errno.h>

#define DEST_PORT       2000
#define SERVER_PORT     2000
#define SERVER_IP_ADDRESS   "192.168.2.22"
#define MAX_MESSAGE_LEN 255

char data_buffer[1024];

void setup_tcp_communication() {
    /*Step 1 : Initialization*/
    /*Socket handle*/
    int sockfd = 0, 
        sent_recv_bytes = 0;

    int addr_len = 0;

    addr_len = sizeof(struct sockaddr);

    /*to store socket addesses : ip address and port*/
    struct sockaddr_in dest;

    /*Step 2: specify server information*/
    /*Ipv4 sockets, Other values are IPv6*/
    dest.sin_family = AF_INET;

    /*Client wants to send data to server process which is running on server machine, and listening on 
     * port on DEST_PORT, server IP address SERVER_IP_ADDRESS.
     * Inform client about which server to send data to : All we need is port number, and server ip address. Pls note that
     * there can be many processes running on the server listening on different no of ports, 
     * our client is interested in sending data to server process which is lisetning on PORT = DEST_PORT*/ 
    dest.sin_port = DEST_PORT;
    struct hostent *host = (struct hostent *)gethostbyname(SERVER_IP_ADDRESS);
    dest.sin_addr = *((struct in_addr *)host->h_addr);

    /*Step 3 : Create a TCP socket*/
    /*Create a socket finally. socket() is a system call, which asks for three paramemeters*/
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    int master_sock_tcp_fd = 0,
            opt = 1;
    int comm_socket_fd = 0;
    fd_set readfds;
    struct sockaddr_in server_addr, /*structure to store the server and client info*/
            client_addr;

    if ((master_sock_tcp_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)  {
        printf("socket creation failed\n");
        exit(1);
    }

    /*Step 3: specify server Information*/
    server_addr.sin_family = AF_INET;/*This socket will process only ipv4 network packets*/
    server_addr.sin_port = SERVER_PORT;/*Server will process any data arriving on port no 2000*/

    /*3232249957; //( = 192.168.56.101); Server's IP address,
    //means, Linux will send all data whose destination address = address of any local interface
    //of this machine, in this case it is 192.168.56.101*/

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
    //Client part
    printf("trying to connect\n");
    int response;
    response = connect(sockfd, (struct sockaddr *)&dest,sizeof(struct sockaddr));
    if (response >= 0) {

    printf("Your message: ");
    char* message;
    scanf("%s", message);

    sent_recv_bytes = sendto(sockfd, 
	   message,
	   MAX_MESSAGE_LEN, 
	   0, 
	   (struct sockaddr *)&dest, 
	   sizeof(struct sockaddr));
    
    printf("No of bytes sent = %d\n", sent_recv_bytes);

    char* received_message;
    sent_recv_bytes =  recvfrom(sockfd, received_message, MAX_MESSAGE_LEN, 0,
	            (struct sockaddr *)&dest, &addr_len);

    printf("No of bytes received = %d\n", sent_recv_bytes);
    
    printf("Result received\n");
    printf("%s\n", received_message);
    }

	//Server part
        FD_ZERO(&readfds);   
        FD_SET(master_sock_tcp_fd, &readfds);

        printf("blocked on select System call...\n");
	
	struct timeval timeout;
	timeout.tv_sec = 5;
        select(master_sock_tcp_fd + 1, &readfds, NULL, NULL, &timeout);

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
	    char* client_message = malloc(MAX_MESSAGE_LEN);

            sent_recv_bytes = recvfrom(comm_socket_fd, client_message, MAX_MESSAGE_LEN, 0, (struct sockaddr *) &client_addr, &addr_len);

            printf("Server recvd %d bytes from client %s:%u\n", sent_recv_bytes,
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            if (sent_recv_bytes == 0) {
                    close(comm_socket_fd);
                    break;
            }

            if (client_message == 0) {

            	close(comm_socket_fd);
                printf("Server closes connection with client : %s:%u\n", inet_ntoa(client_addr.sin_addr),
                           ntohs(client_addr.sin_port));
                    break;
            }

            char* echo_response = malloc(MAX_MESSAGE_LEN);
	    strcpy(echo_response, client_message);

            sent_recv_bytes = sendto(comm_socket_fd, echo_response, MAX_MESSAGE_LEN, 0,
                                         (struct sockaddr *) &client_addr, sizeof(struct sockaddr));

                printf("Server sent %d bytes in reply to client\n", sent_recv_bytes);
	}
    }	
}
    

int main(int argc, char **argv) {
    setup_tcp_communication();
    printf("application quits\n");
    return 0;
}

