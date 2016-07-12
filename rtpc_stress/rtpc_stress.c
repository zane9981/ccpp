/**
 * @brief RTPC stress test
 *  
 * @author huangzheng (2016/7/6) 
 *  
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

typedef struct __main_context {
	char ip[24];               //rtpc ip address
	int port;                  //rtpc port
	int interval_ms;           //interval time(ms)
	int times;                 //one second times
	char rtppl[24];            //rtpp left
	char rtppr[24];            //rtpp right
	int sock;
	int count;
	char *o_cmd_result;        //0: noreply. 1: ok. 2: failed.
	char *d_cmd_result;        //0: noreply. 1: ok. 2: failed.
} main_context;


/**
 * @brief print detail error message to screen.
 * 
 * @author huangzheng (2016/7/8)
 * 
 * @param out put strings.
 * 
 * @return int 0: successful, otherwise: failed.
 */
#define ERROR_LOG(args ...) print_error(__FILE__, __LINE__, args)
int print_error(const char *file, int line, const char *format, ...)
{
	const char *color_start = "\033[31m";
	const char *color_end = "\033[0m";

	va_list vaList;
	char buf[256];
	memset(buf, 0, sizeof(buf[0]) * sizeof(buf));
	va_start(vaList, format);
	vsnprintf(buf, sizeof(buf[0]) * sizeof(buf), format, vaList);
	va_end(vaList);

	time_t timep;
	struct tm *plocaltime;
	time(&timep);
	plocaltime = localtime(&timep);

	fprintf(stderr, "%s(%s:%d)%04d/%02d/%02d %02d:%02d:%02d %s%s",
			 color_start,
			 file,
			 line,
			 (1900 + plocaltime->tm_year),
			 (1 + plocaltime->tm_mon),
			 plocaltime->tm_mday,
			 plocaltime->tm_hour,
			 plocaltime->tm_min,
			 plocaltime->tm_sec, 
			 buf,
			 color_end
			 );

	return 0;
}


/**
 * @brief Get sn and result from json string.
 * 
 * @author huangzheng (2016/7/6)
 * 
 * @param[in] recv_buffer: json buffer 
 * @param[out] sn: save to sn 
 * @param[out] result: save to result 
 * 
 * @return int 0: successfult. -1: failed.
 */
int parse_sn_and_result(char *recv_buffer, int *sn, int *result)
{
	char *p;

	int _sn = 0;
	if( NULL == (p=strstr(recv_buffer, "\"sn\":")) ) {
		ERROR_LOG("\nReceive string(%s) can't get sn\n", recv_buffer);
		return -1;
	}

	if(1 != sscanf(p, "\"sn\":%d,", &_sn)) {
		ERROR_LOG("\nReceive string(%s) can't get sn\n", recv_buffer);
		return -1;
	}

	int _result = 0;
	if( NULL == (p=strstr(recv_buffer, "\"result\":")) ) {
		ERROR_LOG("\nReceive string(%s) can't get result\n", recv_buffer);
		return -1;
	}

	if(1 != sscanf(p, "\"result\":%d", &_result)) {
		ERROR_LOG("\nReceive string(%s) can't get result\n", recv_buffer);
		return -1;
	}

	*sn = _sn;
	*result = _result;
	return 0;
}

/**
 * @brief Reveive UDP pthread.
 * 
 * @author huangzheng (2016/7/6)
 * 
 * @param param: main_context
 * 
 * @return void* 
 */
void* recv_func(void *param) 
{
	main_context *pcontext = (main_context *)param;

	char recv_buffer[1024];
	int recv_buffer_len = 0;

	struct sockaddr_in addr;
	socklen_t sock_len;

	for(;;) {

		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		pthread_testcancel();
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

		int res;
		fd_set rdfds;
		struct timeval tv;
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		FD_ZERO(&rdfds);
		FD_SET(pcontext->sock, &rdfds); 
		res = select(pcontext->sock+1, &rdfds, NULL, NULL, &tv); 
		if(res == -1 || res == 0 || !FD_ISSET(pcontext->sock, &rdfds)) {
			continue;
		}

		if((recv_buffer_len = recvfrom(pcontext->sock, recv_buffer, sizeof(recv_buffer)-1, 0, (struct sockaddr *)&addr, &sock_len)) > 0) {
			recv_buffer[recv_buffer_len] = '\0';
			if(strstr(recv_buffer, "\"cmd\":\"O\"")) {
				int sn, result;
				if(0 == parse_sn_and_result(recv_buffer, &sn, &result)) {
					if(sn >=1 && sn <= pcontext->count) {
						if( 0 == result ) {
							pcontext->o_cmd_result[sn-1] = 1;
						} else {
							pcontext->o_cmd_result[sn-1] = 2;
						}
					} else {
						ERROR_LOG("\nReceive string(%s) sn is not valid\n", recv_buffer);
					}
				}
			} else if(strstr(recv_buffer, "\"cmd\":\"D\"")) {
				int sn, result;
				if(0 == parse_sn_and_result(recv_buffer, &sn, &result)) {
					sn -= pcontext->count;
					if(sn >=1 && sn <= pcontext->count) {
						if( 0 == result ) {
							pcontext->d_cmd_result[sn-1] = 1;
						} else {
							pcontext->d_cmd_result[sn-1] = 2;
						}
					} else {
						ERROR_LOG("\nReceive string(%s) sn is not valid\n", recv_buffer);
					}
				}
			} else {
				ERROR_LOG("\nReceive unknow string(%s)\n", recv_buffer);
			}
		}
	}

	return NULL;
}


