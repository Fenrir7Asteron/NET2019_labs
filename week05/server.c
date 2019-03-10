//Taken from Abhishek Sagar

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <errno.h>
#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*Server process is running on this port no. Client has to send data to this port no*/
#define SERVER_PORT 2000

char data_buffer[1024];
test_struct_t test_struct;
result_struct_t res_struct;

struct address {
	int sockfd;
	test_struct_t *client_data;
	struct sockaddr_in client_addr;
};

void* serve_client(void* args) {
	printf("Hello from new thread [%lu]!\n\n", (unsigned long) pthread_self());

	struct address* address = (struct address*) args;

	int sockfd = address->sockfd;
	test_struct_t *client_data = address->client_data;
	struct sockaddr_in client_addr = address->client_addr;

	result_struct_t result;
	strcpy(result.name, client_data->name);
        result.age = client_data->age;
	strcpy(result.group, client_data->group);

	
	printf("Received next data\nName: %s\nAge: %d\nGroup: %s\n", result.name, result.age, result.group);
	sleep(10);

	result.age = 12;
                /* Server replying back to client now*/
        int sent_recv_bytes = sendto(sockfd, (char *) &result, sizeof(result_struct_t), 0,
                                         (struct sockaddr *) &client_addr, sizeof(struct sockaddr));

        printf("Server sent %d bytes in reply to client\n", sent_recv_bytes);
                /*Goto state machine State 3*/
}

void
setup_udp_server_communication() {

    /*Step 1 : Initialization*/
    /*Socket handle and other variables*/
    /*Master socket file descriptor, used to accept new client connection only, no data exchange*/
    int master_sock_udp_fd = 0,
            sent_recv_bytes = 0,
            addr_len = 0,
            opt = 1;

    /*client specific communication socket file descriptor,
     * used for only data exchange/communication between client and server*/
    //int comm_socket_fd = 0;
    /* Set of file descriptor on which select() polls. Select() unblocks whever data arrives on
     * any fd present in this set*/
    fd_set readfds;
    /*variables to hold server information*/
    struct sockaddr_in server_addr, /*structure to store the server and client info*/
            client_addr;

    /*step 2: udp master socket creation*/
    if ((master_sock_udp_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
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

    /* Bind the server. Binding means, we are telling kernel(OS) that any data
     * you recieve with dest ip address = 192.168.56.101, and udp port no = 2000, pls send that data to this process
     * bind() is a mechnism to tell OS what kind of data server process is interested in to recieve. Remember, server machine
     * can run multiple server processes to process different data and service different clients. Note that, bind() is
     * used on server side, not on client side*/

    if (bind(master_sock_udp_fd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) == -1) {
        printf("socket bind failed\n");
        return;
    }

    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(master_sock_udp_fd, (struct sockaddr *)&sin, &len) == -1)
        perror("getsockname");
    else
        printf("port number %d\n", ntohs(sin.sin_port));

    /* Server infinite loop for servicing the client*/
    while (1) {

        /*Step 5 : initialze and dill readfds*/
        FD_ZERO(&readfds);                     /* Initialize the file descriptor set*/
        FD_SET(master_sock_udp_fd, &readfds);  /*Add the socket to this set on which our server is running*/

        printf("blocked on select System call...\n");

        /*Step 6 : Wait for client connection*/
        /*state Machine state 1 */

        /*Call the select system call, server process blocks here. Linux OS keeps this process blocked untill the data arrives on any of the file Drscriptors in the 'readfds' set*/
        select(master_sock_udp_fd + 1, &readfds, NULL, NULL, NULL);

        /*Some data on some fd present in set has arrived, Now check on which File descriptor the data arrives, and process accordingly*/
        if (FD_ISSET(master_sock_udp_fd, &readfds)) {
            printf("New connection recieved.\n");

                printf("Server ready to service client msgs.\n");
                /*Drain to store client info (ip and port) when data arrives from client, sometimes, server would want to find the identity of the client sending msgs*/
                memset(data_buffer, 0, sizeof(data_buffer));

                /*Step 8: Server recieving the data from client. Client IP and PORT no will be stored in client_addr
                 * by the kernel. Server will use this client_addr info to reply back to client*/

                /*Like in client case, this is also a blocking system call, meaning, server process halts here untill
                 * data arrives on this comm_socket_fd from client whose connection request has been accepted via accept()*/
                /* state Machine state 3*/
                sent_recv_bytes = recvfrom(master_sock_udp_fd, (char *) data_buffer, sizeof(data_buffer), 0,
                                           (struct sockaddr *) &client_addr, &addr_len);

                /* state Machine state 4*/
                printf("Server recvd %d bytes from client %s:%u\n", sent_recv_bytes,
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		
		struct address* args = malloc(sizeof(struct address));
		args->sockfd = master_sock_udp_fd;
		args->client_data = (test_struct_t *) data_buffer;
		args->client_addr = client_addr;

		pthread_t thread;
		pthread_create(&thread, NULL, serve_client, (void *) args);
        }
    }/*step 10 : wait for new client request again*/
}

int
main(int argc, char **argv) {

    setup_udp_server_communication();
    return 0;
}
