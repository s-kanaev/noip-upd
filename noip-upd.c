#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/poll.h>
#include <strings.h>

//*************************** errors definitions
// no config file specified as command line argument
#define NO_CFG_OPT 8
// problems with config file
#define NO_CFG_FILE 9
// problems while connecting
#define CON_PROBL 10
// something wrong while sending request
#define WRITE_FAIL 11
// can't open LOG file
#define NO_LOG_FILE 12
// error while creating socket
#define NO_SOCKET 13
// cant resolve hostname
#define NO_HOST_BY_NAME 14
// results with no runtime errors but of update status
#define GOT_GOOD 0
#define GOT_NOCHG 0
#define GOT_NOHOST 1
#define GOT_BADAUTH 2
#define GOT_BADAGENT 3
#define GOT_DONATOR 4
#define GOT_ABUSE 5
#define GOT_FATALITY 6
// if neither of update responses got
#define GOT_NONE 7

// no-ip update results
#define good "good"
#define nochg "nochg"
#define nohost "nohost"
#define badauth "badauth"
#define badagent "badagent"
#define donator "!donator"
#define abuse "abuse"
#define fatality "911"

// newline
#define NW_LINE 0x0a

// server portnumber
#define PORT 80

// retrieve ip url
#define get_ip_url "http://ip1.dynupdate.no-ip.com"
#define get_ip_host "ip1.dynupdate.no-ip.com"
// URL template
#define upd_url_tpl "/nic/update?hostname=%s&myip=%s"
// Query template
#define query_tpl "GET %s HTTP/1.0\r\nHost: %s\r\nAuthorization: Basic %s\r\nUser-Agent: %s/%s %s\n\n\n"


// client-wide definitions
// client name
#define CLIENT_NAME "noip-upd"
// client version
#define CLIENT_VER "0.1"
// author e-mail address
#define CLIENT_EMAIL "ksergy.91@gmail.com"

// query size
#define QR_SZ 4096
// result size
#define RES_SZ 4096

// log message len
#define LG_MSG_LEN 5000

// log filename
#define LOG_NAME "/var/log/noip-upd.log"


// ***************************** Static data
// log file descriptor
FILE* log_fl;
char lg_msg[LG_MSG_LEN];

//*************************** functions
// Exit function
void Exit(int code)
{
 fclose(log_fl);
 exit(code);
}

// LOG function
/*
 @param msg - what to log
*/
void log_this (char *msg)
{
 char message[LG_MSG_LEN+256];
 time_t cur_tm;
// bzero(message, strlen(msg)+256);
 cur_tm = time (NULL);
 sprintf(message, "%s - %s\n", ctime(&cur_tm), msg);
// printf("%s\n", message);
 fwrite(message, strlen(message), 1, log_fl);
}

// print out usage of client
void usage (char* me)
{
 printf("\n USAGE - %s <configuration file name>\n\n", me);
}

// assign a socket
/*
 @return socket number
*/
int give_me_a_socket()
{
 int sckt=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
 if (sckt<0) {
	perror("socket");
	Exit(NO_SOCKET);
 };
 sprintf(lg_msg, "socket created - 0x%x", sckt);
 log_this(lg_msg);
 return sckt;
}

// fill the server (destination) structure
/*
 @param struct sockaddr_in* - destination structure to be filled
 @param char* - update host name
*/
struct hostent* fill_server_in (struct sockaddr_in **dest, char *upd_host)
{
 struct hostent *hp = gethostbyname(upd_host);
 sprintf(lg_msg, "got hostent struct pointer - 0x%x", hp);
 log_this(lg_msg);
 if (!hp) {
	perror("gethost by name");
	Exit(NO_HOST_BY_NAME);
 };
 // fill destination specifier structure
 dest[0] = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in *));
 dest[0]->sin_family = AF_INET;
 memcpy((char*)(&dest[0]->sin_addr), hp->h_addr, hp->h_length);
 dest[0]->sin_port=htons(PORT);
 return hp;
}

