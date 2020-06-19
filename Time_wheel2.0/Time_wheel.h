#ifndef TIME_WHEEL_H
#define TIME_WHEEL_H

#include<time.h>
#include<netinet/in.h>
#include<stdio.h>
#include<list>
#include<vector>

using namespace std;

#define BUFFER_SIZE 64
#define N  60 //时间轮上的数目
#define SI  1 //每一秒转动一次，即槽的间隔为1s

class Timer;

struct Client_data{
	typedef typename list<Timer>::iterator iter;
	sockaddr_in address;
	int sockfd;
	char buf[BUFFER_SIZE];
	iter timerIter;
};


class Timer{
public:
	//构造函数
	Timer(const int minutes, const int seconds ): rotation(minutes), time_slot(seconds){ }

public:
	int rotation;	//记录定时器转多少圈后失效
	int time_slot;	//记录定时器属于时间轮上哪个槽，从而找出对应的链表。
	void (*cb_func)(Client_data*); //定时器回调函数
	Client_data* user_data;		//客户数据
};


/*
 *	时间轮：轮子外层是一个vector构成的循环对列，vector的每一个格子(slot)中存放一个指向一条链表list的指针，
 *	而list链表将所有定时器timer串联起来。
 *  一个时间轮上有60个slot，计数器cur_slot表示秒针，每隔1s经过一个slot。
 *	不同的槽对应不同的计时秒数，timer中的rotation代表计时分钟数，秒针每转一圈回到同一个个slot，挂在该slot上
 *  的链表上的计时器检查成员变量rotation，rotation等于0的计时器会被erase，否则rotation减一。
*/
class Time_wheel{
public:
    typedef typename list<Timer>::iterator iter;		//时间链表上的迭代器，用来指向定时器。decltype( (*slots[cur_slot]).begin() ) 结果就是list<Timer>::iterator

	//构造
	Time_wheel() : cur_slot(0){
		for(int i = 0; i < N; ++i){
			slots.push_back( list<Timer>() );
		}
	}


	//添加定时器
	iter add_timer(int timeout){
		if(timeout < 0){
			return iter();
		}
		int ticks = (timeout) < SI ? 1 : (timeout / SI);

		int rotation = ticks / N;						//rotation定时器定时的分钟数。
		int ts = (cur_slot + ticks) % N;
		
		slots[ts].push_back(Timer(rotation, ts) );
		auto end = slots[ts].end();
		return --end;

	}

	//删除定时器
	void del_timer(iter timer){
		int ts = timer->time_slot;
		slots[ts].erase(timer);
	}

	//秒针走一格
	void tick(){
        for(auto listPtr = slots[cur_slot].begin(); listPtr != slots[cur_slot].end(); ){
			if(listPtr->rotation > 0){
				listPtr->rotation--;
				listPtr++;
			}
			else{
				listPtr->cb_func(listPtr->user_data);
				slots[cur_slot].erase(listPtr++);		//erase的时候原来位置上的迭代器会失效，所以采用listPtr++
			}
		}
		cur_slot = ++cur_slot % N;
	}

private:
	int cur_slot;				   //当前时刻秒针指向的槽
	vector< list<Timer> > slots;  //时间轮,共有N个槽
};

#endif