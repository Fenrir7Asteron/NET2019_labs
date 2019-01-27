#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_SIZE 256
#define COMMAND_SIZE 16

int main() {
    char* client_command = malloc(sizeof(COMMAND_SIZE));
    char* optional_data = malloc(sizeof(COMMAND_SIZE));
    char* server_command = malloc(sizeof(COMMAND_SIZE));
    int command_len = 0;
    int push_flag = 0;
    
    int command_pipe[2];
    pipe(command_pipe);
    
    if (fork()) { // parent (client)
        while (1) {
            client_command = malloc(sizeof(COMMAND_SIZE));
            
            scanf("%s", client_command);
            command_len = strlen(client_command);
            
            if (strcmp(client_command, "push") == 0) {
                scanf("%s", optional_data);
                int i;
                for (i = 0; i < strlen(optional_data); ++i) {
                    if (optional_data[i] < '0' || optional_data[i] > '9') {
                        push_flag = 1;
                        break;
                    }
                }
                if (push_flag) {
                    push_flag = 0;
                    printf("Wrong format of input. Please, enter a number, not [%s].\n", optional_data);
                    continue;
                }
                
                client_command[command_len++] = ' ';
                strcpy(&(client_command[command_len]), optional_data);
            }
            
            command_len = strlen(client_command);
            client_command[command_len] = '\0';
            
            close(command_pipe[0]);
            write(command_pipe[1], client_command, COMMAND_SIZE);
        }
    } else {  	  // child (server)
        int stack[MAX_SIZE];
        int stack_size = -1;
        
        while (1) {
            server_command = malloc(sizeof(COMMAND_SIZE));
            
            close(command_pipe[1]);
            read(command_pipe[0], server_command, COMMAND_SIZE);
            
            if (strcmp(server_command, "create") == 0) {
                if (stack_size == -1) {
                    stack_size = 0;
                    printf("Stack was created.\n");
                } else {
                    printf("Stack is already created.\n");
                }
            } else if (stack_size == -1) {
                printf("Firstly 'create' a stack.\n");
                continue;
            } else if (strcmp(server_command, "stack_size") == 0) {
                printf("Stack size is: %d.\n", stack_size);
            } else if (strcmp(server_command, "peek") == 0) {
                if (stack_size > 0) {
                    printf("Top element is: [%d].\n", stack[stack_size - 1]);
                } else {
                    printf("Stack is empty.\n");
                }
            } else if (strncmp(server_command, "push ", strlen("push ")) == 0) {
                char* data_str = malloc(sizeof(COMMAND_SIZE));
                data_str = strcpy(data_str, &(server_command[strlen("push ")]));
                int data = atoi(data_str);
                if (stack_size < MAX_SIZE) {
                   stack[stack_size++] = data;
                   printf("[%d] is pushed successfully.\n", data);
                } else {
                   printf("Stack is already full.\n");
                }
            } else if (strcmp(server_command, "pop") == 0) {
                if (stack_size > 0) {
                    int data = stack[--stack_size];
                    printf("[%d] is poped successfully.\n", data);
                } else {
                    printf("Stack is already empty.\n");
                }
            } else if (strcmp(server_command, "empty") == 0) {
                if (stack_size == 0) {
                    printf("Stack is empty.\n");
                } else {
                    printf("Stack is not empty.\n");
                }
            } else if (strcmp(server_command, "display") == 0) {
                if (stack_size == 0) {
                    printf("Stack is empty.\n");
                } else {
                    int i;
                    printf("Stack is [");
                    for (i = 0; i < stack_size - 1; ++i) {
                        printf("%d ", stack[i]);
                    }
                    printf("%d].\n", stack[stack_size - 1]);
                }
            } else {
                printf("There is no such command [%s].\n", server_command);
            }
        }
    }
}