// connect to server
/*
 @param int - socket descriptor
 @param struct sockaddr_in* - destination descriptor
*/
void Connect (int sckt, struct sockaddr_in* dest)
{
 int res;
 res=connect(sckt, (const struct sockaddr*)dest, sizeof(*dest));
 if (res<0) {
    perror("connect");
    Exit(CON_PROBL);
 };
}

// send smth to server
/*
 @param skt - sckt through which to send smth
 @param smth - what to send
 @param len - how much bytes to send
*/
void Send (int skt, char* smth, int len)
{
 int sent=0;		// summary sent bytes (<=len)
 int c_sent=0;		// curently sent bytes (<=len), SUMM_ALL(c_sent) = len

 while (sent < len) {
	c_sent = write(skt, smth, strlen(smth));
	if (c_sent < 0) {
		perror("write");
		Exit(WRITE_FAIL);
	};
	smth += c_sent;
	sent += c_sent;
 };
}
// read smth from server
/*
 @param sckt - sckt to read from
 @param buf - where to write data read
 @param max_len - maximum length of read message
 @return - number of bytes read
*/
int Recv(int sckt, char* buf, int max_len)
{
 struct pollfd to_poll;
 int res;
 int bytes_read = 0;

 to_poll.fd=sckt;
 to_poll.events=POLLIN;

 res = poll(&to_poll, 1, 2000);
 if (res) {
    bytes_read=recv(sckt, buf, max_len-1, 0);
 };

 return bytes_read;
}

// get last line of a response
char* get_last_line (char* str)
{
 char *l1=NULL;
 char *l2=NULL;
 int len = strlen(str);
 int i=0;
 for (i=0; i<len; i++) {
    if (str[i]==NW_LINE) {
	l2 = l1;
	l1=str+i;
//	printf("l1 = %x\n", l1);
    };
 };
 return l1+1;
}

