#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include "app_util.h"

int sockserv(int sock, int opts, char *host, saddrinfo *sai, saddrinfo **spai) {

	int temp = 1;
	memset(sai, 0, sizeof(*sai));
	sai -> ai_family = AF_INET;
	sai -> ai_socktype = SOCK_STREAM;
	if (opts == 1)
		sai -> ai_flags = AI_PASSIVE;
	getaddrinfo(host, PORT, sai, spai);
	if ((sock = socket(sai -> ai_family, sai -> ai_socktype, sai -> ai_protocol)) == -1) {
		perror("sock err");
		exit(1);
	}
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(int)) == -1) {
            perror("setsock err");
            exit(1);
        }
	return sock;

}
