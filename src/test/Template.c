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
#include <pthread.h>

#define SENDTYPE 's'
#define GETTYPE 'g'
#define TEMP 2

typedef unsigned int app_id;
typedef struct {
	char s_id;	//CHAR IDENTIFIER THAT DETERMINES WHETHER THE STRUCT CONTAINS SENDLOG DATA OR GETLOG DATA; MAY NOT BE NULL
	int is_mod;	//BOOLEAN THAT IS SET TO 'TRUE' IF THE STRUCT IS MODIFIED BY THE MODIFYING AGENT; INFORMS THE PROGRAM WHETHER OR NOT THE STRUCT IS BEING SENT BACK FROM A THREAD; MAY NOT BE NULL
	app_id aid;	//APP_ID IDENTIFIER; EITHER EXPORTED TO OUTBOUND FELLOW IN SENDLOG, OR RETRIEVED FROM INBOUND FELLOW IN GETLOG; MAY BE NULL
	char *buffer;	//BUFFER TO CONTAIN CHARACTER DATA; MAY BE NULL
} log_data;

void *sendlog(void *args);	//THREAD FUNCTION TO SEND LOGS TO NEAREST FELLOW
void *getlog(void *args);	//THREAD FUNCTION TO RETRIEVE INBOUND LOGS
char *perftask(void); 	//PERFORM PRIMARY TASK; RETURNS TASKLOG

int main(void) {

	pthread_t send, get;
	log_data ld;	//WILL FIRST BE PASSED TO SEND THREAD, CONTAINING NECESSARY DATA FOR FELLOW COMMUNICATION; WILL THEN BE MODIFIED BY SEND THREAD AND RETURNED WITH NECESSARY INFORMATION FOR MAIN FUNC
	ld.s_id	= SENDTYPE;
	ld.is_mod = 0;
	ld.aid = TEMP;
	//ld.buffer = perftask();
	pthread_create(&send, NULL, sendlog, &ld);
	pthread_join(send, NULL);
	printf("%i\n", ld.is_mod);
	return 0;

}

void *sendlog(void *logdata) {

	log_data *ld = (log_data *) logdata;
	ld -> is_mod = 1;

}
