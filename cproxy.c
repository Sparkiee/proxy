// strncpy issues
// snprintf
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>

// modifies the function and removes the file name from path
void removeFileName(char *path, char *filename) {
    char *lastSlash = strrchr(path, '/');

    if (lastSlash != NULL) {
        // Check if a file name exists in the path
        if (lastSlash[1] != '\0') {
            strcpy(filename, lastSlash + 1);  // Copy the file name
            *lastSlash = '\0';  // Remove the file name from the path
        } else {
            filename[0] = '\0';  // No file name found, set filename to empty
        }
    } else {
        // No slash found, set filename to the entire path
        strcpy(filename, path);
        path[0] = '\0';  // Set path to empty
    }
}

void createPathAndFile(const char *folderPath, const char *fileName) {
    char currentPath[256] = {0};  // Keep track of the current working directory
    if (getcwd(currentPath, sizeof(currentPath)) == NULL) {
        perror("getcwd");
        return;
    }

    char pathCopy[256] = {0};
    snprintf(pathCopy, sizeof(pathCopy), "%s", folderPath);

    // Create directory path
    char *token = strtok(pathCopy, "/");

    while (token != NULL) {
        char partialPath[256] = {0};
        snprintf(partialPath, sizeof(partialPath), "%s", token);

        // Check if the directory already exists
        struct stat st;
        if (stat(partialPath, &st) != 0 || !S_ISDIR(st.st_mode)) {
            // Directory doesn't exist, attempt to create it
            if (mkdir(partialPath, 0777) != 0) {
//                printf("Error creating directory %s.\n", partialPath);
                perror("mkdir");
                return;
            }
        }

        // Change current working directory to the created directory
        if (chdir(partialPath) != 0) {
            perror("chdir");
            return;
        }

        token = strtok(NULL, "/");
    }

    // Restore the original working directory
    if (chdir(currentPath) != 0) {
        perror("chdir");
        return;
    }

    // Concatenate current working directory, folder path, and file name
    char fullPath[512] = {0};
    snprintf(fullPath, sizeof(fullPath), "%s/%s/%s", currentPath, folderPath, fileName);

    // Create the file
    FILE *file = fopen(fullPath, "w");
    if (file != NULL) {
        fclose(file);
//        printf("File '%s' created successfully in folder '%s'.\n", fileName, folderPath);
    } else {
        perror("fopen");
//        printf("Error creating file '%s' in folder '%s'.\n", fileName, folderPath);
    }
}

// checks if file exists in given path
int fileExists(const char *filePath) {
    struct stat buffer;
    return (stat(filePath, &buffer) == 0);
}

// getting file size
long getFileSize(char *filepath) {
    struct stat st;

    if (stat(filepath, &st) == 0) {
        return st.st_size;
    } else {
        // Return -1 to indicate an error
        return -1;
    }
}

void httpRequest(int orignalPort, char* hostname, char* path, char* combinedPath) {
    in_port_t port = orignalPort;
    struct hostent* srv = NULL;
    int socket_fd;
    // creating the socket
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        perror("socket");
        exit(1);
    }

    // getting the ipv4 from the dns server
    srv = gethostbyname(hostname);
    if (!srv) {
        herror("gethostbyname");
        close(socket_fd);
        exit(1);
    }

    struct sockaddr_in socketInfo;
    memset(&socketInfo, 0, sizeof(struct sockaddr_in));
    socketInfo.sin_family = AF_INET;
    socketInfo.sin_port = htons(port);
    socketInfo.sin_addr.s_addr = ((struct in_addr*)srv->h_addr)->s_addr;

    // doing the connection
    if (connect(socket_fd, (struct sockaddr*)&socketInfo, sizeof(struct sockaddr_in)) == -1) {
        perror("connect");
        close(socket_fd);
        exit(1);
    }

    // Construct a simple HTTP GET request
    char request[1024] = {0};
    // formatting the GET request for the server
    snprintf(request, sizeof(request), "GET %s HTTP/1.0\r\nHost: %s:%d\r\n\r\n",path, hostname,orignalPort);

    printf("HTTP request =\n%s\nLEN = %d\n", request,(int)strlen(request));

    // sending the given request
    if (send(socket_fd, request, strlen(request), 0) < 0) {
        perror("send");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    // if there's no path at the end, it adds up /index.html
    int len = (int)strlen(combinedPath);
    if (combinedPath[len - 1] == '/') {
        strcat(combinedPath, "index.html");
    }

    char filename[256] = {0};  // Assuming a maximum file name length of 255 characters

    unsigned char response[1024] = {0};  // Adjust the buffer size as needed
    ssize_t bytesRead;
    size_t totalSize = 0;

    FILE* file = NULL;  // File pointer for writing data to the file
    int statusCode = -1;

    int processingHeaders = 1;  // Flag to indicate whether we're still processing headers

    // reading the response
    while ((bytesRead = read(socket_fd, response, sizeof(response) - 1)) > 0) {
        response[bytesRead] = '\0';  // Null-terminate the received data
        totalSize += bytesRead;

        // handing the headers received from the server
        if (processingHeaders) {
            unsigned char* headerEndPtr = (unsigned char*) strstr((const char*)response, "\r\n\r\n");
            if (headerEndPtr != NULL) {
                ssize_t headerLength = headerEndPtr - response + 4;
                printf("%.*s", (int)headerLength, response);  // Print headers using printf
                //printf("\n");

                char* statusStart = strstr((const char*)response, "HTTP/1.");
                if (sscanf(statusStart, "HTTP/1.%*d %d", &statusCode) == 1) {
                    if (statusCode == 200) { // if we received 200 code (success)
                        // Create folders and file
                        removeFileName(combinedPath, filename);
                        createPathAndFile(combinedPath, filename);

                        strcat(combinedPath, "/");
                        strcat(combinedPath, filename);

                        // Open the file in binary write mode
                        file = fopen(combinedPath, "wb");
                        if (!file) {
                            perror("fopen");
                            close(socket_fd);
                            exit(EXIT_FAILURE);
                        }

                        // Write the remaining data (after headers) to the file
                        fwrite(response + headerLength, 1, bytesRead - headerLength, file);

                        // Writing to the terminal
                        fwrite(response + headerLength, 1, bytesRead - headerLength, stdout);
                        // Set the flag to indicate we've finished processing headers
                        processingHeaders = 0;
                    }
                }
            }
        } else { //not processing the headers, aka writing the file
            // Continue reading and printing the rest of the response
            printf("%s", response);
            fwrite(response, 1, bytesRead, file);
        }
    }

    printf("\nTotal response bytes: %zu\n", totalSize);

    // Close the file if it was opened
    if (file) {
        fclose(file);
    }

    // Close the socket
    close(socket_fd);
}

