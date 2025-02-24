#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>

#define PORT "6666"
#define MAXLOGS 20
#define MAXLOG_LEN 100
#define TEMP 2

enum datatype {SENDLOG, GETLOG};

typedef unsigned int app_id;
typedef struct addrinfo saddrinfo;
typedef struct sockaddr_storage sstorage;
typedef struct {
	enum datatype s_id;	//ENUM IDENTIFIER THAT DETERMINES WHETHER THE STRUCT CONTAINS SENDLOG DATA OR GETLOG DATA; MAY NOT BE NULL
	int is_mod;	//BOOLEAN THAT IS SET TO 'TRUE' IF THE STRUCT IS MODIFIED BY THE MODIFYING AGENT; INFORMS THE PROGRAM WHETHER OR NOT THE STRUCT IS BEING SENT BACK FROM A THREAD; MAY NOT BE NULL
	int sock;	//SOCKET USED FOR TRANSMISSION OF INFORMATION; MUST BE PREPPED WITH GETADDRINFO PRIOR TO INSTANTIATION; MAY BE NULL 
	app_id aid;	//APP_ID IDENTIFIER; EITHER EXPORTED TO OUTBOUND FELLOW IN SENDLOG, OR RETRIEVED FROM INBOUND FELLOW IN GETLOG; MAY BE NULL
	char *buffer;	//BUFFER TO CONTAIN CHARACTER DATA; MAY BE NULL
	saddrinfo *spai;	//LINKED LIST ADDRINFO FOR SOCKET TRANSMISSION; MAY BE NULL
} capsule;

void *sendlog(void *args);	//THREAD FUNCTION TO SEND LOGS TO NEAREST FELLOW
void *getlog(void *args);	//THREAD FUNCTION TO RETRIEVE INBOUND LOGS
int logalloc(char **log_array, int nextlog);	//CALLED FROM PERFTASK TO ASSIGN NEW VALUE TO NEXTLOG POINTER; DYNAMICALLY ALLOCATES MEMORY TO NEXT AVAILABLE LOGSPACE
int sockserv(char *hostname, saddrinfo *hints, saddrinfo **res);	//SERVES UP A FUNCTIONAL TCP SOCKET GIVEN A FELLOW HOSTNAME AND USABLE ADDRINFO STRUCTS; RETURNS NULL ON ERROR
char *perftask(char *con, char **log_array, int *nextlog); 	//PERFORM PRIMARY TASK; RETURNS TASKLOG

int main(void) {
	
	char **logarr, *empty_container;
	pthread_t send, get;
	capsule cap;	//WILL FIRST BE PASSED TO SEND THREAD, CONTAINING NECESSARY DATA FOR FELLOW COMMUNICATION; WILL THEN BE MODIFIED BY SEND THREAD AND RETURNED WITH NECESSARY INFORMATION FOR MAIN FUNC
	saddrinfo sai, *spai, kai, *kaip;	//ONE PAIR FOR L_SOCK & A_SOCK; ONE PAIR FOR C_SOCK
	int inc, l_sock, a_sock, c_sock;	//LISTENER SOCKET, ACCEPTANCE SOCKET, COMMUNICATIONS SOCKET
	inc = 0;
	if ((l_sock = sockserv(NULL, &sai, &spai)) == -1) {
		perror("socket servup err");
		exit(1);
	}
	

	if ((logarr = (char **) malloc(sizeof(char *) * MAXLOGS)) == NULL) {	//ALLOCATE MEMORY TO THE ARRAY OF ARRAYS
		perror("MAJOR malloc err");
		exit(1);
	}

	cap.s_id = GETLOG;
	cap.is_mod = 0;
	cap.aid = TEMP;
	cap.buffer = perftask(empty_container, logarr, &inc);	//ALLOCATE MEMORY FOR ARRAYS IN TOPLEVEL ARRAY ONLY WHEN NECESSARY
	cap.sock = l_sock;
	cap.spai = spai;
	if (cap.s_id == GETLOG)		//IF OBTAINED A LOG TO BE STORED, COPIES INFORMATION FROM BUFFER TO NEXT AVAILABLE LOGSPACE;
		if (strcpy(*(logarr + inc - 1), cap.buffer) == NULL) {
			perror("strcpy err");
			exit(1);
		}
	
	pthread_create(&send, NULL, sendlog, &cap);
	pthread_join(send, NULL);
	printf("%d\n", inc);
	printf("%s\n", *(logarr + inc - 1));

	for (int i = 0; i < inc; i++)	//FREE ONLY THOSE LOWLEVEL ARRAYS THAT NEED IT
		free(*(logarr + i));
	freeaddrinfo(spai);
	free(logarr);	//FREE TOPLEVEL ARRAY, ENSURING THAT THE 2D LOG CONSTRUCT IS PROPERLY UNDONE WITHOUT LEAKS
	return 0;

}

