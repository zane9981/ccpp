/**
 * @code utf-8 signature 
 *  
 * @author huangzheng 
 *  
 * @date 2016-09-09 
 *  
 * @brief Fast and asynchronous to operator MySQL DB. 
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "mysql_pool.h"

#include <stdio.h>
#include <stdlib.h>
#include <mysql/mysql.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

//设置SQL语句最长可以多少个字符
#define MP_MAX_SQL_SIZE (1024*3)

/**
 * @brief 线程相关的结构体
 */
typedef struct _mp_thread_t {
	//条件变量
	pthread_cond_t cond;

	//互斥锁
	pthread_mutex_t mutex;

	//线程ID
	pthread_t *id;
} mp_thread_t;


/**
 * @brief SQL语句相关的结构体
 */
typedef struct _mp_sql_t {
	//要执行的SQL语句
	char sql[MP_MAX_SQL_SIZE];

	//执行SQL以后调用的回调函数.
	//回调函数的参数说明:
	//int: 返回执行SQL成功的标志,0表示成功,其它表示失败.
	//MYSQL*: MySQL的连接句柄,可以用于SELECT之后,通过调用mysql_store_result获取数据.
	int (*callback)(int, MYSQL*);
} mp_sql_t;


/**
 * @brief 核心的结构体，关联上下文。
 */
typedef struct _mp_context_t {
	//配置信息,由create_mysql_pool指定
	mp_config_t config;

	//动态申请的内存，用来存放SQL语句相关的数据,相当于一个队列。
	mp_sql_t *ring_buffer;

	//Ring Buffer的索引, a: 前面, b: 后面.
	//每次判断a和b是否相等,如果a大于b,表示有(a-b)个缓存数据
	unsigned int index_a;
	unsigned int index_b;

	//线程池相关的数据。
	mp_thread_t thread;
} mp_context_t;


/**
 * 
 * @author huangzheng (2016/9/9)
 *  
 * @brief 关闭MySQl数据库连接
 *  
 * @param mysql[in] mysql连接句柄 
 *  
 * @return void
 *  
 */
static void db_close_conn(MYSQL *mysql)
{
	if(mysql) {
		mysql_close(mysql);
	}
}


/**
 * 
 * @author huangzheng (2016/9/18)
 *  
 * @brief 建立一个MySQl数据库连接 
 *  
 * @param addr[in] 数据库地址 
 * @param port[in] 数据库端口 
 * @param name[in] 数据库名称 
 * @param user[in] 数据库用户名 
 * @param pwd[in] 数据库密码
 * 
 * @return MYSQL* NULL:建立失败. 
 *                其它:建立成功,并返回数据库连接句柄.
 */
static MYSQL *db_create_conn(const char* addr, unsigned int port, const char* name, const char* user, const char* pwd)
{
	MYSQL *mysql;
	const char *unix_sock = NULL;
	int timeout = 1;

	mysql = mysql_init(NULL);
	if(mysql == NULL) {
		return NULL;
	}

	if (memcmp(addr, "localhost", sizeof("localhost")) == 0) {
		unix_sock = "/tmp/mysql.sock";
	}

	//设置查询数据库(select)超时时间
	mysql_options(mysql,MYSQL_OPT_READ_TIMEOUT,(const char *)&timeout);

	//设置写数据库(update,delect,insert,replace等)的超时时间。
	mysql_options(mysql,MYSQL_OPT_WRITE_TIMEOUT,(const char *)&timeout);

	if (!mysql_real_connect(mysql, addr, user, pwd, name, port, unix_sock, 0)) {
		db_close_conn(mysql);
		return NULL;
	}

	return mysql;
}


/**
 * 
 * 
 * @author huangzheng (2016/9/9) 
 *  
 * @brief 线程池函数,用来真正操作数据库.
 * 
 * @param param[in] 用来指向mp_context_t,上下文相关. 
 * 
 * @return void* 线程结束返回NULL.
 */
static void* thread_pool_func(void *param) 
{
	//上下文相关关键句柄m_pcontext_t*,由param传入.
	mp_context_t *mp;

	//MySQL的连接句柄
	MYSQL *mysql;

	//MySQL执行完毕的回调函数指针.
	int (*callback)(int, MYSQL*);

	//SQL语句.
	char sql[MP_MAX_SQL_SIZE];

	unsigned int i;

	//SQL语句执行知否成功,告知回调函数. 0:成功, 其它:失败.
	int code;

	if(NULL == param) {
		//assert
		return NULL;
	}

	mp = (mp_context_t *)param;

	mysql = db_create_conn(mp->config.mysql_addr, 
						   mp->config.mysql_port, 
						   mp->config.mysql_name, 
						   mp->config.mysql_user, 
						   mp->config.mysql_pwd);

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL); 

	for(;;) {

loop_start:
		//设置退出点,此处允许调用pthread_cancel()退出线程.
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		pthread_testcancel();
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL); 

		pthread_mutex_lock(&mp->thread.mutex);
		while(mp->index_a == mp->index_b) {
			int ret;
			struct timespec tv;
			clock_gettime(CLOCK_MONOTONIC, &tv);
			tv.tv_sec += 1;
			ret = pthread_cond_timedwait(&mp->thread.cond, &mp->thread.mutex, &tv);

			//有数据进来,pthread_cond_broadcast触发.
			if(ret == 0) {
				continue;

			//等待超时了,还没有数据进来.
			} else {
				pthread_mutex_unlock(&mp->thread.mutex);
				goto loop_start;
			}
		}

		strncpy(sql, mp->ring_buffer[mp->index_b].sql, sizeof(sql)); 
		callback = mp->ring_buffer[mp->index_b].callback;
		++mp->index_b;
		mp->index_b %= mp->config.ring_buffer_size;
		pthread_mutex_unlock(&mp->thread.mutex);

		code = 1;
		for(i = 0; i < mp->config.mysql_error_count; i++) {
			if(NULL != mysql && 0 == mysql_real_query(mysql, sql, (unsigned int)strlen(sql))) {
				code = 0;
				break;
			} else {
				db_close_conn(mysql);

				//第一次执行失败,先直接断开数据库,再快速的连接试试看.
				if(i != 0) {
					usleep(500 * 1000);
				}
				mysql = db_create_conn(mp->config.mysql_addr,
									   mp->config.mysql_port,
									   mp->config.mysql_name,
									   mp->config.mysql_user,
									   mp->config.mysql_pwd);
			}
		}

		if(callback) {
			callback(code, mysql);
		}
	}

	return NULL;
}



