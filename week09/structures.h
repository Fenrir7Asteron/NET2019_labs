#include <netinet/in.h>

#define IP_LENGTH 16
#define ADDRESS_LENGTH 22
#define MAX_NUMBER_OF_PEERS 256
#define MAX_MESSAGE_SIZE 256
#define MAX_FILE_NUMBER 64

int cast_chars_to_int(char* chars) {
    return chars[0] * 256 + chars[1];
}

char* cast_int_to_chars(int integer) {
    char* chars = malloc(2);
    chars[0] = integer / 256;
    chars[1] = integer % 256;
    return chars;
}

struct message {
    char type;
    char data[MAX_MESSAGE_SIZE - 1];
};

struct connections {
    int connection_number;
    char* nodes;
    char* data[MAX_FILE_NUMBER];    
    char* my_address;
};
