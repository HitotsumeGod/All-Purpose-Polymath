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
#include <sys/ioctl.h>
#include <net/if.h>

#define PORT "6666"
#define MAXLOGS 20
#define MAXLOG_LEN 100
#define TEMP 2

enum datatype {SENDLOG, GETLOG, MAP};

typedef unsigned int app_id;
typedef struct addrinfo saddrinfo;
typedef struct sockaddr_storage sstorage;
typedef struct mapping {
	char *ipaddr;	//FELLOW IP ADDRESS
	app_id aid;	//FELLOW APP_ID
	struct mapping *next;	//NEXT NODE
} mapping;
typedef struct {
	enum datatype s_id;	//ENUM IDENTIFIER THAT DETERMINES WHETHER THE STRUCT CONTAINS SENDLOG DATA OR GETLOG DATA; MAY NOT BE NULL
	int is_mod;	//BOOLEAN THAT IS SET TO 'TRUE' IF THE STRUCT IS MODIFIED BY THE MODIFYING AGENT; INFORMS THE PROGRAM WHETHER OR NOT THE STRUCT IS BEING SENT BACK FROM A THREAD; MAY NOT BE NULL
	int sock;	//SOCKET USED FOR TRANSMISSION OF INFORMATION; MUST BE PREPPED WITH GETADDRINFO PRIOR TO INSTANTIATION; MAY BE NULL 
	app_id aid;	//APP_ID IDENTIFIER; EITHER EXPORTED TO OUTBOUND FELLOW IN SENDLOG, OR RETRIEVED FROM INBOUND FELLOW IN GETLOG; MAY BE NULL
	char *log;	//BUFFER TO CONTAIN LOG DATA; MAY BE NULL
	char *message;	//BUFFER TO CONTAIN MESSAGE DATA; MAY BE NULL
	saddrinfo sai;
	saddrinfo *spai;	//LINKED LIST ADDRINFO FOR SOCKET TRANSMISSION; MAY BE NULL
} capsule;

void *sendlog(void *args);	//THREAD FUNCTION TO SEND LOGS TO NEAREST FELLOW
void *getlog(void *args);	//THREAD FUNCTION TO RETRIEVE INBOUND LOGS
int logalloc(char **log_array, int nextlog);	//CALLED FROM PERFTASK TO ASSIGN NEW VALUE TO NEXTLOG POINTER; DYNAMICALLY ALLOCATES MEMORY TO NEXT AVAILABLE LOGSPACE
int sockserv(int sock, int opts, char *hostname, saddrinfo *hints, saddrinfo **res);	//SERVES UP A FUNCTIONAL TCP SOCKET GIVEN A FELLOW HOSTNAME AND USABLE ADDRINFO STRUCTS; RETURNS NULL ON ERROR
char *perftask(char *con, char **log_array, int *nextlog); 	//PERFORM PRIMARY TASK; RETURNS TASKLOG
char *getmyhostname();	//OBTAINS THIS FELLOW'S HOSTNAME TO BE FED INTO GET_APPID
mapping *chart(capsule *cap, int nargs, char *hostnames[]); 	//DETERMINES WHETHER OR NOT PASSED FELLOW IPS RESOLVE TO ACTIVE FELLOWS; RETURNS MAPPING LL FILLED OUT WITH SAID INFO; MOST IMPORTANT PART OF THE ENTIRE SYSTEM
app_id get_appid(char *hostname);	//GIVEN IP ADDRESS, RETURNS CORRESPONDING APP_ID 

app_id this_id;