/**
 * 
 * 
 * @author huangzheng (2016/9/9) 
 *  
 * @brief 创建一个mysql_pool的实例
 * 
 * @param config[in] 配置信息
 * 
 * @return void* 非空:
 *                  创建成功，返回mp_context_t上下文的句柄，为了封装性，所以用void指针。
 *               其它:
 *                  创建失败。
 *  
 */
void* mp_create_mysql_pool(mp_config_t *config) 
{
	if(NULL == config) {
		return NULL;
	}

	mp_context_t* mp;

	//创建整个上下文相关的结构体,在destroy_mysql_pool里面释放.
	mp = (mp_context_t*)malloc(sizeof(mp_context_t));

	memcpy(&mp->config, config, sizeof(mp->config));

	///////////////////////////////////////////////////////
	// 初始化环形队列,在destroy_mysql_pool里面释放.
	///////////////////////////////////////////////////////
	mp->index_a = 0;
	mp->index_b = 0;

	mp->ring_buffer = (mp_sql_t*)malloc(sizeof(mp_sql_t) * mp->config.ring_buffer_size);
	///////////////////////////////////////////////////////


	///////////////////////////////////////////////////////
	// 初始化数据库操作的线程池,在destroy_mysql_pool里面释放.
	///////////////////////////////////////////////////////
	pthread_condattr_t condattr;
	pthread_condattr_init(&condattr);
	//设置相对时间,更靠谱一点.
	//例如如果中途修改系统时间,可能受会影响.
	pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
	pthread_cond_init(&mp->thread.cond, &condattr);
	pthread_mutex_init(&mp->thread.mutex, NULL);

	mp->thread.id = (pthread_t*)malloc(sizeof(pthread_t) * mp->config.pool_size);

	unsigned int i;
	for(i = 0; i < mp->config.pool_size; i++) {
		pthread_create(&mp->thread.id[i], NULL, thread_pool_func, (void*)mp);
	}
	///////////////////////////////////////////////////////


	return (void*)mp;
}


/**
 * 
 * 
 * @author huangzheng (2016/9/9) 
 *  
 * @brief 销毁一个mysql_pool实例
 * 
 * @param handle[in] mp_context_t上下文相关的句柄
 * 
 * @return void
 *  
 */
void mp_destroy_mysql_pool(void *handle) 
{
	unsigned int i;

	mp_context_t *mp;

	if(NULL == handle) {
		return;
	}

	mp = (mp_context_t *)handle;

	//如果有Ring Buffer里面还有数据,需要等数据在线程池里面处理完成才退出.
	////////////////////////////////////////////////////
	pthread_mutex_lock(&mp->thread.mutex);
	while(mp->index_a != mp->index_b) {
		pthread_mutex_unlock(&mp->thread.mutex);
		usleep(1000);
		pthread_mutex_lock(&mp->thread.mutex);
	}
	pthread_mutex_unlock(&mp->thread.mutex);
	////////////////////////////////////////////////////

	//退出线程池.
	////////////////////////////////////////////////////
	if(mp->thread.id) {
		for(i = 0; i < mp->config.pool_size; i++) {
			pthread_cancel(mp->thread.id[i]);
		}

		for(i = 0; i < mp->config.pool_size; i++) {
			pthread_join(mp->thread.id[i], NULL);
		}
		free(mp->thread.id);
		mp->thread.id = NULL;
	}
	////////////////////////////////////////////////////

	if(mp->ring_buffer) {
		free(mp->ring_buffer);
		mp->ring_buffer = NULL;
	}

	pthread_mutex_destroy(&mp->thread.mutex);
	pthread_cond_destroy(&mp->thread.cond);

	free(mp);
}


/**
 * 
 * 
 * @author huangzheng (2016/9/9)
 *  
 * @brief 发送一个待执行的SQL的命令 
 * @param handle[in] mp_context_t上下文相关的句柄
 * @param sql[in] 要执行的SQL语句
 * @param callback 执行完成要调用的回调函数
 * 
 * @return int   0: 成功 
 *             其它: 失败 
 */
int mp_exec_sql(void *handle, const char *sql, int(*callback)(int, MYSQL*))
{
	if(NULL == handle || sql == NULL) {
		return -1;
	}

	mp_context_t *mp;
	mp = (mp_context_t *)handle;

	pthread_mutex_lock(&mp->thread.mutex);
	strncpy(mp->ring_buffer[mp->index_a].sql, sql, sizeof(mp->ring_buffer[mp->index_a].sql)); 
	mp->ring_buffer[mp->index_a].callback = callback;
	++mp->index_a;
	mp->index_a %= mp->config.ring_buffer_size;
	pthread_cond_broadcast(&mp->thread.cond);
	pthread_mutex_unlock(&mp->thread.mutex);

	return 0;
}


#ifdef __cplusplus
}
#endif