//*************************** MAIN function
int main (int argc, char** argv)
{
 FILE* conf;
 int res;					// error number from "current" operation
 struct hostent *hp;		
 struct sockaddr_in *dest;	// destination specifier structure
 int sckt;				// sckt to write to and to read from
 char* iface;				// network interface - maybe
// char login[51];				// noip login
// char password[256];			// noip password
 char hostname[64];			// noip hostname to update
 char upd_host[64];			// noip update server ("dynupdate.no-ip.com")
 char* my_ip;				// my ip address
 char ln_pass_enc[1024];			// Base64 "login:password" encoded string
 char *upd_res;				// result of update
 char query[QR_SZ];			// query string
 char query_part[QR_SZ>>2];		// url part of query
 char result[RES_SZ];			// resulting (recieved) string

 // read configuration file									+ LOG
 if (argc < 2) {	// no config file specified
	usage(argv[0]);
	Exit(NO_CFG_OPT);
 }
 // open LOG file
 log_fl = fopen(LOG_NAME, "a");
 if (!log_fl) {
	perror("fopen @ log file");
	Exit(NO_LOG_FILE);
 };
 log_this("\n----------------------\nHey. I've been started!");
 	// data to read in config file
		// network interface - maybe
		// noip login
		// noip password
		// noip hostname to update
		// noip update server
 conf=fopen(argv[1], "r");
 if (!conf) {
    perror("fopen");
    Exit(NO_CFG_FILE);
 };
 log_this("configuration file opened. Hurrah!");
		// parse config file
 	// parameters should be listed in a strict order
	// char* ln_pass_enc			// Base64 encoded "login:password" string
	// char* hostname;			// noip hostname to update
	// char* upd_host;			// noip update server ("dynupdate.no-ip.com")
 	fscanf(conf, "%s", ln_pass_enc);
// 	fscanf(conf, "%s", password);
 	fscanf(conf, "%s", hostname);
 	fscanf(conf, "%s", upd_host);
 fclose(conf);
 sprintf(lg_msg, "conf file read\n\tLogin:Password - %s\n\thostname - %s\n\tupdhost - %s", ln_pass_enc, hostname, upd_host);
 log_this(lg_msg);
 // create sckt											+ LOG
 sckt = give_me_a_socket();
 // retrieve update host by hostname
 hp=fill_server_in (&dest, upd_host);
 Connect (sckt, dest);

 // retrieve my ip											+ LOG
 sprintf(query, query_tpl, get_ip_url, get_ip_host, ln_pass_enc, CLIENT_NAME, CLIENT_VER, CLIENT_EMAIL);
 Send(sckt, query, strlen(query));	// send retrieve-ip request
 sprintf(lg_msg, "QUERY - \n%s\n", query);
 log_this(lg_msg);
 res=Recv(sckt, result, RES_SZ);	// TODO retrieve ip
// sprintf(lg_msg, "RESULT -\n%s\n", result);
// log_this(lg_msg);
// printf("res=%x, RES_SZ=%x\n-\n%s\n-", res, RES_SZ, result);
 my_ip=get_last_line(result);
// printf("my-ip - %x\n",my_ip);
 sprintf(lg_msg, "my-ip -\n#%s#\n", my_ip);
 log_this(lg_msg);
 // request for ip update									+ LOG
	// build a query
	sprintf(query_part, upd_url_tpl, hostname, my_ip);
	sprintf(query, query_tpl, query_part, upd_host, ln_pass_enc, CLIENT_NAME, CLIENT_VER, CLIENT_EMAIL);
 // send the query
 // thus connection is already closed we're going to connect to server again
 // connect to server
 shutdown(sckt, SHUT_RDWR);
 close(sckt);
 // create sckt											+ LOG
 sckt = give_me_a_socket();
 // retrieve update host by hostname
 hp=fill_server_in (&dest, upd_host);
 // connect to server
 Connect (sckt, dest);

 Send(sckt, query, strlen(query));
 sprintf(lg_msg, "QUERY2 - \n%s\n", query);
 log_this(lg_msg);
 // read response											+ LOG
 bzero(result, RES_SZ);		// flush result string
 res=Recv(sckt, result, RES_SZ);	// TODO retrieve ip
 // make conclusion of response								+ LOG
// sprintf(lg_msg, "RESULT -\n%s\n", result);
// log_this(lg_msg);
// printf("res=%x, RES_SZ=%x\n-\n%s\n-", res, RES_SZ, result);
 upd_res=get_last_line(result);
// printf("my-ip - %x\n",my_ip);
 sprintf(lg_msg, "result_of_update -\n#%s#\n", upd_res);
 log_this(lg_msg);
 shutdown(sckt, SHUT_RDWR);
 close(sckt);
 fclose(log_fl);
 free(dest);
 if ((strstr(upd_res, good) || strstr(upd_res, nochg))) return GOT_GOOD;
 if (strstr(upd_res, nohost)) return GOT_NOHOST;
 if (strstr(upd_res, badauth)) return GOT_BADAUTH;
 if (strstr(upd_res, badagent)) return GOT_BADAGENT;
 if (strstr(upd_res, donator)) return GOT_DONATOR;
 if (strstr(upd_res, abuse)) return GOT_ABUSE;
 if (strstr(upd_res, fatality)) return GOT_FATALITY;
 return GOT_NONE;
}

/*
 while ((res>=0) && (bytes_read < max_len)) {
	if (to_poll.revents & POLLIN) {	// can read
		res = recv(sckt, buf, 1, 0);
//		printf("\n res=%d\n", res);
		if (res<0) {	// error occured
			perror("something on recv()");
			log_this("something on recv()");
			return bytes_read;
		};
		bytes_read++;
//		printf("%c", buf[0]);
		buf++;
	};

	to_poll.fd=sckt;
	to_poll.events=POLLIN;
	res = poll(&to_poll, 1, 1);
//	printf("\n res=%d\n", res);
	if (res<0) {
		perror("something on poll()");
		log_this("something on poll()");
		return bytes_read;
	};
 };
 if (res<0) {
//    printf("\n- res=%d\n", res);
    perror("something on poll()");
 };
 to_poll.events=POLLIN; to_poll.fd=sckt;
// printf("\n-%d-\n", poll(&to_poll, 1, 1));
// printf("\nmax_len=%d, res=%d\n", max_len, res);
*/