int main(int argc, char *argv[]) {

	sleep(1);
	if (argc < 2) {
		printf("%s\n", "Improper format. Please provide at least one fellow IP Address to map onto.");
		exit(1);
	}	
	//DECLARE ESSENTIAL VARIABLES AND ALLOCATE MEMORY FOR TOPLEVEL ARRAY
	char **logarr, *task, *empty_container;
	pthread_t send, get;
	mapping *themap, *dummy;
	capsule mcap, caps, capg;	//WILL FIRST BE PASSED TO SEND THREAD, CONTAINING NECESSARY DATA FOR FELLOW COMMUNICATION; WILL THEN BE MODIFIED BY SEND THREAD AND RETURNED WITH NECESSARY INFORMATION FOR MAIN FUNC
	saddrinfo sai, *spai, kai, *kaip;	//ONE PAIR FOR L_SOCK & A_SOCK; ONE PAIR FOR C_SOCK
	int inc, ssock;
	inc = 0;
	this_id = get_appid(getmyhostname());
	printf("%s%u%c\n", "APP ID is ", this_id, '.');

	if ((logarr = (char **) malloc(sizeof(char *) * MAXLOGS)) == NULL) {	//ALLOCATE MEMORY TO THE ARRAY OF ARRAYS
		perror("MAJOR malloc err");
		exit(1);
	}
	if ((task = malloc(sizeof(char) * 9)) == NULL) {
		perror("malloc err");
		exit(1);
	}
	
	//FILL OUT CAPSULES 
	mcap.s_id = MAP;
	mcap.is_mod = 0;
	mcap.aid = this_id;
	mcap.log = NULL;
	mcap.message = NULL;
	mcap.sock = -1;		//SINCE MULTIPLE SOCKETS WILL HAVE TO BE CREATED, KEEP MCAP SOCK OBSOLETE UNTIL MAPPING()
	if ((themap = chart(&mcap, argc, argv)) == NULL) {
		perror("mapit err");
		exit(1);
	}
	/*for (int i = 0; i < argc - 1; i++) {	//PRINT MAPPED FELLOWS' ADDRESSES
		printf("%s\n", themap -> ipaddr);
		printf("%u\n", themap -> aid);
		themap = themap -> next;
	}*/
	task = perftask(empty_container, logarr, &inc);
	caps.s_id = SENDLOG;
	caps.is_mod = 0;
	caps.aid = this_id;
	caps.log = task;
	if ((caps.message = malloc(sizeof(char) * 120)) == NULL) {
		perror("malloc err");
		exit(1);
	}
	caps.sock = sockserv(ssock, 0, themap -> ipaddr, &sai, &spai);	//SERVES UP A FUNCTIONAL SOCKET; MAPPING MUST BE FILLED OUT TO WORK
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
	capg.sock = sockserv(ssock, 1, NULL, &kai, &kaip);
	capg.spai = kaip;
	
	//BEGIN AND CONTROL THREADS
	pthread_create(&send, NULL, sendlog, &caps);
	pthread_create(&get, NULL, getlog, &capg);
	pthread_join(send, NULL);
	pthread_join(get, NULL);
	if (strcpy(*(logarr + inc - 1), capg.log) == NULL) {
		perror("strcpy err");
		exit(1);
	}
	printf("%s\n", capg.message);
	printf("%s\n", *(logarr + inc - 1));	//NEWLINE IS TYPICALLY ALREADY DEFINED IN LOG MESSAGE; FIX LATER

	//FREE ALLOCATED MEMORY AND CONCLUDE PROGRAM
	if (close(caps.sock) == -1) {
		perror("sclose err");
		exit(1);
	}
	if (close(capg.sock) == -1) {
		perror("sclose err");
		exit(1);
	}
	free(task);
	free(capg.log);
	free(capg.message);
	for (int i = 0; i < inc; i++)	//FREE ONLY THOSE LOWLEVEL ARRAYS THAT NEED IT
		free(*(logarr + i));
	free(logarr);	//FREE TOPLEVEL ARRAY, ENSURING THAT THE 2D LOG CONSTRUCT IS PROPERLY UNDONE WITHOUT LEAKS
	freeaddrinfo(spai);
	return 0;

}

void *sendlog(void *capdata) {
	
	char buf[120], *ipinfo;
	size_t *inc_size;
	capsule *cap = (capsule *) capdata;
	saddrinfo *mspai = cap -> spai;
	if ((inc_size = malloc(sizeof(size_t))) == NULL) {
		perror("malloc err");
		exit(EXIT_FAILURE);
	}
	while (connect(cap -> sock, mspai -> ai_addr, mspai -> ai_addrlen) == -1) {
		//perror("con err");
		sleep(1);
	}
	printf("%s\n", "MAILMAN: Connected to host.");
	if (recv(cap -> sock, inc_size, sizeof(int), 0) == -1) {
		perror("mailman recv err");
		exit(1);
	}
	printf("Inc Size is %zu\n", *inc_size);
	if ((ipinfo = malloc(sizeof(char) * (*inc_size + 1))) == NULL) {
		perror("malloc err");
		exit(EXIT_FAILURE);
	}
	if (recv(cap -> sock, ipinfo, *inc_size, 0) == -1) {
		perror("mailman recv err");
		exit(1);
	}
	*(ipinfo + *inc_size + 1) = '\0';
	printf("Fellow IP is : %s\n", ipinfo);
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
	free(inc_size);

}