/**
 * @brief Show rtpc stress result.
 * 
 * @author huangzheng (2016/7/6)
 * 
 * @param param: main_context
 */
void show_result(main_context* pcontext)
{
	int o_cmd_ok_count = 0;
	int o_cmd_failed_count = 0;
	int o_cmd_noreply_count = 0;
	int d_cmd_ok_count = 0;
	int d_cmd_failed_count = 0;
	int d_cmd_noreply_count = 0;

	int i;
	for(i = 0; i < pcontext->count; i++) {
		if(1 == pcontext->o_cmd_result[i]) {
			++o_cmd_ok_count;
		} else if(2 == pcontext->o_cmd_result[i]) {
			++o_cmd_failed_count;
		} else {
			++o_cmd_noreply_count;
		}

		if(1 == pcontext->d_cmd_result[i]) {
			++d_cmd_ok_count;
		} else if(2 == pcontext->d_cmd_result[i]) {
			++d_cmd_failed_count;
		} else {
			++d_cmd_noreply_count;
		}
	}

	printf("\n\n");
	printf("*************************\n");
	printf("*        Result         *\n");
	printf("*************************\n");
	printf("Stress count:   %d\n", pcontext->count);
	printf("*************************\n");
	printf("\033[32mO cmd ok:       %d\033[0m\n", o_cmd_ok_count);
	printf("\033[33mO cmd failed:   %d\033[0m\n", o_cmd_failed_count);
	printf("\033[31mO cmd noreply:  %d\033[0m\n", o_cmd_noreply_count);
	printf("success rate:   %.02f%%\n", (((double)o_cmd_ok_count) / ((double)pcontext->count) * 100));
	printf("*************************\n");
	printf("\033[32mD cmd ok:       %d\033[0m\n", d_cmd_ok_count);
	printf("\033[33mD cmd failed:   %d\033[0m\n", d_cmd_failed_count);
	printf("\033[31mD cmd noreply:  %d\033[0m\n", d_cmd_noreply_count);
	printf("success rate:   %.02f%%\n", (((double)d_cmd_ok_count) / ((double)pcontext->count) * 100));
	printf("*************************\n");
}


/**
 * @brief Init main_context from input args.
 * 
 * @author huangzheng (2016/7/6)
 * 
 * @param context 
 * @param argc 
 * @param argv 
 * 
 * @return int 0: successful. 1: failed.
 */
int init_contex(int argc, char* argv[], main_context *pcontext) 
{
	#define PUSAGE() do { \
		printf("Usage:%s <rtpc ip>[:rtpc port] <-l rtpp left> [-r rtpp right] [-c count] [-i interval] [-t one second times]\n", argv[0]); \
	} while(0)

	int interval_ms = 1000 * 5;
	int times = 100;
	int port = 9966;
	char ip[24] = "";
	char rtppl[24] = "";
	char rtppr[24] = "";
	int count = 10;

	if(argc < 2) {
		PUSAGE();
		return 1;
	}

	char *p;
	if( NULL != (p=strchr(argv[1],':')) ) {
		*p = '\0';
		strncpy(ip, argv[1], sizeof(ip));
		port = atoi(p + 1);
	} else {
		strncpy(ip, argv[1], sizeof(ip));
	}

	int opt;
	while ((opt = getopt(argc, argv, "c:i:l:r:t:")) != -1) {
		switch (opt) {
		case 'c':
			count = atoi(optarg);
			break;
		case 'i':
			interval_ms = atoi(optarg);
			break;
		case 'l':
			strncpy(rtppl, optarg, sizeof(rtppl));
			break;
		case 'r':
			strncpy(rtppr, optarg, sizeof(rtppr));
			break;
		case 't':
			times = atoi(optarg);
			break;
		default:
			PUSAGE();
			return 1;
		}
	}

	if(count <= 0) {
		ERROR_LOG("count must > 0\n");
		PUSAGE();
		return 1;
	}

	if(rtppl[0] == '\0') {
		ERROR_LOG("rtpp left must set\n");
		PUSAGE();
		return 1;
	}

	if(times < 0) {
		ERROR_LOG("times must > 0\n");
		PUSAGE();
		return 1;
	}

	pcontext->times = times;
	pcontext->interval_ms = interval_ms;
	pcontext->port = port;
	strncpy(pcontext->ip, ip, sizeof(pcontext->ip));
	strncpy(pcontext->rtppl, rtppl, sizeof(pcontext->rtppl));
	strncpy(pcontext->rtppr, rtppr, sizeof(pcontext->rtppr));
	pcontext->count = count;

	return 0;
}

