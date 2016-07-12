/**
 * @brief RTPC stress test 2
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

typedef struct _cmd_t {
	int flag;                  //O command save it, D command use it.
	char cmd_type;             //command type, 'O' or 'D'
} cmd_t;

typedef struct __main_context {
	//////////////////////////////////////////////////////
	//After initialization, readonly. (Start of block)
	//////////////////////////////////////////////////////
	char ip[24];               //rtpc ip address
	int port;                  //rtpc port
	int interval_ms;           //interval time(ms)
	int times;                 //one second times
	char rtppl[24];            //rtpp left
	char rtppr[24];            //rtpp right
	int sock;                  //UDP socket
	int count;                 //Send command count
	int max_concurrence;       //Max concurrence
	//////////////////////////////////////////////////////
	//After initialization, readonly. (End of block)
	//////////////////////////////////////////////////////

	unsigned int send_cmd_counter; //For O and D command sn auto increase
	int concurrence_counter;

	int o_cmd_send_counter;
	int o_cmd_ok_counter;
	int o_cmd_failed_counter;

	int d_cmd_send_counter;
	int d_cmd_ok_counter;
	int d_cmd_failed_counter;

	cmd_t* ring_send_info;
	int ring_send_info_first;
	int ring_send_info_second;

	pthread_mutex_t lock;
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
 * @brief init ring send buffer;
 * 
 * @author huangzheng (2016/7/8)
 * 
 * @param pcontext 
 * 
 * @return int 0: successful, otherwise: failed.
 */
int init_ring_info(main_context *pcontext) 
{
	pcontext->ring_send_info_first = 0;
	pcontext->ring_send_info_second = 0;

	pcontext->ring_send_info = (cmd_t *)malloc((pcontext->max_concurrence*2) * sizeof(cmd_t)); //Must greater than max_concurrence;
	if(NULL == pcontext->ring_send_info) {
		ERROR_LOG("No enough memory to malloc\n");
		return 1;
	}

	return 0;
}


/**
 * @brief destroy ring send buffer.
 * 
 * @author huangzheng (2016/7/8)
 * 
 * @param pcontext 
 */
void destroy_ring_info(main_context *pcontext)
{
	pcontext->ring_send_info_first = 0;
	pcontext->ring_send_info_second = 0;

	if(pcontext->ring_send_info) {
		free(pcontext->ring_send_info);
	}
}


/** 
 * @brief Set data to ring buffer 
 *  
 * @author huangzheng (2016/7/8)
 * 
 * @param pcontext 
 * @param[in] flag save to ring buffer
 * @param[in] cmd_type save to ring buffer
 * 
 * @return int 
 */
int push_ring_send_info(main_context *pcontext, int flag, char cmd_type) 
{
	pcontext->ring_send_info[pcontext->ring_send_info_first].flag = flag;
	pcontext->ring_send_info[pcontext->ring_send_info_first].cmd_type = cmd_type;
	++pcontext->ring_send_info_first;
	pcontext->ring_send_info_first %= (pcontext->max_concurrence*2);

	return 0;
}


/**
 * @brief Get data from ring buffer
 * 
 * @author huangzheng (2016/7/11)
 * 
 * @param pcontext 
 * @param[out] flag get from ring buffer
 * @param[out] cmd_type get from ring buffer
 * 
 * @return int 0: successful -1: failed.
 */
int pop_ring_send_info(main_context *pcontext, int *flag, char *cmd_type)
{
	if(pcontext->ring_send_info_first != pcontext->ring_send_info_second) { //Has data.

		*flag = pcontext->ring_send_info[pcontext->ring_send_info_second].flag;
		*cmd_type = pcontext->ring_send_info[pcontext->ring_send_info_second].cmd_type;

		++pcontext->ring_send_info_second;
		pcontext->ring_send_info_second %= (pcontext->max_concurrence*2);
		return 0;
	}

	return -1;
}


/**
 * @brief Get flag and result from json string.
 * 
 * @author huangzheng (2016/7/6)
 * 
 * @param[in] recv_buffer: json buffer 
 * @param[out] flag: save to flag 
 * @param[out] result: save to result 
 * 
 * @return int 0: successful. -1: failed.
 */
int parse_flag_and_result(char *recv_buffer, int *flag, int *result)
{
	char *p;

	int _flag = 0;
	if( NULL == (p=strstr(recv_buffer, "cookie\":\"113.31.21.228@")) ) {
		ERROR_LOG("\nReceive string(%s) can't find flag\n", recv_buffer);
		return -1;
	}

	if(1 != sscanf(p, "cookie\":\"113.31.21.228@%d@", &_flag)) {
		ERROR_LOG("\nReceive string(%s) can't get flag\n", recv_buffer);
		return -1;
	}

	int _result = 0;
	if( NULL == (p=strstr(recv_buffer, "\"result\":")) ) {
		ERROR_LOG("\nReceive string(%s) can't find result\n", recv_buffer);
		return -1;
	}

	if(1 != sscanf(p, "\"result\":%d", &_result)) {
		ERROR_LOG("\nReceive string(%s) can't get result\n", recv_buffer);
		return -1;
	}

	*flag = _flag;
	*result = _result;
	return 0;
}

