//Li, Mengqi: 92059150
//Kim, IL: 36806039

#include <netinet/in.h>
#include "csapp.h"
#define LOGFILE "proxy.log"

/* Global variables */
FILE *logFileFP;

/* Function prototypes */
int parse_uri(char *uri, char *hostname, char *pathname, int *port);
void format_log_entry(char *ipAddress, char *url, unsigned int size);

int main(int argc, char **argv) 
{
	int done = 0;
	if (argc != 2) 
	{
		fprintf(stderr, "Invalid number of arguments, Use port number.\n");
		exit(0);
	}

	int listenFD = Open_listenfd(argv[1]);
	SA *clientIP;
	socklen_t  *lenOfSocketStruct;

	while (1)
	{	
		int serverFD;
		
		printf("Accept connection\n");

		
		int clientFD = Accept(listenFD, clientIP, lenOfSocketStruct);

		printf("Try connect request received\n");
		
		int s_port;
		char buf[MAXLINE], hostname[MAXLINE], pathname[MAXLINE];
		
		ssize_t requestDataSizeReceived = recv(clientFD, buf, MAXLINE, 0);

		char *restOfRequest = malloc(sizeof(buf));
		strcpy(restOfRequest, buf);

		printf("Parsing request\n");
		
		char *httpMethod = strtok(buf, " ");
		char *url = strtok(NULL, " ");
		
        if (url == NULL)
		{
			goto CloseProcess;
        }
		
		char *httpVersion = strtok(NULL, " ");
		parse_uri(url, hostname, pathname, &s_port);

		printf("Try forward request to proxy\n");
		
		struct hostent *h = gethostbyname(hostname);
		
		char *clientHostIPAddress = inet_ntoa(*(struct in_addr*)(h->h_addr_list[0]));
		
		restOfRequest = strpbrk(restOfRequest, "\n") + 1;
		
		printf("Try open connection to server\n");
		

		char str[10];
		sprintf(str, "%d", s_port);
		serverFD = open_clientfd(hostname, str);
		
		printf("Try send request to server\n");
		
		rio_writen(serverFD, "GET /", strlen("GET /"));	
		rio_writen(serverFD, pathname, strlen(pathname));
		rio_writen(serverFD, " HTTP/1.0\r\n", strlen(" HTTP/1.1\r\n"));
		rio_writen(serverFD, restOfRequest, strlen(restOfRequest));
	
		printf("Waiting for response\n");
		
		char serverBuf[MAXLINE];
		rio_t rio_server;
		int n, response_len;
		rio_readinitb(&rio_server, serverFD);
		response_len = 0;

		
		while((n = Rio_readlineb(&rio_server, serverBuf, MAXLINE) ) != 0)
		//while((n = recv(serverFD, serverBuf, MAXLINE, MSG_WAITALL)) > 0 )
		{	
			if (response_len == 0)
				printf("\n###serverBuf:###\n\n");
			printf("\%s", serverBuf);
			response_len += n;
			rio_writen(clientFD, serverBuf, n);
		}
		
		printf("\n\nResponse sent to client\n");
		
		printf("Saving log now\n");
		
		logFileFP = fopen(LOGFILE, "a");
		format_log_entry(clientHostIPAddress, url, requestDataSizeReceived);
		fclose(logFileFP);
		printf("Log saved!\n");
	
	
	CloseProcess:
		printf("Closing server\n");
		close(serverFD);
		printf("Closing client\n");
		close(clientFD);
	}
	exit(0);
}

int parse_uri(char *uri, char *hostname, char *pathname, int *port)
{
	
	char *host_begin;
	char *host_end;
	char *path_begin;
	int differ;

	if (strncasecmp(uri, "http://", 7) != 0)
	{
		hostname[0] = '\0';
		return -1;
	}

	host_begin = uri + 7;
	host_end = strpbrk(host_begin, " :/\r\n\0");
	differ = host_end - host_begin;
	strncpy(hostname, host_begin, differ);
	hostname[differ] = '\0';

	*port = 80;
	
	if (*host_end == ':')
	{
		*port = atoi(host_end + 1);
	}
	path_begin = strchr(host_begin, '/');
	if (path_begin == NULL)
	{
		pathname[0] = '\0';
	}
	else
	{
		path_begin++;
		strcpy(pathname, path_begin);
	}

	return 0;
}

void format_log_entry(char *ipAddress, char *url, unsigned int size)
{
	
	time_t now;
	char time_str[MAXLINE];
	unsigned long host;
	unsigned char a, b, c, d;

	now = time(NULL);
	strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

	fprintf(logFileFP, "%s\t%s\t%-110s\t%zdB\n", time_str, ipAddress, url, size);
}
