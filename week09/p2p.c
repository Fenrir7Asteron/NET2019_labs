#include "p2p.h"

struct message* message() {
    struct message* new_message = malloc(sizeof(struct message));
    new_message->type = 2;
    new_message->data[0] = '\0';
    return new_message;
}

struct connections* connections;
int lock = 0;
int sent_recv_bytes = 0,
    addr_len = sizeof(struct sockaddr);
    
int recv_till_null(int sockfd, char* message) {
    char byte = 1;
    int bytes_received = 0;
    while (byte != '\0') {
        byte = 0;
        recv(sockfd, &byte, 1, 0);
        char to_concat[2];
        to_concat[0] = byte;
        to_concat[1] = '\0';
        strcat(message, to_concat);
        ++bytes_received;
    }
    message[bytes_received] = '\0';
    return bytes_received;
}

char* get_files_list() {
    DIR *dir = opendir(FILES_DIRECTORY);
    if (dir == NULL) {
        printf("Directory %s does not exist.\n");
        free(dir);
        return NULL;
    }
    
    struct dirent *entry;
    char* files_list = malloc(MAX_FILE_NUMBER * MAX_FILE_LENGTH);
    files_list[0] = '\0';
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        sprintf(files_list + strlen(files_list), "%s", entry->d_name);
        break;
    }
    while (entry != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            entry = readdir(dir);
            continue;
        }
        sprintf(files_list + strlen(files_list), ",%s", entry->d_name);
        entry = readdir(dir);
    }

    free(dir);
    free(entry);
    return files_list;
}

void send_syn(int sockfd, char* trash) {
    char* response = malloc(MAX_MESSAGE_SIZE);
    
    //Send 1
    response[0] = 1;
    response[1] = '\0';
    sent_recv_bytes = send(sockfd, response, SIZE_OF_CHAR, 0);
    memset(response, 0, MAX_MESSAGE_SIZE);
    
    //Send name:ip_address:port:file1,...
    sprintf(response, "%s:", connections->my_address);
    char* files_list = get_files_list();
    strcat(response, files_list);
    response[strlen(response)] = '\0';
    sent_recv_bytes = send(sockfd, response, strlen(response) + 1, 0);
    
    //Send n (number of peers)
    response[0] = '\0';
    sent_recv_bytes = send(sockfd, (int*) &connections->connection_number, SIZE_OF_INT, 0);
    send(sockfd, "\0", 1, 0);
    memset(response, 0, MAX_MESSAGE_SIZE);
    
    //Iterate: Send name:ip_address:port
    int i;
    char* nodes_list = malloc(MAX_ADDRESS_LENGTH * MAX_PEER_NUMBER);
    nodes_list[0] = '\0';
    strcat(nodes_list, connections->nodes);
    
    char* address = strtok(nodes_list, "\n");
    for (i = 0; i < connections->connection_number; ++i) {
        if (address == NULL) {
            printf("Something went wrong. strtok() returned NULL\n");
        }
        char new_address[MAX_ADDRESS_LENGTH];
        new_address['\0'];
        memcpy(new_address, address, strlen(address));
        new_address[strlen(address)] = ':';
        new_address[strlen(address) + 1] = '\0';
        sent_recv_bytes = send(sockfd, new_address, strlen(new_address) + 1, 0);
        address = strtok(NULL, "\n");   
    }
    
    free(address);
    free(response);
}

void send_request(int sockfd, char* filename) {
    char* request = malloc(MAX_MESSAGE_SIZE);
    
    //Send 0
    request[0] = 0;
    request[1] = '\0';
    sent_recv_bytes = send(sockfd, request, SIZE_OF_CHAR, 0);
    memset(request, 0, MAX_MESSAGE_SIZE);
    
    //Send filename
    sent_recv_bytes = send(sockfd, filename, strlen(filename), 0);
    send(sockfd, "\0", SIZE_OF_CHAR, 0);
    
    //Receive n (word count)
    int n = 0;
    sent_recv_bytes = recv(sockfd, (int*) &n, SIZE_OF_INT, 0);
    recv(sockfd, request, 1, 0); // Reading '\0'

    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    
    char* directory = FILES_DIRECTORY;
    char* pathname = malloc(MAX_PATH_LENGTH);
    pathname[0] = '\0';
    sprintf(pathname, "%s/%d%s", directory, rawtime, filename);
    
    FILE* output = fopen(pathname, "w");
    int i;
    printf("File contains: ");
    for (i = 0; i < n; ++i) {
        memset(request, 0, MAX_MESSAGE_SIZE);
        sent_recv_bytes = recv_till_null(sockfd, request);
        printf("%s ", request);
        fputs(request, output);
        fputs(" ", output);
    }
    printf("\n");
    fclose(output);
    free(request);
}