/**
 * @brief Reveive UDP pthread function.
 * 
 * @author huangzheng (2016/7/6)
 * 
 * @param param: main_context pointer,use it and change it.
 * 
 * @return void* 
 */
void* recv_func(void *param) 
{
	main_context *pcontext = (main_context *)param;

	if(NULL == pcontext) {
		ERROR_LOG("Call receive thread function error\n");
		return NULL;
	}

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
		tv.tv_sec = 2;    //Wait read io 2 seconds.
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
				int flag, result;
				if(0 == parse_flag_and_result(recv_buffer, &flag, &result)) {
					if(flag >= 0 && flag < pcontext->count) {
						pthread_mutex_lock(&pcontext->lock);
						if( 0 == result ) {
							++pcontext->o_cmd_ok_counter;
							++pcontext->concurrence_counter;
							push_ring_send_info(pcontext, flag, 'D');
						} else {
							++pcontext->o_cmd_failed_counter; 
						}
						pthread_mutex_unlock(&pcontext->lock);
					} else {
						ERROR_LOG("\nReceive string(%s) flag is not valid\n", recv_buffer);
					}
				}
			} else if(strstr(recv_buffer, "\"cmd\":\"D\"")) {
				int flag, result;
				if(0 == parse_flag_and_result(recv_buffer, &flag, &result)) {
					if(flag >=0 && flag < pcontext->count) {
						pthread_mutex_lock(&pcontext->lock);
						if( 0 == result ) {
							++pcontext->d_cmd_ok_counter;
							--pcontext->concurrence_counter;
						} else {
							++pcontext->d_cmd_failed_counter; 
						}
						pthread_mutex_unlock(&pcontext->lock);
					} else {
						ERROR_LOG("\nReceive string(%s) flag is not valid\n", recv_buffer);
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
	printf("\n\n");
	printf("*************************\n");
	printf("*        Result         *\n");
	printf("*************************\n");
	printf("Stress count:   %d\n", pcontext->count);
	printf("*************************\n");
	printf("\033[35mO cmd count:    %d\033[0m\n", pcontext->o_cmd_send_counter); 
	printf("\033[32mO cmd ok:       %d\033[0m\n", pcontext->o_cmd_ok_counter); 
	printf("\033[33mO cmd failed:   %d\033[0m\n", pcontext->o_cmd_failed_counter);
	printf("\033[31mO cmd noreply:  %d\033[0m\n", pcontext->o_cmd_send_counter - pcontext->o_cmd_ok_counter - pcontext->o_cmd_failed_counter);
	printf("success rate:   %.02f%%\n", (((double)pcontext->o_cmd_ok_counter) / ((double)pcontext->o_cmd_send_counter) * 100)); 
	printf("*************************\n");
	printf("\033[35mD cmd count:    %d\033[0m\n", pcontext->d_cmd_send_counter); 
	printf("\033[32mD cmd ok:       %d\033[0m\n", pcontext->d_cmd_ok_counter);
	printf("\033[33mD cmd failed:   %d\033[0m\n", pcontext->d_cmd_failed_counter);
	printf("\033[31mD cmd noreply:  %d\033[0m\n", pcontext->d_cmd_send_counter - pcontext->d_cmd_ok_counter - pcontext->d_cmd_failed_counter);
	printf("success rate:   %.02f%%\n", (((double)pcontext->d_cmd_ok_counter) / ((double)pcontext->d_cmd_send_counter) * 100));
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
int init_context(int argc, char* argv[], main_context *pcontext) 
{
	#define PUSAGE() do {                              \
		printf("Usage:%s <rtpc ip>[:rtpc port]\n"      \
			"<-l rtpp left>\n"                         \
			"[-r rtpp right]\n"                        \
			"[-c count]\n"                             \
			"[-i interval(millisecond)]\n"             \
			"[-t one second times]\n"                  \
			"[-m max concurrence]\n"                   \
			"\n",                                      \
			argv[0]);                                  \
	} while(0)

	int interval_ms = 1000 * 5;
	int times = 100;
	int port = 9966;
	char ip[24] = "";
	char rtppl[24] = "";
	char rtppr[24] = "";
	int count = 10;
	int max_concurrence = 4000;

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
	while ((opt = getopt(argc, argv, "c:i:l:r:t:m:")) != -1) {
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
		case 'm':
			max_concurrence = atoi(optarg);
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
	pcontext->max_concurrence = max_concurrence;

	return 0;
}


int main(int argc, char *argv[]) 
{
	main_context context;
	memset(&context, 0, sizeof(context));

	if(0 != init_context(argc, argv, &context)) {
		exit(1);
	}

	if(0 != init_ring_info(&context)) {
		exit(1);
	}

	printf("Start rtpc stress:\n");
	printf("rtpc ip:               %s\n", context.ip);
	printf("rtpc port:             %d\n", context.port);
	printf("rtpp left:             %s\n", context.rtppl);
	printf("rtpp right:            %s\n", context.rtppr);
	printf("count:                 %d\n", context.count);
	printf("interval(ms):          %d\n", context.interval_ms);
	printf("one second times:      %d\n", context.times);
	printf("max concurrence:       %d\n", context.max_concurrence); 

	int sock;
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock < 0) {
		ERROR_LOG("\nCreate socket failed.\n");
		exit(1);
	}
	fcntl(sock, F_SETFL, O_NONBLOCK);
	context.sock = sock;

	//Init lock
	pthread_mutex_init(&context.lock, NULL);

	struct sockaddr_in sock_addr;
	memset(&sock_addr, 0, sizeof(sock_addr));
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = inet_addr(context.ip);
	sock_addr.sin_port = htons(context.port);
	char send_buffer[1024];
	int send_buffer_len;

	pthread_t recv_thread;
	pthread_create(&recv_thread, NULL, (void *)recv_func, (void *)&context);

#define O_COMMAND "{\"cmd\":\"O\",\"sn\":%d,\"cookie\":\"113.31.21.228@%d@1467099885123\","    \
		"\"notify\":\"172.16.2.99:1998\",\"action\":1,\"callmode\":5,\"funcswitch\":%d,"      \
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

	int concurrence_counter = 0;
	int o_cmd_ok_counter = 0;
	int o_cmd_failed_counter = 0;
	int d_cmd_ok_counter = 0;
	int d_cmd_failed_counter = 0;

	int need_send = 0;
	int flag; 
	char cmd_type;

	printf("\n");
	for(;;) {

		pthread_mutex_lock(&context.lock);
		if(context.concurrence_counter < context.max_concurrence) {
			if(context.o_cmd_send_counter < context.count) {
				flag = context.o_cmd_send_counter;
				cmd_type = 'O';
				need_send = 1;
			} else {
				need_send = pop_ring_send_info(&context, &flag, &cmd_type) == 0 ? 1 : 0;
			}
		} else {
			need_send = pop_ring_send_info(&context, &flag, &cmd_type) == 0 ? 1 : 0;
		}
		pthread_mutex_unlock(&context.lock);

		if(need_send) {
			if(cmd_type == 'O') {
				send_buffer_len = snprintf(send_buffer, sizeof(send_buffer), O_COMMAND, 
					++context.send_cmd_counter, flag, funcswitch, flag, context.rtppl, flag, rtppr_list);
			} else if(cmd_type == 'D') {
				send_buffer_len = snprintf(send_buffer, sizeof(send_buffer), D_COMMAND, 
					++context.send_cmd_counter, flag, flag, flag);
			} else {
				ERROR_LOG("Run error.\n");
				exit(1);
				break;
			}

			if(sendto(sock, send_buffer, send_buffer_len, 0, (struct sockaddr *)&sock_addr, sizeof(struct sockaddr)) != send_buffer_len) {
				ERROR_LOG("\nSend to(%s:%d) failed\n", context.ip, context.port);
				break;
			}

			if(cmd_type == 'O') {
				++context.o_cmd_send_counter;
			} else if(cmd_type == 'D') {
				++context.d_cmd_send_counter;
			}
		} else {
			if(context.o_cmd_send_counter >= context.count) {   //Check O command send over
				if(context.d_cmd_send_counter < o_cmd_ok_counter) { //Check D command send over.
					usleep(500); //Wait Send D command(push D command in receive UDP thread).
					continue;
				}
				break; //All command Send over.
			}
		}

		pthread_mutex_lock(&context.lock);
		o_cmd_ok_counter = context.o_cmd_ok_counter;
		o_cmd_failed_counter = context.o_cmd_failed_counter;
		d_cmd_ok_counter = context.d_cmd_ok_counter;
		d_cmd_failed_counter = context.d_cmd_failed_counter;
		concurrence_counter = context.concurrence_counter;
		pthread_mutex_unlock(&context.lock);

		//Realtime show info.
		printf("Send command:"
		       " { \033[34mconcurrence:%04d\033[0m, "
		       " O:{\033[35mcount:%06d\033[0m, "
		       "\033[32mok:%06d\033[0m, "
		       "\033[33mfailed:%06d\033[0m}, "
		       " D:{\033[35mcount:%06d\033[0m, "
		       "\033[32mok:%06d\033[0m, "
		       "\033[33mfailed:%06d\033[0m}"
		       " }\r",
		       concurrence_counter,
		       context.o_cmd_send_counter,
		       o_cmd_ok_counter,
		       o_cmd_failed_counter,
		       context.d_cmd_send_counter,
		       d_cmd_ok_counter,
		       d_cmd_failed_counter
		      );

		fflush(stdout);

		usleep(1000 * 1000 / context.times);
	}
	printf("\n");

	//Wait sometimes.
	printf("Sleep %d millisecond.\n", context.interval_ms);
	usleep(context.interval_ms * 1000);


	//Stop reveive UDP pthread
	pthread_cancel(recv_thread);
	pthread_join(recv_thread, NULL);

	//Destroy lock
	pthread_mutex_destroy(&context.lock); 

	//Close UDP socket.
	close(context.sock);

	show_result(&context);

	destroy_ring_info(&context);

	return 0;
}