int main(int argc, char *argv[]) 
{
	main_context context;
	memset(&context, 0, sizeof(context));

	if(0 != init_contex(argc, argv, &context)) {
		exit(1);
	}

	printf("Start rtpc stress:\n");
	printf("rtpc ip:          %s\n", context.ip);
	printf("rtpc port:        %d\n", context.port);
	printf("rtpp left:        %s\n", context.rtppl);
	printf("rtpp right:       %s\n", context.rtppr);
	printf("count:            %d\n", context.count);
	printf("interval(ms):     %d\n", context.interval_ms);
	printf("one second times: %d\n", context.times);

	int sock;
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock < 0) {
		ERROR_LOG("\nCreate socket failed.\n");
		exit(1);
	}
	fcntl(sock, F_SETFL, O_NONBLOCK);
	context.sock = sock;

	context.o_cmd_result = (char *)malloc(context.count * sizeof(char));
	if(NULL == context.o_cmd_result) {
		ERROR_LOG("\nmalloc memory failed\n");
		exit(1);
	}
	context.d_cmd_result = (char *)malloc(context.count * sizeof(char));
	if(NULL == context.d_cmd_result) {
		ERROR_LOG("\nmalloc memory failed\n");
		exit(1);
	}

	memset(context.o_cmd_result, 0, context.count);
	memset(context.d_cmd_result, 0, context.count);

	struct sockaddr_in sock_addr;
	memset(&sock_addr, 0, sizeof(sock_addr));
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = inet_addr(context.ip);
	sock_addr.sin_port = htons(context.port);
	char send_buffer[1024];
	int send_buffer_len;
	int i;

	pthread_t recv_thread;
	pthread_create(&recv_thread, NULL, (void *)recv_func, (void *)&context);

#define O_COMMAND "{\"cmd\":\"O\",\"sn\":%d,\"cookie\":\"113.31.21.228@%d@1467099885123\","    \
		"\"notify\":\"172.16.2.99:1998\",\"action\":1,\"callmode\":5,\"funcswitch\":%d,"       \
		"\"caller\":{\"uid\":\"8%010d\",\"ip\":\"172.17.1.182\",\"port\":60992,\"pt\":0,"      \
		"\"rtpplist\":[{\"ip\":\"%s\",\"delay\":0,\"lost\":0}]},"                              \
		"\"callee\":{\"uid\":\"9%010d\",\"ip\":\"127.0.0.1\",\"port\":0,\"pt\":0,"             \
		"\"rtpplist\":[%s]}}"

#define D_COMMAND "{\"cmd\":\"D\",\"sn\":%d,\"cookie\":\"113.31.21.228@%d@1467099885123\","    \
		"\"action\":1,\"callmode\":5,\"caller\":\"8%010d\",\"callee\":\"9%010d\"}"

	int funcswitch = 8;
	char rtppr_list[64] = "";
	if(context.rtppr[0] != '\0') {
		snprintf(rtppr_list, sizeof(rtppr_list), "{\"ip\":\"%s\",\"delay\":0,\"lost\":0}", context.rtppr);
		funcswitch = 24;
	}

	printf("\n");
	for(i = 1; i <= context.count; i++) {
		send_buffer_len = snprintf(send_buffer, sizeof(send_buffer), O_COMMAND, i, i, funcswitch, i, context.rtppl, i, rtppr_list);
		if(sendto(sock, send_buffer, send_buffer_len, 0, (struct sockaddr *)&sock_addr, sizeof(struct sockaddr)) != send_buffer_len) {
			ERROR_LOG("\nsendto(%s:%d) failed\n", context.ip, context.port);
			break;
		}
		//printf("%s\n", send_buffer);
		printf("Send O command: \033[45m\033[1m %d\r\033[0m", i);
		fflush(stdout);
		usleep(1000 * 1000 / context.times);
	}
	printf("\n");

	printf("Sleep %d millisecond.\n", context.interval_ms);
	usleep(context.interval_ms * 1000);

#if 1
	printf("\n");
	for(i = 1; i <= context.count; i++) {
		send_buffer_len = snprintf(send_buffer, sizeof(send_buffer), D_COMMAND, i + context.count, i, i, i);
		if(sendto(sock, send_buffer, send_buffer_len, 0, (struct sockaddr *)&sock_addr, sizeof(struct sockaddr)) != send_buffer_len) {
			ERROR_LOG("\nsendto(%s:%d) failed\n", context.ip, context.port);
			break;
		}
		//printf("%s\n", send_buffer);
		printf("Send D command: \033[45m\033[1m %d\r\033[0m", i);
		fflush(stdout);		
		usleep(1000 * 1000 / context.times);
	}
	printf("\n");

	printf("Sleep %d millisecond.\n", context.interval_ms);
	usleep(context.interval_ms * 1000);
#endif

	pthread_cancel(recv_thread);
	pthread_join(recv_thread, NULL);
	close(context.sock);

	show_result(&context);

	free(context.o_cmd_result);
	free(context.d_cmd_result);

	return 0;
}
