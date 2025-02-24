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
	char *log;	//BUFFER TO CONTAIN LOG DATA; MAY BE NULL
	char *message;	//BUFFER TO CONTAIN MESSAGE DATA; MAY BE NULL
	saddrinfo *spai;	//LINKED LIST ADDRINFO FOR SOCKET TRANSMISSION; MAY BE NULL
} capsule;

void *sendlog(void *args);	//THREAD FUNCTION TO SEND LOGS TO NEAREST FELLOW
void *getlog(void *args);	//THREAD FUNCTION TO RETRIEVE INBOUND LOGS
int logalloc(char **log_array, int nextlog);	//CALLED FROM PERFTASK TO ASSIGN NEW VALUE TO NEXTLOG POINTER; DYNAMICALLY ALLOCATES MEMORY TO NEXT AVAILABLE LOGSPACE
int sockserv(int sock, char *hostname, saddrinfo *hints, saddrinfo **res);	//SERVES UP A FUNCTIONAL TCP SOCKET GIVEN A FELLOW HOSTNAME AND USABLE ADDRINFO STRUCTS; RETURNS NULL ON ERROR
char *perftask(char *con, char **log_array, int *nextlog); 	//PERFORM PRIMARY TASK; RETURNS TASKLOG

app_id this_id;

int main(void) {
	
	//DECLARE ESSENTIAL VARIABLES AND ALLOCATE MEMORY FOR TOPLEVEL ARRAY
	char **logarr, *empty_container;
	pthread_t send, get;
	capsule caps, capg;	//WILL FIRST BE PASSED TO SEND THREAD, CONTAINING NECESSARY DATA FOR FELLOW COMMUNICATION; WILL THEN BE MODIFIED BY SEND THREAD AND RETURNED WITH NECESSARY INFORMATION FOR MAIN FUNC
	saddrinfo sai, *spai, kai, *kaip;	//ONE PAIR FOR L_SOCK & A_SOCK; ONE PAIR FOR C_SOCK
	int inc, ssock;
	inc = 0;
	this_id = (app_id) getpid() * 3;
	printf("%s%u%c\n", "APP ID is ", this_id, '.');
	
	if ((logarr = (char **) malloc(sizeof(char *) * MAXLOGS)) == NULL) {	//ALLOCATE MEMORY TO THE ARRAY OF ARRAYS
		perror("MAJOR malloc err");
		exit(1);
	}
	
	//FILL OUT CAPSULES 
	caps.s_id = SENDLOG;
	caps.is_mod = 0;
	caps.aid = this_id;
	caps.log = perftask(empty_container, logarr, &inc);	//ALLOCATE MEMORY FOR ARRAYS IN TOPLEVEL ARRAY ONLY WHEN NECESSARY
	caps.sock = sockserv(ssock, "127.0.0.1", &sai, &spai);	//SERVES UP A FUNCTIONAL SOCKET
	caps.spai = spai;
	memset(&ssock, 0, sizeof(ssock));	//ENSURE THAT SSOCK IS AN EMPTY CONTAINER
	capg.s_id = GETLOG;
	capg.is_mod = 0;
	if ((capg.log = malloc(sizeof(char) * 120)) == NULL) {
		perror("malloc err");
		exit(1);
	}
	if ((capg.message = malloc(sizeof(char) * 120)) == NULL) {
		perror("malloc err");
		exit(1);
	}
	capg.sock = sockserv(ssock, NULL, &kai, &kaip);
	capg.spai = kaip;
	
	//BEGIN AND CONTROL THREADS
	//pthread_create(&send, NULL, sendlog, &caps);
	pthread_create(&get, NULL, getlog, &capg);
	//pthread_join(send, NULL);
	pthread_join(get, NULL);
	if (strcpy(*(logarr + inc - 1), capg.log) == NULL) {
		perror("strcpy err");
		exit(1);
	}
	printf("%s\n", capg.message);
	printf("%s", *(logarr + inc - 1));	//NEWLINE IS TYPICALLY ALREADY DEFINED IN LOG MESSAGE; FIX LATER

	//FREE ALLOCATED MEMORY AND CONCLUDE PROGRAM
	free(capg.log);
	free(capg.message);
	for (int i = 0; i < inc; i++)	//FREE ONLY THOSE LOWLEVEL ARRAYS THAT NEED IT
		free(*(logarr + i));
	free(logarr);	//FREE TOPLEVEL ARRAY, ENSURING THAT THE 2D LOG CONSTRUCT IS PROPERLY UNDONE WITHOUT LEAKS
	freeaddrinfo(spai);
	return 0;

}

