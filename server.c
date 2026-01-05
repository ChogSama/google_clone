#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024

// Decode URL-encoded strings (e.g. %20 -> space)
void url_decode(char *dst, const char *src)
{
    char a, b;
    while (*src)
    {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit(a) && isxdigit(b)))
        {
            if (a >= 'a')
                a -= 'a' - 'A';
            if (a >= 'A')
                a -= ('A' - 10);
            else
                a -= '0';

            if (b >= 'a')
                b -= 'a' - 'A';
            if (b >= 'A')
                b -= ('A' - 10);
            else
                b -= '0';

            *dst++ = 16 * a + b;
            src += 3;
        }
        else if (*src == '+')
        {
            *dst++ = ' ';
            src++;
        }
        else
        {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

void highlight(char *dst, const char *src, const char *word)
{
    const char *p = src;
    size_t wlen = strlen(word);

    while (*p)
    {
        if (strncasecmp(p, word, wlen) == 0)
        {
            strcat(dst, "<mark>");
            strncat(dst, p, wlen);
            strcat(dst, "</mark>");
            p += wlen;
        }
        else
        {
            size_t len = strlen(dst);
            dst[len] = *p;
            dst[len + 1] = '\0';
            p++;
        }
    }
}

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

        char file_buffer[BUFFER_SIZE * 4] = {0};
        FILE *file = NULL;

        // Determine which file to serve
        if (strstr(buffer, "GET / ") != NULL || strstr(buffer, "GET /index.html") != NULL)
        {
            file = fopen("index.html", "r");
        }
        else if (strstr(buffer, "GET /search?q=") != NULL)
        {
            file = fopen("search.html", "r");
        }

        if (file)
        {
            size_t n = fread(file_buffer, 1, sizeof(file_buffer) - 1, file);
            file_buffer[n] = '\0';
            fclose(file);

            // If search, replace {{query}} with actual query
            char *q_ptr = strstr(buffer, "GET /search?q=");
            if (q_ptr)
            {
                q_ptr += strlen("GET /search?q=");
                char raw_query[256] = "";
                char query[256] = "";
                char *space_ptr = strstr(q_ptr, " ");
                if (space_ptr)
                {
                    int len = space_ptr - q_ptr;
                    if (len > 255)
                        len = 255;
                    strncpy(raw_query, q_ptr, len);
                    raw_query[len] = '\0';

                    // Decode URL-encoded query
                    url_decode(query, raw_query);

                    // Split query into words
                    char query_copy[256];
                    strcpy(query_copy, query);

                    char *words[10];
                    int word_count = 0;

                    char *token = strtok(query_copy, " ");
                    while (token && word_count < 10)
                    {
                        words[word_count++] = token;
                        token = strtok(NULL, " ");
                    }

                    // Generate multiple fake search results dynamically
                    char results[BUFFER_SIZE * 2] = ""; // buffer for generated results
                    for (int i = 1; i <= 5; i++)        // generate 5 fake results
                    {
                        char highlighted[512];
                        strcpy(highlighted, query);

                        for (int w = 0; w < word_count; w++)
                        {
                            char temp2[512] = "";
                            highlight(temp2, highlighted, words[w]); // apply on previous result
                            strcpy(highlighted, temp2);
                        }

                        char temp[512];
                        snprintf(temp, sizeof(temp),
                                 "<div class='result'>"
                                 "<div class='title'>Result %d for %s</div>"
                                 "<div class='url'>https://www.example%d.com</div>"
                                 "<div class='snippet'>This is a fake snippet describing result %d for %s.</div>"
                                 "</div>",
                                 i, highlighted, i, i, highlighted);
                        strcat(results, temp);
                    }

                    // Replace {{query}} placeholder in file_buffer
                    char final_response[BUFFER_SIZE * 4] = {0};
                    char *pos = strstr(file_buffer, "{{query}}");
                    if (pos)
                    {
                        int before_len = pos - file_buffer;
                        snprintf(final_response, sizeof(final_response), "%.*s%s%s", before_len, file_buffer, query, pos + strlen("{{query}}"));
                        strcpy(file_buffer, final_response);
                    }

                    // Replace {{results}} placeholder with dynamically generated results
                    pos = strstr(file_buffer, "{{results}}");
                    if (pos)
                    {
                        int before_len = pos - file_buffer;
                        char temp_response[BUFFER_SIZE * 4] = {0};
                        snprintf(temp_response, sizeof(temp_response), "%.*s%s%s", before_len, file_buffer, results, pos + strlen("{{results}}"));
                        strcpy(file_buffer, temp_response);
                    }
                }
            }

            char header[256];
            snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
            send(new_socket, header, (int)strlen(header), 0);
            send(new_socket, file_buffer, (int)strlen(file_buffer), 0);
        }
        else
        {
            const char *notfound = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n<h1>404 Not Found</h1>";
            send(new_socket, notfound, (int)strlen(notfound), 0);
        }

        closesocket(new_socket);
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}