void *sendlog(void *capdata) {
	
	char buf[100];
	capsule *cap = (capsule *) capdata;
	saddrinfo *mspai = cap -> spai;
	cap -> is_mod = 1;
	if (snprintf(buf, sizeof(buf), "%s%s", "Sent log = ", cap -> buffer) == -1) {
		perror("string formatting err");
		exit(1);
	}
	if (strcpy(cap -> buffer, buf) == NULL) {
		perror("strcpy err");
		exit(1);
	}

}

void *getlog(void *capdata) {

	char buf[100];
	int a_sock;
	sstorage crate;
	socklen_t crate_len = sizeof(crate);
	capsule *cap = (capsule *) capdata;
	saddrinfo *mspai = cap -> spai;
	if (bind(cap -> sock, mspai -> ai_addr, mspai -> ai_addrlen) == -1) {
		perror("bind err");
		exit(1)
	}
	if (listen(cap -> sock, 0) == -1) {
		perror("listen err");
		exit(1);
	}
	if ((a_sock = accept(cap -> sock, (struct sockaddr *) &crate, &crate_len)) == NULL) {
		perror("accept err");
		exit(1);
	}
	if (fclose(a_sock) == -1) {
		perror("sock close err");
		exit(1);
	}
	if (fclose(cap -> sock) == -1) {
		perror("sock close err");
		exit(1);
	}

}

int logalloc(char **logs, int n) {

	if ((*(logs + n) = malloc(sizeof(char) * MAXLOG_LEN)) == NULL) {	
		perror("malloc err");			
		exit(1);
	}
	return ++n;

}

int sockserv(char *host, saddrinfo *sai, saddrinfo **spai) {

	memset(sai, 0, sizeof(*sai));
	sai -> ai_family = AF_INET;
	sai -> ai_socktype = SOCK_STREAM;
	sai -> ai_flags = AI_PASSIVE;
	getaddrinfo(host, PORT, sai, spai);
	return socket(sai -> ai_family, sai -> ai_socktype, sai -> ai_protocol);

}

char *perftask(char *log, char **logs, int *nptr) {

	FILE* checkf;
	clock_t start, end;
	float timespent;
	if ((log = malloc(sizeof(char) * 90)) == NULL) {
		perror("malloc err");
		exit(1);
	}
	start = clock();
	system("ping -c 1 www.google.com > ping_info");
	if ((checkf = fopen("ping_info", "r")) == NULL)
	       log = "Failed execution of ping task on target 'www.google.com'.";	
	end = clock();
	timespent = (float) 1000 * ((end - start) / CLOCKS_PER_SEC);	//execution time in milliseconds
	if (snprintf(log, 90, "Completed execution of ping task on target 'www.google.com' in %f milliseconds.", timespent) == -1) {
		perror("string formatting err");
		exit(1);
	}
	*nptr = logalloc(logs, *nptr);
	return log;

}
