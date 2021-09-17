//
// Created by chen on 9/16/21.
//
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unordered_map>
#include <string>
#include <cstring>
#include <vector>

void handle_package(const char buf[], std::vector<std::string>& ans){
	// 0 类型
	// 1 pid
	// 2 发送端
	// 3 之后的数据 需要使用时再次处理
	std::string s = "";
	for(int i = 0; i < 128; i++){
		s += buf[i];
		if(i == 1 || i == 7 || i == 17 || i == 127){
			ans.push_back(s);
			s = "";
		}
	}
}

//TODO 客户端不是正常退出 会造成残留管道文件 服务端写管道一直被阻塞

int main(){
	std::unordered_map<std::string, std::string> client_table;
	int fd = open("main_fifo",O_RDONLY);
	if(fd == -1){
		perror("open main_fifo error:");
		exit(1);
	}
	std::cout << "server is running ..." << std::endl;
	while(1){
		char buf[128] = {'\0'};
		int n = read(fd, buf, sizeof(buf));
		if(n < 0){
			perror("read error:");
			exit(1);
		}
		if(n == 0) continue;
		std::vector<std::string> pack;
		handle_package(buf,pack);
		std::cout << "接收数据包：";
		for(int i = 0; i < pack.size(); i++) std::cout << pack[i] << "|";
		std::cout << std::endl;

		if(pack[0] == "00"){ // 0 登录
			int c_fd = open(pack[1].c_str(), O_WRONLY);
			if(c_fd < -1){
				perror("00 open c_fd error:");
				exit(1);
			}
			if(client_table.count(pack[2]) != 0 || client_table.size() > 8){ // 已有该用户名 或者 在线人数大于5
				// fail
				buf[18] = '0';
				write(c_fd,buf, 128);
				std::cout << pack[2] + " login fail" << std::endl;
			}else{
				//success
				buf[18] = '1';
				write(c_fd,buf, 128);
				std::cout << pack[2] + " login success" << std::endl;
				client_table[pack[2]] = pack[1];
			}
			close(c_fd);
		}else if(pack[0] == "01"){ // 1 当前在线人
			int c_fd = open(pack[1].c_str(), O_WRONLY);
			if(c_fd < -1){
				//打开失败错误退出
				perror("01 open c_fd error:");
				exit(1);
			}
			// 制作发送数据包
			char send[128] = {'\0'};
			// 填入数据包类型
			send[0] = '0';
			send[1] = '1';
			// 填入数据包pid部分
			for(int i = 2, j = 0; i < 8 && j < pack[1].length(); j++,i++) send[i] = pack[1][j];
			//填入数据包用户部分
			for(int i = 8, j = 0; i < 18 && j < pack[2].length(); j++,i++) send[i] = pack[2][j];
			//填入数据包返回数据
			send[18] = char(client_table.size() + '0');
			int i = 19;
			for(auto &c: client_table){
				for(int j = 0; j < c.first.length() && i < 128; j++, i++) send[i] = c.first[j];
			}
			write(c_fd, send, 128);
			close(c_fd);
		}else if(pack[0] == "10"){ // 2 发送消息
			// 获取接受端 进行判断
			std::string to;
			for(int i = 0; i < 10; i++) to += pack[3][i];
			if(client_table.count(to) == 0){ // 不再线
				// B  is not online 将接受端置空 返还给发送端 表示 不在线
				std::cout << to + " is not online" << std::endl;
				int c_fd = open(pack[1].c_str(), O_WRONLY);
				if(c_fd < -1){
					perror("10 open c_fd error:");
					exit(1);
				}
				for(int i = 18; i < 28; i++) buf[i] = '\0'; //将接受端位 置空 返还给发送端 表示 不在线
				write(c_fd, buf, 128);
				close(c_fd);
			}else{
				// B is online
				std::cout << to + " is online" << std::endl;
				int to_fd = open(client_table[to].c_str(), O_WRONLY);
				if(to_fd < -1){
					perror("10 open to_fd error:");
					exit(1);
				}
				write(to_fd, buf, 128); // 直接将数据报转发 由B端客户端 解包
				close(to_fd);
			}
		}else if(pack[0] == "11"){// 3 退出
			int c_fd = open(pack[1].c_str(), O_WRONLY);
			if(c_fd < -1){
				perror("11 open c_fd error:");
				exit(1);
			}
			write(c_fd, buf, 128); // 直接返还数据包 表示准许退出
			client_table.erase(pack[2]);
			close(c_fd);
		}
	}
	close(fd);
	return 0;
}

