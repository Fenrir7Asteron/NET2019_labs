#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <memory.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "structures.h"

#define PORT_LENGTH 6
#define MAX_ADDRESS_LENGTH 64
#define MAX_PEER_NUMBER 16
#define MAX_FILE_LENGTH 32
#define MAX_PATH_LENGTH 64
#define MAX_WORD_SIZE 32
#define MAX_NAME_LENGTH 42

#define SIZE_OF_CHAR 1
#define SIZE_OF_INT 4

#define FILES_DIRECTORY "files"

struct message* message();
int recv_till_null(int sockfd, char* message);
char* get_files_list();
void send_message_to_all(void (*function_pointer)(int, char*), char* filename); // filename is optional here
void send_syn(int sockfd, char* trash);              // this function may be passed to send_message_to_all
void send_request(int sockfd, char* filename);       // this function may be passed to send_message_to_all
void recv_syn(int sockfd);
char* make_address(char* name, char* ip, int port);
void send_syn_to_all(void (*function_pointer)(int));

void* input_thread_function(void* arg);
void* syn_thread_function(void* arg);

int address_inside(char* address_to_check);
void insert_address(char* address);
int data_inside(char* node_to_check);
void update_data(char* data);
