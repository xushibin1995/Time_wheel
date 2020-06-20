#ifndef TIME_WHEEL_H
#define TIME_WHEEL_H
#include<time.h>
#include<stdio.h>
#include<list>
#include<vector>
#include<utility>
using namespace std;

#define BUFFER_SIZE 1024
#define N 60
#define SI 1

class Timer;

struct Client_data{
//	sockaddr_in address;
	int sockfd;
	char buf[BUFFER_SIZE];

};

class Timer{
public:
    Timer(){}
	Timer(int minutes, int seconds):rotation(minutes),slot_index(seconds){}

	void operator=(const Timer & timer){
		rotation = timer.rotation;
		cb_func = timer.cb_func;
		user_data = timer.user_data;
	}

public:
	int rotation; //记录定时器的分钟数，也就是秒针转多少圈后失效
	int slot_index;
	void (*cb_func)(Client_data *);
	Client_data * user_data;
	int next_index;
	int pre_index;

};


class Time_wheel{
public:
	Time_wheel() : slots(N), slot_cur(0) {
		for(int i = 0; i < N; i++){
			slots[i].emplace_back();
			slots[i][0].next_index = 0;
			slots[i][0].pre_index = 0;

			slots[i].emplace_back();
			slots[i][1].next_index = 1;
			slots[i][1].pre_index = 1;
		}
	}
	pair<int, int> add_timer(int timeout){
		if(timeout < 0){
			return {};
		}
		int r = timeout / N;
		int slot_index = (timeout + slot_cur) % N;
		vector<Timer>& vec = slots[slot_index];
		//vec[0]是空闲链表头
		//vec[1]是定时器链表头
		int vec_index;
		if(vec[0].next_index == 0){
			vec.emplace_back(r, slot_index);
			vec_index = vec.size() -1;
            int tmp_index = vec[1].next_index;

			vec[vec_index].next_index = tmp_index;
			vec[1].next_index = vec_index;

			vec[tmp_index].pre_index = vec_index;
			vec[vec_index].pre_index = 1;

		}
		else{
			vec_index = vec[0].next_index;
			int tmp = vec[vec_index].next_index;
			vec[0].next_index = tmp;
			vec[tmp].pre_index = 0;
            vec[vec_index] = Timer(r, slot_index);

            tmp = vec[1].next_index;
			vec[vec_index].next_index = tmp;
			vec[1].next_index = vec_index;
			vec[tmp].pre_index = vec_index;
			vec[vec_index].pre_index = 1;
		}

		return {slot_index, vec_index};
	}

	void delete_timer(const pair<int, int>& location){
		int slot_index = location.first;
		int vec_index = location.second;
		if(slot_index >= N){
			return;
		}
		vector<Timer> & vec = slots[slot_index];
		if(vec[1].next_index == 1){
			return;
		}
		int tmp_index = vec[vec_index].pre_index;
		vec[tmp_index].next_index = vec[vec_index].next_index;

		tmp_index = vec[vec_index].next_index;
		vec[tmp_index].pre_index = vec[vec_index].pre_index;

        tmp_index = vec[0].next_index;
        vec[0].next_index = vec_index;
        vec[vec_index].next_index = tmp_index;
        vec[tmp_index].pre_index = vec_index;
        vec[vec_index].pre_index = 0;

	}

	void tick(){
		 vector<Timer> & vec  = slots[slot_cur];
		 int tmp = vec[1].next_index;   
		 while(tmp != 1){           
		 	if(vec[tmp].rotation != 0){
		 		vec[tmp].rotation--;
		 	}
		 	else{
		 		tmp = vec[tmp].next_index;		//一定要先让tmp向后移动，然后再去运行delete_timer,不然索引会失效
		 		delete_timer({slot_cur, tmp});
		 	}
		 }
		 slot_cur = (slot_cur + 1)%N;
	}


public:
	int slot_cur;
	vector<vector<Timer>> slots;

};

#endif