void *sendlog(void *capdata) {
	
	char buf[120];
	capsule *cap = (capsule *) capdata;
	saddrinfo *mspai = cap -> spai;
	if (connect(cap -> sock, mspai -> ai_addr, mspai -> ai_addrlen) == -1) {
		perror("con err");
		exit(1);
	}
	if (send(cap -> sock, cap -> log, strlen(cap -> log) + 1, 0) == -1) {
		perror("send err");
		exit(1);
	}
	if (snprintf(buf, sizeof(buf), "%s%s%s%u%c", "Sent log = ", cap -> log, " to fellow ", cap -> aid, '.') == -1) {
		perror("string formatting err");
		exit(1);
	}
	if (strcpy(cap -> message, buf) == NULL) {
		perror("strcpy err");
		exit(1);
	}
	cap -> is_mod = 1;

}

void *getlog(void *capdata) {

	char buf[120], *mes = "Got log from fellow ", *fin_mes;
	int a_sock;
	sstorage crate;
	socklen_t crate_len = sizeof(crate);
	capsule *cap = (capsule *) capdata;
	saddrinfo *mspai = cap -> spai;
	app_id their_id;
	if (bind(cap -> sock, mspai -> ai_addr, mspai -> ai_addrlen) == -1) {
		perror("bind err");
		exit(1);
	}
	if (listen(cap -> sock, 0) == -1) {
		perror("listen err");
		exit(1);
	}
	if ((a_sock = accept(cap -> sock, (struct sockaddr *) &crate, &crate_len)) == -1) {
		perror("accept err");
		exit(1);
	}
	if (recv(a_sock, buf, sizeof(buf), 0) == -1) {
		perror("recv err");
		exit(1);
	}
	cap -> aid = their_id;
	if (strcpy(cap -> log, buf) == NULL) {
		perror("strcpy err");
		exit(1);
	}
	if ((fin_mes = malloc(sizeof(char) * 30)) == NULL) {
		perror("malloc err");
		exit(1);
	}
	if (snprintf(fin_mes, 30, "%s%u%c", mes, cap -> aid, '.') == -1) {
		perror("string formatting err");
		exit(1);
	}
	if (strcpy(cap -> message, fin_mes) == NULL) {
		perror("strcpy err");
		exit(1);
	}
	if (close(a_sock) == -1) {
		perror("sock close err");
		exit(1);
	}
	free(fin_mes);

}

int logalloc(char **logs, int n) {

	if ((*(logs + n) = malloc(sizeof(char) * MAXLOG_LEN)) == NULL) {	
		perror("malloc err");			
		exit(1);
	}
	return ++n;

}

int sockserv(int sock, char *host, saddrinfo *sai, saddrinfo **spai) {

	int temp = 1;
	memset(sai, 0, sizeof(*sai));
	sai -> ai_family = AF_INET;
	sai -> ai_socktype = SOCK_STREAM;
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

char *perftask(char *log, char **logs, int *nptr) {

	FILE* checkf;
	clock_t start, end;
	float timespent;
	if ((log = malloc(sizeof(char) * 90)) == NULL) {
		perror("malloc err");
		exit(1);
	}
	start = clock();
	system("ping -c 1 www.worcesterschools.org > ping_info");
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
