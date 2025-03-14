#ifndef APP_UTIL_H
#define APP_UTIL_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#define PORT "6666"
#define MAXLOGS 20
#define MAXLOG_LEN 100
#define TEMP 2

enum datatype {SENDLOG, GETLOG, MAP};

typedef unsigned int app_id;
typedef struct addrinfo saddrinfo;
typedef struct sockaddr_storage sstorage;
typedef struct mapping {
        char *ipaddr;   //FELLOW IP ADDRESS
        app_id aid;     //FELLOW APP_ID
        struct mapping *next;   //NEXT NODE
} mapping;
typedef struct {
        enum datatype s_id;     //ENUM IDENTIFIER THAT DETERMINES WHETHER THE STRUCT CONTAINS SENDLOG DATA OR GETLOG DATA; MAY NOT BE NULL
        int is_mod;     //BOOLEAN THAT IS SET TO 'TRUE' IF THE STRUCT IS MODIFIED BY THE MODIFYING AGENT; INFORMS THE PROGRAM WHETHER OR NOT THE STRUCT IS BEING SENT BACK FROM A THREAD; MAY NOT BE NULL
        int sock;       //SOCKET USED FOR TRANSMISSION OF INFORMATION; MUST BE PREPPED WITH GETADDRINFO PRIOR TO INSTANTIATION; MAY BE NULL 
        app_id aid;     //APP_ID IDENTIFIER; EITHER EXPORTED TO OUTBOUND FELLOW IN SENDLOG, OR RETRIEVED FROM INBOUND FELLOW IN GETLOG; MAY BE NULL
        char *log;      //BUFFER TO CONTAIN LOG DATA; MAY BE NULL
        char *message;  //BUFFER TO CONTAIN MESSAGE DATA; MAY BE NULL
        saddrinfo sai;
        saddrinfo *spai;        //LINKED LIST ADDRINFO FOR SOCKET TRANSMISSION; MAY BE NULL
} capsule;

extern char *perftask(char *con, char **log_array, int *nextlog);      //PERFORM PRIMARY TASK; RETURNS TASKLOG
extern int logalloc(char **log_array, int nextlog); //CALLED FROM PERFTASK TO ASSIGN NEW VALUE TO NEXTLOG POINTER; DYNAMICALLY ALLOCATES MEMORY TO NEXT AVAILABLE LOGSPACE
extern int sockserv(int sock, int opts, char *hostname, saddrinfo *hints, saddrinfo **res);    //SERVES UP A FUNCTIONAL TCP SOCKET GIVEN A FELLOW HOSTNAME AND USABLE ADDRINFO STRUCTS; RETURNS NULL ON ERROR
extern char *getmyhostname();  //OBTAINS THIS FELLOW'S HOSTNAME TO BE FED INTO GET_APPID
extern mapping *chart(capsule *cap, int nargs, char *hostnames[]);     //DETERMINES WHETHER OR NOT PASSED FELLOW IPS RESOLVE TO ACTIVE FELLOWS; RETURNS MAPPING LL FILLED OUT WITH SAID INFO; MOST IMPORTANT PART OF THE ENTIRE SYSTEM
extern app_id get_appid(char *hostname);       //GIVEN IP ADDRESS, RETURNS CORRESPONDING APP_ID

#endif //APP_UTIL_H
