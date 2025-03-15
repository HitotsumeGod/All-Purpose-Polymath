#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include "app_util.h"

mapping *chart(capsule *cap, int n, char *argv[]) {             //GODDAMN BEAUTIFUL

        mapping *head, *swap, *point;
	saddrinfo msai, *mspai;
        int temp, errcode;

	temp = 1;
        if ((point = malloc(sizeof(mapping))) == NULL) {
                perror("malloc err");
                exit(1);
        }
        head = point;
        cap -> sai.ai_family = AF_INET;
        cap -> sai.ai_socktype = SOCK_STREAM;
	cap -> sai.ai_flags = AI_PASSIVE;
        msai = cap -> sai;
        mspai = cap -> spai; //DEREFERENCE SPAI INTO AN EASIER FORM
        for (int i = 1; i < n; i++) {           //CREATES LINKED LIST OF ARBITRARY LENGTH USING A SWAP STRUCT
                if ((errcode = getaddrinfo(argv[i], PORT, &msai, &mspai)) != 0) {
			printf("getaddrinfo error : %s\n", gai_strerror(errcode));
			exit(EXIT_FAILURE);
		}
                if ((cap -> sock = socket(mspai -> ai_family, mspai -> ai_socktype, mspai -> ai_protocol)) == -1) {
                        perror("sock err");
                        exit(1);
                }
                if (setsockopt(cap -> sock, SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(int)) == -1) {
                        perror("setsock err");
                        exit(1);
                }
                if (connect(cap -> sock, mspai -> ai_addr, mspai -> ai_addrlen) == -1) {
                        point -> ipaddr = argv[i];
                        point -> aid = get_appid(argv[i]);
                } else {
                        point -> ipaddr = NULL;
                }
                if (close(cap -> sock) == -1) {
                        perror("sock close err");
                        exit(1);
                }
                freeaddrinfo(mspai);
                if ((point -> next = malloc(sizeof(mapping))) == NULL) {
                        perror("malloc err");
                        exit(1);
                }
                swap = point;
                point = swap -> next;
        }
        point = NULL;           //MARK THE END OF THE LIST
        return head;
    
}