void recv_request(int sockfd) {
    char* filename = malloc(MAX_MESSAGE_SIZE);
    filename[0] = '\0';
    recv_till_null(sockfd, filename);

    char* dir_name = FILES_DIRECTORY;
    
    DIR *dir = opendir(dir_name);
    if (dir == NULL) {
        printf("Directory \"%s\" does not exist.\n");
        return;
    }
    
    int file_is_there = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        if (strcmp(entry->d_name, filename) == 0) {
            file_is_there = 1;
            
            char* pathname = malloc(MAX_PATH_LENGTH);
            pathname[0] = '\0';
            sprintf(pathname, "%s/%s", dir_name, filename);
            
            FILE* input = fopen(pathname, "r");
            int word_count = 0;
            if (input) {
                char str[255];
                while (fscanf(input, "%s", str)!=EOF) {
                    ++word_count;
                }
            }
            
            int sent_recv_bytes = send(sockfd, (int*) &word_count, SIZE_OF_INT, 0);
            send(sockfd, "\0", 1, 0);
            
            char* response = malloc(MAX_MESSAGE_SIZE);
            input = fopen(pathname, "r");
            if (input) {
                char str[255];
                while (fscanf(input, "%s", str)!=EOF) {
                    response[0] = '\0';
                    strcpy(response, str);
                    response[strlen(response)] = '\0';
                    int sent_recv_bytes = send(sockfd, response, strlen(response) + 1, 0);
                }
            }
            free(response);
            fclose(input);
            
            break;
        }
    }
    
    if (file_is_there == 0) {
        printf("File \"%s\" not found", filename);
    }
    free(filename);
}

char* get_address(char* message) {
    char* address = malloc(MAX_ADDRESS_LENGTH);
    char name[MAX_NAME_LENGTH];
    char ip[IP_LENGTH];
    int port;
    sscanf(message, "%[^:]:%[^:]:%d", name, ip, &port);
    sprintf(address, "%s:%s:%d", name, ip, port);
    return address;
}

void recv_syn(int sockfd) {
    char* message = malloc(MAX_MESSAGE_SIZE);
    message[0] = '\0';
    
    //Receive name:ip_address:port:file1,...
    sent_recv_bytes = recv_till_null(sockfd, message);
    char* node_address = get_address(message);
    insert_address(node_address);
    update_data(message);
    int j = 0;   
    memset(message, 0, MAX_MESSAGE_SIZE);
    
    //Receive n (number of peers)
    int n = 0;
    sent_recv_bytes = recv(sockfd, (int*) &n, SIZE_OF_INT, 0);
    recv(sockfd, message, 1, 0);
    
    //Iterate: Receive name:ip_address:port
    int i;
    for (i = 0; i < n; ++i) {
        char address[MAX_ADDRESS_LENGTH];
        address[0] = '\0';
        sent_recv_bytes = recv_till_null(sockfd, address);
        char new_address[MAX_ADDRESS_LENGTH];
        char name[MAX_NAME_LENGTH];
        char ip[IP_LENGTH];
        int port;
        sscanf(address, "%[^:]:%[^:]:%d", name, ip, &port);
        sprintf(new_address, "%s:%s:%d", name, ip, port);
        insert_address(new_address);
    }
    
    free(message);
}


char* make_address(char* name, char* ip, int port) {
    char* address = malloc(MAX_ADDRESS_LENGTH);
    sprintf(address, "%s:%s:%d", name, ip, port);
    return address;
}

int file_inside(char* filename_to_check, int index) {
    char* name = malloc(MAX_NAME_LENGTH);
    char* ip = malloc(IP_LENGTH);
    int port;
    char* files_list = malloc(MAX_ADDRESS_LENGTH * MAX_PEER_NUMBER);
    sscanf(connections->data[index], "%[^:]:%[^:]:%d:%s", name, ip, &port, files_list);
    int result = 0;
    char* file = strtok(files_list, ",");
    while (file != NULL) {
        int diff = strcmp(file, filename_to_check);
        if (diff == 0) {
            result = 1;
            break;
        }
        file = strtok(NULL, ",");
    }
    return result;
}

