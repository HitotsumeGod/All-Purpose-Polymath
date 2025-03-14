#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <ifaddrs.h>

#include "app_util.h"

char *getmyhostname() {

        struct ifaddrs *my_interface, *pointer;
        struct sockaddr_in addressin;
        if (getifaddrs(&my_interface) == -1) {
                perror("interface grab err");
                exit(EXIT_FAILURE);
        }
        if ((pointer = malloc(sizeof my_interface)) == NULL) {
                perror("malloc err");
                exit(EXIT_FAILURE);
        }
        memcpy(pointer, my_interface, sizeof my_interface);
        freeifaddrs(my_interface);
        while (pointer != NULL) {
                if (strcmp("eth0", pointer -> ifa_name) == 0 || strcmp("wlan0", pointer -> ifa_name) == 0 || strcmp("wlp2s0", pointer -> ifa_name) == 0) {
                        if (*(pointer -> ifa_addr -> sa_data) == '1')
                                return pointer -> ifa_addr -> sa_data;          //RETURN FIRST LEGITIMATE ADDRESS
                }
                pointer = pointer -> ifa_next;
        }
        return NULL;    //NO INTERFACES DETECTED
}

app_id get_appid(char *ip) {    //IPV4 ADDR TO APP_ID CONVERTER

        char c = 'a';   //PLACEHOLDER CHAR
        int i = 0, test;
        app_id fin = 0;
        if (ip == NULL) {
                printf("%s\n", "No valid interface detected. Exiting.");
                exit(EXIT_FAILURE);
        }
        while (c != '\0' && c != '\n') {
                c = *(ip + i);
                test = c - '0';
                if (test == 1 || test == 2 || test == 3 || test == 4 || test == 5 || test == 6 || test == 7 || test == 8 || test == 9)
                        fin += test;
                i++;
        }
        return fin;

}