void *getlog(void *capdata) {

	char buf[120], *ipinfo, *mes = "MAILBOX: Got log from fellow ", *fin_mes;
	int a_sock;
	size_t *info_size; 
	sstorage crate;
	socklen_t crate_len = sizeof(crate);
	capsule *cap = (capsule *) capdata;
	saddrinfo *mspai = cap -> spai;
	app_id their_id;
	if ((info_size = malloc(sizeof(size_t))) == NULL) {
		perror("mailbox malloc err");
		exit(1);
	}
	*info_size = strlen(getmyhostname());
	if ((ipinfo = malloc(sizeof(char) * *info_size)) == NULL) {
		perror("malloc err");
		exit(EXIT_FAILURE);
	}
	strcpy(ipinfo, getmyhostname());
	if (bind(cap -> sock, mspai -> ai_addr, mspai -> ai_addrlen) == -1) {
		perror("mailbox bind err");
		exit(1);
	}
	if (listen(cap -> sock, 0) == -1) {
		perror("mailbox listen err");
		exit(1);
	}
	while ((a_sock = accept(cap -> sock, (struct sockaddr *) &crate, &crate_len)) == -1) {
		perror("mailbox accept err");
		sleep(1);
	}
	printf("%s\n", "MAILBOX: Connection accepted.");
	if (send(a_sock, info_size, sizeof(size_t), 0) == -1) {
		perror("mailbox send 1err");
		exit(1);
	}
	if (send(a_sock, ipinfo, *info_size, 0) == -1) {
		perror("mailbox send 2err");
		exit(1);
	}
	if (recv(a_sock, buf, sizeof(buf), 0) == -1) {
		perror("mailbox recv err");
		exit(1);
	}
	cap -> aid = their_id;
	if (strcpy(cap -> log, buf) == NULL) {
		perror("mailbox strcpy err");
		exit(1);
	}
	if ((fin_mes = malloc(sizeof(char) * 30)) == NULL) {
		perror("mailbox malloc err");
		exit(1);
	}
	if (snprintf(fin_mes, 30, "%s%u%c", mes, cap -> aid, '.') == -1) {
		perror("mailbox string formatting err");
		exit(1);
	}
	if (strcpy(cap -> message, fin_mes) == NULL) {
		perror("mailbox strcpy err");
		exit(1);
	}
	if (close(a_sock) == -1) {
		perror("mailbox sock close err");
		exit(1);
	}
	free(info_size);
	free(fin_mes);

}

int logalloc(char **logs, int n) {

	if ((*(logs + n) = malloc(sizeof(char) * MAXLOG_LEN)) == NULL) {	
		perror("malloc err");			
		exit(1);
	}
	return ++n;

}

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

char *getmyhostname() {		//CODE COPIED FROM https://www.geekpage.jp/en/programming/linux-network/get-ipaddr.php WITH A FEW MODIFICATIONS

	int fd;
 	struct ifreq ifr;
 	fd = socket(AF_INET, SOCK_DGRAM, 0);

 	/* I want to get an IPv4 IP address */
 	ifr.ifr_addr.sa_family = AF_INET;

 	/* I want IP address attached to "eth0" */
 	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);

 	ioctl(fd, SIOCGIFADDR, &ifr);

 	close(fd);

 	return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);

}

mapping *chart(capsule *cap, int n, char *argv[]) {		//GODDAMN BEAUTIFUL	

	mapping *head, *swap, *point;
	int temp = 1;
	if ((point = malloc(sizeof(mapping))) == NULL) {
		perror("malloc err");
		exit(1);
	}
	head = point;
	cap -> sai.ai_family = AF_INET;
	cap -> sai.ai_socktype = SOCK_STREAM;
	saddrinfo msai = cap -> sai;
	saddrinfo *mspai = cap -> spai;	//DEREFERENCE SPAI INTO AN EASIER FORM
	for (int i = 1; i < n; i++) {		//CREATES LINKED LIST OF ARBITRARY LENGTH USING A SWAP STRUCT
		getaddrinfo(argv[i], PORT, &msai, &mspai);
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
	point = NULL;		//MARK THE END OF THE LIST
	return head;
    
}

app_id get_appid(char *ip) {	//IPV4 ADDR TO APP_ID CONVERTER

	char c = 'a';	//PLACEHOLDER CHAR
	int i = 0, test;
	app_id fin = 0;
	while (c != '\0' && c != '\n') {
		c = *(ip + i);
		test = c - '0';
		if (test == 1 || test == 2 || test == 3 || test == 4 || test == 5 || test == 6 || test == 7 || test == 8 || test == 9)
			fin += test;
		i++;
	}
	return fin;

}