void send_message_to_all(void (*function_pointer)(int, char*), char* filename) {
    int offset = 0;
    char* nodes_list = malloc(MAX_ADDRESS_LENGTH * MAX_PEER_NUMBER);
    nodes_list[0] = '\0';
    strcat(nodes_list, connections->nodes);
    char* address = strtok(nodes_list, "\n");
    int peer_number = 0;
    int file_found = 0;
    while (address != NULL) {
        char* ip = malloc(IP_LENGTH);
        ip[0] = '\0';
        char* sport = malloc(PORT_LENGTH);
        sport[0] = '\0';
        int i = 0;
        
        while (1) {
            if (address[i] == ':') {
                ++i;
                break;
            }
            ++i;
        }
        
        while (1) {
            if (address[i] == ':') {
                ++i;
                break;
            }
            char symbol[2];
            symbol[0] = address[i];
            symbol[1] = '\0';
            strcat(ip, symbol);
            ++i;
        }
        
        while (1) {
            if (address[i] == '\0') {
                ++i;
                break;
            }
            char symbol[2];
            symbol[0] = address[i];
            symbol[1] = '\0';
            strcat(sport, symbol);
            ++i;
        }
    
        if (filename == NULL || file_inside(filename, peer_number)) {
            file_found = 1;
            int port;
            sscanf(sport, "%d", &port);
            
            struct sockaddr_in dest;
            dest.sin_family = AF_INET;
            dest.sin_port = port;
            struct hostent *host = (struct hostent *)gethostbyname(ip);
            dest.sin_addr = *((struct in_addr *)host->h_addr);
            
            int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            int response = connect(sockfd, (struct sockaddr *)&dest,sizeof(struct sockaddr));
            if (response == 0) {
                function_pointer(sockfd, filename);
                if (filename != NULL) {
                    break;
                }
            } else {
                printf("Connection error on socket [%d]\n", sockfd);
                exit(0);
            }
            close(sockfd);
        }
        
        offset += strlen(address) + 1;
        address = strtok(nodes_list + offset, "\n");
        ++peer_number;
    }
    
    if (!file_found && filename != NULL) {
        printf("No node contain file: %s\n", filename);
    }
}

void* input_thread_function(void* arg) {
    while(1) {
        char filename[MAX_FILE_LENGTH];
        printf("Enter filename you want to download: ");
        scanf("%s", filename);
        printf("\n");
    
        send_message_to_all(send_request, filename);
    }
    exit(0);
}

void* syn_thread_function(void* arg) {
    while(1) {
        sleep(2);
        send_message_to_all(send_syn, NULL);
    }
}

int address_inside(char* address_to_check) {
    int result = 0;
    char* nodes_list = malloc(MAX_ADDRESS_LENGTH * MAX_PEER_NUMBER);
    nodes_list[0] = '\0';
    strcat(nodes_list, connections->nodes);
    char* address = strtok(nodes_list, "\n");
    
    while (address != NULL) {
        int diff = strcmp(address, address_to_check);
        if (diff == 0) {
            result = 1;
            break;
        }
        diff = strcmp(address_to_check, connections->my_address);
        if (diff == 0) {
            result = 1;
            break;
        }
        address = strtok(NULL, "\n");
    }
    return result;
}

void insert_address(char* address) {
    if (!address_inside(address)) {
        sprintf(connections->nodes + strlen(connections->nodes), "%s\n", address);
        connections->connection_number++;
    }
}

int data_inside(char* node_to_check) {
    char* address_to_check = malloc(MAX_ADDRESS_LENGTH);
    char* name = malloc(MAX_NAME_LENGTH);
    char* ip = malloc(IP_LENGTH);
    int port;
    sscanf(node_to_check, "%[^:]:%[^:]:%d", name, ip, &port);
    sprintf(address_to_check, "%s:%s:%d", name, ip, port);
    int result = 0;
    char* nodes_list = malloc(MAX_ADDRESS_LENGTH * MAX_PEER_NUMBER);
    nodes_list[0] = '\0';
    strcat(nodes_list, connections->nodes);
    char* address = strtok(nodes_list, "\n");
    int i = 0;
    while (address != NULL) {
        int diff = strcmp(address, address_to_check);
        if (diff == 0) {
            diff = strcmp(connections->data[i], node_to_check);
            if (diff != 0) {
                printf("Updated file list for peer\n%s\n", node_to_check);
                connections->data[i][0] = '\0';
                strcat(connections->data[i], node_to_check);
            }
            result = 1;
            break;
        }
        diff = strcmp(address_to_check, connections->my_address);
        if (diff == 0) {
            result = 1;
            break;
        }
        address = strtok(NULL, "\n");
        ++i;
    }
    return result;
}

