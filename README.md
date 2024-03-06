## cProxy | C Language | Communication Applications Project
The code is a C program that fetches files from the internet using HTTP requests. It can handle HTTP GET requests to retrieve files from remote servers specified by a URL. The program first checks if the file exists locally. If it does, it serves the file from the local filesystem. If not, it establishes a connection to the remote server, sends an HTTP GET request, and retrieves the file. After downloading the file, it saves it to the local filesystem and optionally opens it in a web browser. The program also includes basic error handling to handle failures during file retrieval or creation.

### How to run:
Open linux terminal and navigate to the file folder
Once inside the folder type gcc -g cproxy.c -o run
Usage: cproxy <URL> [-s] (Use -s tag to open result in browser if applicable)