void checkRequestAvailability(char* hostname, int port, char* path, int flag)
{
    //combining the path and the hostname
    char combinedPath[512] = {0};  // Adjust the size as needed
    snprintf(combinedPath, sizeof(combinedPath), "%s%s", hostname, path);

    //fixing up the url if it has no path or only a '/', adding up index.html
    char fixedPath[512] = {0};
    strcat(fixedPath, combinedPath);
    int len = (int)strlen(combinedPath);
    if(len == 0)
        strcat(fixedPath, "/");
    if (combinedPath[len - 1] == '/' || len == 0) {
        strcat(fixedPath, "index.html");
    }
    // if file exists on PC
    if(fileExists(fixedPath)) {

        char *message1 = "File is given from local filesystem\n";
        char *message2 = "HTTP/1.0 200 OK\r\nContent-Length: %ld\r\n\r\n";
        // Temporary buffers for storing formatted strings
        char buffer1[256]; // Adjust the size according to your needs
        char buffer2[256]; // Adjust the size according to your needs

        // Format the strings and store them in temporary buffers
        snprintf(buffer1, sizeof(buffer1), "%s", message1);
        snprintf(buffer2, sizeof(buffer2), message2, getFileSize(fixedPath));
        int total_length = snprintf(NULL, 0, "%s", buffer2);
        printf("%s", buffer1);
        printf("%s", buffer2);
        // printing the file body:
        FILE* file = fopen(fixedPath, "rb");  // Open the file in binary read mode
        if (!file) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
        char buffer[1024] = {0};
        size_t bytesRead;

        while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
            fwrite(buffer, 1, bytesRead, stdout);  // Print file content to stdout
        }

        printf("\n Total response bytes: %ld\n", (total_length + getFileSize(fixedPath)));

        fclose(file);
    }
    else // if file doesn't exist on PC
    {
        httpRequest(port,hostname, path, combinedPath);

    }
    // if -s flag existed, it opens the file from the pc in the browser
    if(flag == 1 && fileExists(fixedPath))
    {
        char openPath[1024] = {0};
        snprintf(openPath, sizeof(openPath), "xdg-open %s", fixedPath);
        system(openPath);
    }
}

void parseURL(char* url, int flag) {
    // if the code doesn't start with "http://" we print usage
    if (strncmp(url, "http://", 7) != 0) {
        printf("Usage: cproxy <URL> [-s]\n");
        return;
    }
    // pointers to split up the url from the format of http://[hostname]:[port][filepath]
    char *remaining = url + 7;
    char *colon = strchr(remaining, ':');
    char *slash = strchr(remaining, '/');
    char *end = strchr(remaining, '\0');
    char hostname[256] = {0};
    int port;
    if (colon != NULL && (slash == NULL || colon < slash)) {
        // Found ':', and it appears before '/'
        strncpy(hostname, remaining, colon - remaining);
        hostname[colon - remaining] = '\0';

        port = atoi(colon + 1);
    } else {
        // No ':' or ':' appears after '/'
        strncpy(hostname, remaining, (slash != NULL ? slash : end) - remaining);
        hostname[(slash != NULL ? slash : end) - remaining] = '\0';
        port = 80;
    }
    char path[512] = {0};
    strcat(path, slash != NULL ? slash : end);

    if(strlen(path) == 0)
        strcat(path, "/");

    checkRequestAvailability(hostname, port, path, flag);
}

int main(int argc, char* argv[]) {
    // Grabbing URL from first input
    char* url = argv[1];
    //char* url = "http://www.pdf995.com/";
    //char* url = "http://blala/";
    //char* url = "http://www.josephwcarrillo.com";
    //char* url = "http://www.josephwcarrillo.com/JosephWhitfieldCarrillo.jpg";
    //char* url = "http://www.csun.edu/science/ref/games/questions/97_phys.pdf";
    //char* url = "http://placekitten.com/200/300";
    // a flag to know if we need to open browser

    int openBrowser = 0;
    if(url == NULL || strlen(url) == 0 || argc > 3 || (argc == 3 && strcmp(argv[2], "-s") != 0))
    {
        // Extra flag but not -s, therefore printing usage
        // if no input has been entered, no url prints usage
        printf("Usage: cproxy <URL> [-s]\n");
        exit(1);
    }

    if (argc == 3 && strcmp(argv[2], "-s") == 0) {
        openBrowser = 1;
    }

    // parse URL
    parseURL(url, openBrowser);
    return 0;
}