void update_data(char* data) {
    if (!data_inside(data)) {
        connections->data[connections->connection_number - 1][0] = '\0';
        strcat(connections->data[connections->connection_number - 1], data);
    }
}

void setup_tcp_communication(char* my_name, char* my_ip, int my_port, char* node_ip, int node_port) {
    connections = malloc(sizeof(struct connections));
    
    connections->connection_number = 0;
    connections->nodes = malloc(MAX_ADDRESS_LENGTH * MAX_PEER_NUMBER);
    int i;
    for (i = 0; i < MAX_PEER_NUMBER; ++i) {
        connections->data[i] = malloc(MAX_FILE_NUMBER * MAX_FILE_LENGTH + MAX_ADDRESS_LENGTH);
    }
    connections->my_address = malloc(MAX_ADDRESS_LENGTH);
    connections->my_address = make_address(my_name, my_ip, my_port);
    
    pthread_t user_input_thread;
    pthread_create(&user_input_thread, NULL, input_thread_function, NULL);
    
    int master_sock_tcp_fd = 0;

    fd_set readfds;
    struct sockaddr_in server_addr, /*structure to store the server and client info*/
            client_addr;

    if ((master_sock_tcp_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("socket creation failed\n");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = my_port;
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

    if (listen(master_sock_tcp_fd, 5) < 0) {
        printf("listen failed\n");
        return;
    }
    
    // Tell about yourself
    if (node_port != -1) {
        struct sockaddr_in dest;
        dest.sin_family = AF_INET;
        dest.sin_port = node_port;
        struct hostent *host = (struct hostent *)gethostbyname(node_ip);
        dest.sin_addr = *((struct in_addr *)host->h_addr);
        
        int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        int response;
        response = connect(sockfd, (struct sockaddr *)&dest,sizeof(struct sockaddr));
        if (response == 0) {
            send_syn(sockfd, NULL);
        } else {
            printf("Connection error : errno = %d\n", errno);
            exit(1);
        }
        close(sockfd);
    }
    
    pthread_t syn_thread;
    pthread_create(&syn_thread, NULL, syn_thread_function, NULL);
    
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(master_sock_tcp_fd, &readfds);
        struct timeval timeout;
        timeout.tv_sec = 2;
        select(master_sock_tcp_fd + 1, &readfds, NULL, NULL, NULL);
        while (lock == 1) {
            // wait
        }
        
        if (FD_ISSET(master_sock_tcp_fd, &readfds)) {
            int comm_socket_fd = accept(master_sock_tcp_fd, (struct sockaddr *) &client_addr, &addr_len);
            if (comm_socket_fd < 0) {
                printf("accept error : errno = %d\n", errno);
                exit(1);
            }
            
            char request_type;
            sent_recv_bytes = recv(comm_socket_fd, &request_type, SIZE_OF_CHAR, 0);
            
            if (sent_recv_bytes == 0) {
                printf("Someone closed connection\n");
            } else if (sent_recv_bytes > 0) {
                if (request_type == 1) {
                    recv_syn(comm_socket_fd);
                } else {
                    recv_request(comm_socket_fd);
                }
            } else {
                printf("Something went wrong, exiting...\n");
                exit(1);
            }
            
            close(comm_socket_fd);
        }
    }
}
    

int main(int argc, char **argv, char** env) {
    char* my_ip = malloc(IP_LENGTH);
    int my_port = 0;
    
    char* node_ip = malloc(IP_LENGTH);
    int node_port = -1;
    
    if (argc < 2) {
        printf("Too low number of arguments. Forgot port number?\n");
        exit(1);
    } else {
        sscanf(argv[1], "%s", my_ip);
        sscanf(argv[2], "%d", &my_port);
        if (argc >= 4 && argc <= 5) {
            printf("%d\n", argc);
            memcpy(node_ip, argv[3], IP_LENGTH);
            sscanf(argv[4], "%d", &node_port);
        } else if (argc >= 6) {
            printf("Too many arguments.\n");
            exit(1);
        }
    }
    
    char* my_name = getenv("USER");
    setup_tcp_communication(my_name, my_ip, my_port, node_ip, node_port);
    printf("application quits\n");
    return 0;
}
