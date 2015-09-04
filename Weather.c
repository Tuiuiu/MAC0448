/* Standard headers */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <strings.h>
#include <string.h>

/* Linux headers */
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

/* Network headers */
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* External headers */
#include "cJSON.h"

/* Internal headers */
#include "Weather.h"

/* Macros */
#define MAXLINE 4096
#define WEATHERMSG 50

/* Prototypes */
float get_celsius(char* JSONinput);

float get_celsius(char* JSONinput) {
   cJSON *json;
   cJSON *main_Json;
   cJSON *item;
   char* aux;
   float temperatura;

   json = cJSON_Parse(JSONinput);
   if(!json) {
      printf("Error before: [%s]\n", cJSON_GetErrorPtr());
   }
   else {
      main_Json = cJSON_GetObjectItem(json, "main");
      item = cJSON_GetArrayItem(main_Json, 0);
      aux = cJSON_Print(item);
      temperatura = atof(aux);
      free(aux);
      return temperatura - 273;
   }
   return 0;
}

int get_weather (int connfd) {
   int sockfd, n;
   char recvline[MAXLINE + 1];
   char sndline[MAXLINE + 1];
   char weather_msg[WEATHERMSG + 1];
   struct sockaddr_in servaddr;
   char* desired_ip = "144.76.83.20";
   /* 144.76.83.20 endereço relativo ao openweathermap.com */
   
   if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
      fprintf(stderr,"socket error :( \n");
   
   bzero(&servaddr, sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_port = htons(80);  /* HTTP padrao roda na 80 */

   if (inet_pton(AF_INET, desired_ip, &servaddr.sin_addr) <= 0)  
      fprintf(stderr,"inet_pton error for %s :(\n", desired_ip);
   
   if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
      fprintf(stderr,"connect error :(\n");

   strcpy(sndline,"GET /data/2.5/weather?q=São+Paulo,BR\n\n");
   if (write(sockfd,sndline,strlen(sndline)) < 0)
      fprintf(stderr,"write error :(\n"); 
   
   while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
      recvline[n] = 0;
      sprintf(weather_msg, "%.1fºC site:openweathermap.org/\n", get_celsius(recvline));

      if (write(connfd, weather_msg, strlen(weather_msg)) < 0)
      	fprintf(stderr, "write error :(\n");
   }
   if (n < 0)
      fprintf(stderr,"read error :(\n");
   
   return 0;
}
