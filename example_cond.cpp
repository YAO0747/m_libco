//自己照着写的代码
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <queue>
#include "co_routine.h"

using namespace std;

//资源(生产者生产，消费者消费)
struct sTask_t
{
	int id;
};

//共享临界区(条件变量cond + 资源队列)
//生产者将资源放入临界区，消费者则将资源取出
struct stEnv_t
{
	stCoCond_t* cond;
	//资源队列存储的是指针
	queue<sTask_t*> task_queue;
};

//生产者协程
void* Producer(void* args)
{
	co_enable_hook_sys();
	//获取临界区地址
	stEnv_t* env = (stEnv_t*) args;
	int id = 0;

	while(true)
	{
		//(在堆上)生产一个资源
		sTask_t* task = (sTask_t*)calloc(1,sizeof(sTask_t));
		task->id = id++;
		//资源放入临界区
		env->task_queue.push(task);
		printf("%s:%d produce task %d\n",__func__,__LINE__,task->id);
		//信号量
		co_cond_signal(env->cond);
		poll(NULL,0,1000);
	}
	return NULL;
}

void* Consumer(void* args)
{
	co_enable_hook_sys();
	//获取临界区地址
	stEnv_t* env = (stEnv_t*)args;

	while(true)
	{
		if(env->task_queue.empty())
		{
			co_cond_timedwait(env->cond,-1);
			continue;
		}
		//从临界区取出资源
		sTask_t* task = env->task_queue.front();
		env->task_queue.pop();
		printf("%s:%d consume task %d\n",__func__,__LINE__,task->id);
		free(task);
	}
	return NULL;
}

int main()
{
	//创建临界区
	stEnv_t* env = new stEnv_t;
	env->cond = co_cond_alloc();

	//消费者协程
	stCoRoutine_t* consumer_routine;
	//参数:(协程，协程属性，协程函数，协程函数参数)
	co_create(&consumer_routine, NULL, Consumer, env);
	co_resume(consumer_routine);

	//生产者协程
	stCoRoutine_t* producer_routine;
	co_create(&consumer_routine, NULL, Producer, env);
	co_resume(producer_routine);

	co_eventloop(co_get_epoll_ct(), NULL, NULL);
	return 0;
}
