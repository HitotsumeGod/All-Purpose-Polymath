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

typedef unsigned int app_id;

app_id sendlog(app_id fellow_id, char type_id, char *log);	//pass own app_id, id char representing inbound type and buffer containing contents of log; sends data to nearest fellow; returns fellow id ON SUCCESS, -1 ON FAILURE
app_id getlog(char *log);	//PASS BUFFER CONTAINING CONTENTS OF LOG; RETRIEVES INBOUND DATA; RETURNS FELLOW ID ON SUCCESS, -1 ON FALURE
int perftask(void); 	//PERFORM PRIMARY TASK; RETURNS NON-NEGATIVE INT ON SUCCESS, NEGATIVE INT ON FAILURE

int main(void) {

	return 0;

}
