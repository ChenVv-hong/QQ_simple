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

//TODO 出于练习管道通信 暂时没有对用户输入进行严格判断
//TODO 客户端不是正常退出 会造成残留管道文件 服务端写管道一直被阻塞 客户端再次发送数据 服务端无法接受 因为被阻塞
//TODO 客户端运行在终端时 输入中文消息 无效    |  在IDE中运行客户端程序 输入中文消息 能够成功发送

int main(){
	std::string fifo_name = std::to_string(getpid());
	std::cout << "pid:" << fifo_name << std::endl;
	int ret = mkfifo(fifo_name.c_str(), 0664);
	if(ret == -1){
		perror("mkfifo error:");
		exit(1);
	}

	int fd_w = open("main_fifo",O_WRONLY);
	if(fd_w == -1){
		perror("open fd_w error:");
		exit(1);
	}
	int fd_r = open(std::to_string(getpid()).c_str(), O_RDONLY | O_NONBLOCK);
	if(fd_r == -1){
		perror("open fd_w error:");
		exit(1);
	}
	std::string name; // 我的用户名
	while(1){
		std::cout << "input your name to login" << std::endl;
		std::cin >> name;
		char send[128] = {'\0'};
		char receive[128] = {'\0'};
		send[0] = '0';
		send[1] = '0';
		for(int i = 2, j = 0; i < 8 && j < fifo_name.length(); j++,i++){
			send[i] = fifo_name[j];
		}
		for(int i = 8, j = 0; i < 18 && j < name.length(); j++,i++){
			send[i] = name[j];
		}
		write(fd_w, send, 128);
		int flag = 0; // 是否退出登录循环
		while(1){
			int n = read(fd_r, receive, 128);
			if(n <= 0) continue;
//			for(int i = 0; i < 128; i++) std::cout << receive[i];
//			std::cout << std::endl;
			if(receive[0] == '0' && receive[1] == '0' && receive[18] == '1'){
				flag = 1;
				std::cout << "login success" << std::endl;
			}
			else{
				std::cout << "login fail" << std::endl;
			}
			break;
		}
		if(flag == 1) break;

	}

	int flag = fcntl(STDIN_FILENO,F_GETFL);
	flag |= O_NONBLOCK;
	fcntl(STDIN_FILENO,F_SETFL, flag);
	std::cout << "1 查看在线人数" <<std::endl;
	std::cout << "2+用户名+消息 向用户发送消息 空格间隔" <<std::endl;
	std::cout << "3 退出程序" <<std::endl;
	while(1){
		char send[128] = {'\0'};
		char receive[128] = {'\0'};
		int n = read(fd_r, receive, 128);
		if(n > 0){
			//解包
			if(receive[0] == '0' && receive[1] == '1'){ // 显示人数包
				std::cout << "当前在线人数：" << receive[18] <<std::endl;
				for(int i = 0; i < receive[18] - '0'; i++){
					for(int j = 0; j < 10; j++){
						if(receive[19 + i * 10 + j] == '\0') break;
						else std::cout << receive[19 + i * 10 + j];
					}
					std::cout << std::endl;
				}
			}
			else if(receive[0] == '1' && receive[1] == '0'){ // 消息数据包
				//判断在不在线
				if(receive[18] == '\0'){
					std::cout << "用户不在线 或 用户名输入错误 请输入 1 查看在线用户" << std::endl;
					continue;
				}
				for(int i = 8; i < 18; i++){ // 发送消息者
					if(receive[i] == '\0') break;
					std::cout << receive[i];
				}
				std::cout << " : ";
				for(int i = 28; i < 128; i++){ // 消息内容
					if(receive[i] == '\0') break;
					std::cout << receive[i];
				}
				std::cout << std::endl;

			}
			else if(receive[0] == '1' && receive[1] == '1'){
				std::cout << "logout !" << std::endl;
				break;
			}
		}
		int m = read(STDIN_FILENO, send, 128);
		if(m > 0){
			char ws[128] = {'\0'};
			// 处理客户端用户输入
			std::vector<std::string> v;
			std::string s;
			for(int i = 0; i < strlen(send) - 1; i++){
				if(send[i] == ' ' && v.size() < 2){ // 分割
					v.push_back(s);
					s = "";
					continue;
				}
				s += send[i];
			}
			v.push_back(s);
			if(v.size() <= 0){
				std::cout << "空 重新输入" << std::endl;
				continue;
			}
			if(v[0] == "1"){
				if(v.size() != 1){
					std::cout << "只能1个字段 重新输入" << std::endl;
					continue;
				}
				ws[0] = '0';
				ws[1] = '1';
				for(int i = 2, j = 0; i < 8 && j < fifo_name.length(); j++,i++){
					ws[i] = fifo_name[j];
				}
				for(int i = 8, j = 0; i < 18 && j < name.length(); j++,i++){
					ws[i] = name[j];
				}
				write(fd_w, ws, 128);
			}
			else if(v[0] == "2"){
				if(v.size() != 3){
					std::cout << "只能3个字段 重新输入" << std::endl;
					continue;
				}
				//判断是否想自己发送消息
				if(name == v[1]){
					std::cout << "请不要想自己发送消息！" << std::endl;
					continue;
				}
				ws[0] = '1';
				ws[1] = '0';
				for(int i = 2, j = 0; i < 8 && j < fifo_name.length(); j++,i++){
					ws[i] = fifo_name[j];
				}
				for(int i = 8, j = 0; i < 18 && j < name.length(); j++,i++){
					ws[i] = name[j];
				}
				for(int i = 18, j = 0; i < 28 && j < v[1].length(); i++, j++){
					ws[i] = v[1][j];
				}
				for(int i = 28, j = 0; i < 128 && j < v[2].length(); i++, j++){
					ws[i] = v[2][j];
				}
				write(fd_w, ws, 128);
			}
			else if(v[0] == "3"){
				if(v.size() != 1){
					std::cout << "只能1个字段 重新输入" << std::endl;
					continue;
				}
				ws[0] = '1';
				ws[1] = '1';
				for(int i = 2, j = 0; i < 8 && j < fifo_name.length(); j++,i++){
					ws[i] = fifo_name[j];
				}
				for(int i = 8, j = 0; i < 18 && j < name.length(); j++,i++){
					ws[i] = name[j];
				}
				write(fd_w, ws, 128);
			}

		}

	}
	close(fd_r);
	close(fd_w);
	ret = unlink(fifo_name.c_str());
	if(ret == -1){
		perror("unlink error:");
		exit(1);
	}
	return 0;
}

