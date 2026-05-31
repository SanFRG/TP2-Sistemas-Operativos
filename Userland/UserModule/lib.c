#include <lib.h>
#include <stdint.h>

#define NULL ((void*)0)

// String length
int strlen(const char* str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

// String compare
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

// Helper: convert to lowercase
static char to_lower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    return c;
}

// String compare (case-insensitive)
int strcasecmp(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        char c1 = to_lower(*s1);
        char c2 = (*s2);
        if (c1 != c2) {
            return (unsigned char)c1 - (unsigned char)c2;
        }
        
        s1++;
        s2++;
    }
    
    return to_lower(*s1) - (*s2);
}

// Print string
void print(const char* str) {
    int len = strlen(str);
    write(1, str, len);
}

// Print string with newline
void println(const char* str) {
    print(str);
    print("\n");
}

// Print integer
void printInt(int num) {
    char buffer[20];
    itoa(num, buffer);
    print(buffer);
}

// Print hex
void printHex(uint64_t num) {
    char buffer[20];
    hexToString(num, buffer);
    print("0x");
    print(buffer);
}

// Convert string to integer
int atoi(const char *str) {
    int result = 0;
    int sign = 1;
    if (*str == '-') {
        sign = -1;
        str++;
    }
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str++ - '0');
    }
    return sign * result;
}

// Convert integer to string
void itoa(int num, char* buffer) {
    if (num == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }
    
    int is_negative = 0;
    if (num < 0) {
        is_negative = 1;
        num = -num;
    }
    
    int i = 0;
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    if (is_negative) {
        buffer[i++] = '-';
    }
    
    buffer[i] = '\0';
    
    // Reverse the string
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;
        start++;
        end--;
    }
}

// Convert hex to string
void hexToString(uint64_t num, char* buffer) {
    const char hex_chars[] = "0123456789ABCDEF";
    
    if (num == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }
    
    int i = 0;
    while (num > 0) {
        buffer[i++] = hex_chars[num % 16];
        num /= 16;
    }
    buffer[i] = '\0';
    
    // Reverse
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;
        start++;
        end--;
    }
}

// Print two-digit number (with leading zero if needed)
void print2Digits(int num) {
    char buffer[3];
    buffer[0] = '0' + (num / 10);
    buffer[1] = '0' + (num % 10);
    buffer[2] = '\0';
    print(buffer);
}

char getChar() {
    char c;
    read(&c, 1);  // Read 1 character (blocks until Enter or buffer has input)
    return c;
}

int64_t create_process_piped(char *name, void (*entry)(void *), void *arg,
                              int priority, int foreground, int fd_in, int fd_out) {
    create_proc_piped_args_t args;
    args.name = name;
    args.entry = entry;
    args.arg = arg;
    args.priority = priority;
    args.foreground = foreground;
    args.fd_in = fd_in;
    args.fd_out = fd_out;
    return create_process_piped_raw(&args);
}   
