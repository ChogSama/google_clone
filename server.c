#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024

int main()
{
    WSADATA wsa;
    SOCKET server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    const char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<!DOCTYPE html>"
        "<html lang='en'>"
        "<head><meta charset='UTF-8'><title>Google Clone</title></head>"
        "<body>"
        "<div id='logo'>Google</div>"
        "<form action='/search' method='GET'>"
        "<input type='text' name='q' placeholder='Search Google Clone'>"
        "<input type='submit' value='Search'>"
        "</form>"
        "</body></html>";

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("Failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        printf("Could not create socket: %d\n", WSAGetLastError());
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR)
    {
        printf("Bind failed with error code: %d\n", WSAGetLastError());
        return 1;
    }

    // Listen
    listen(server_fd, 3);
    printf("Server running on http://localhost:%d\n", PORT);

    while (1)
    {
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket == INVALID_SOCKET)
        {
            printf("Accept failed: %d\n", WSAGetLastError());
            continue;
        }

        int valread = recv(new_socket, buffer, BUFFER_SIZE, 0);
        buffer[valread] = '\0';

        char response[BUFFER_SIZE * 4];
        char query[256] = "";

        // Parse GET request for "/search/q=..."
        char *q_ptr = strstr(buffer, "GET /search?q=");
        if (q_ptr)
        {
            q_ptr += strlen("GET /search?q=");
            char *space_ptr = strstr(q_ptr, " ");
            if (space_ptr)
            {
                int len = space_ptr - q_ptr;
                if (len > 255)
                    len = 255;
                strncpy(query, q_ptr, len);
                query[len] = '\0';
            }
        }

        // Build HTML response
        snprintf(response, sizeof(response),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/html\r\n\r\n"
                 "<html><body>"
                 "<h1>Search Results for: %s</h1>"
                 "<p>(This is a fake Google Clone)</p>"
                 "<a href='/'>Back to homepage</a>"
                 "</body></html>",
                 query);

        send(new_socket, response, (int)strlen(response), 0);
        closesocket(new_socket);
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}