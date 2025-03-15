#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "app_util.h"

void *sendlog(void *args);	//THREAD FUNCTION TO SEND LOGS TO NEAREST FELLOW
void *getlog(void *args);	//THREAD FUNCTION TO RETRIEVE INBOUND LOGS

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
	while (1) {
	if ((logarr = (char **) malloc(sizeof(char *) * MAXLOGS)) == NULL) {	//ALLOCATE MEMORY TO THE ARRAY OF ARRAYS
		perror("MAJOR malloc err");
		exit(1);
	}
	if ((task = malloc(sizeof(char) * 9)) == NULL) {
		perror("malloc err");
		exit(1);
	}
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
	}
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
