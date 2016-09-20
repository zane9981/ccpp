/**
 * @code utf-8 signature 
 *  
 * @author huangzheng 
 *  
 * @date 2016-09-09 
 *  
 * @brief Fast and asynchronous to operator MySQL DB. 
 */

#ifndef __MYSQL_POOL_H__
#define __MYSQL_POOL_H__

#include <mysql/mysql.h>

/* 
    Use Example
*/

/* demo.c */
/* 
#include "mysql_pool.h"
#include <stdlib.h>
#include <stdio.h>


int read_func(int code, MYSQL *mysql) 
{
    if (0 != code || NULL == mysql) {
        printf("code=%d, mysql=%p\n", code, mysql);
        return -1;
    }

    MYSQL_RES *res_ptr;
    MYSQL_ROW sqlrow;  
    res_ptr = mysql_store_result(mysql);  
    if (res_ptr) {
        printf("Retrieved %lu rows\n", (unsigned long)mysql_num_rows(res_ptr));
        while ((sqlrow = mysql_fetch_row(res_ptr))) {
            printf("%s,%s\n",sqlrow[0],sqlrow[1]);
        }
        if (mysql_errno(mysql)) {
            fprintf(stderr, "Retrive error: %s\n", mysql_error(mysql));
        }
        mysql_free_result(res_ptr);
    }
    return 0;
}

int main(int argc, char *argv[]) 
{
	void *read_handle;
	void *write_handle;

	mp_config_t read_config = {
		"172.16.2.88",
		3306,
		"city",
		"root",
		"admin",
		3,
		10,
		2048
	};


    mp_config_t write_config = {
		"172.16.2.88",
		3306,
		"city",
		"root",
		"admin",
		3,
		30,
		2048
	};

	read_handle = mp_create_mysql_pool(&read_config);
	mp_exec_sql(read_handle, "SELECT * FROM zone", read_func);
	mp_destroy_mysql_pool(read_handle);

    write_handle = mp_create_mysql_pool(&write_config);
    mp_exec_sql(write_handle, "INSERT INTO t1 (l1) VALUES ('abc')", NULL);
    mp_destroy_mysql_pool(write_handle);

	return 0;
}
*/


/* Makefile */
/* 
demo:demo.c mysql_pool.c mysql_pool.h
	gcc -o demo demo.c mysql_pool.c -lpthread -lmysqlclient -L/usr/lib64/mysql -g

clean:
	-rm -rf demo 
*/


typedef struct _mp_config_t {
	//MySQL地址
	char mysql_addr[256];

	//MySQL端口
	unsigned int mysql_port;

	//MySQL数据库名
	char mysql_name[256];

	//MySQL用户
	char mysql_user[256];

	//MySQL密码
	char mysql_pwd[256];

	//MySQL 执行错误的重连次数
	unsigned int mysql_error_count;

	//线程池的大小
	unsigned int pool_size;

	//环形队列的大小
	unsigned int ring_buffer_size;
} mp_config_t;

//创建一个mysql_pool的实例,成功返回一个非0的句柄.
void* mp_create_mysql_pool(mp_config_t* config);

//销毁一个mysql_pool实例.
void mp_destroy_mysql_pool(void* handle);

//执行一个SQL语句.
int mp_exec_sql(void *handle, const char *sql, int (*callback)(int, MYSQL*));


#endif