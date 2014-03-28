//============================================================================
// Name        : EmulabStarter.cpp
// Author      : Jie Li
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../CommUtil/StatusProxy.h"
#include "../MVCTP/ConfigInfo.h"
#include "../MVCTP/Tester.h"

using namespace std;

ConfigInfo* ptr_config_info;
StatusProxy* ptr_monitor;

void StartStatusMonitor();
void Clean();


int main(int argc, char** argv) {
	if (argc != 2) {
		cout << "usage: a.out config_file_name" << endl;
		return -1;
	}

	// Parse configuration file
	ptr_config_info = ConfigInfo::GetInstance();
	ptr_config_info->Parse(argv[1]);


	Tester tester;
	tester.StartTest();

	Clean();
	return 0;
}


// Start status monitor and connect to remote server (if configured)
void StartStatusMonitor() {
	pid_t pid;
	if ((pid = fork()) < 0)
		return;
	else if (pid == 0) {
		// run status monitor as a daemon in the child process
		setsid();
		chdir("/");
		umask(0);

		string serv_addr = ptr_config_info->GetValue("Monitor Server");
		string port = ptr_config_info->GetValue("Monitor Server Port");
		if (serv_addr.length() > 0) {
			ptr_monitor = new StatusProxy(serv_addr, atoi(port.c_str()));
			ptr_monitor->ConnectServer();
			ptr_monitor->StartService();
		}
	}
}


void Clean() {
	delete ptr_config_info;
	delete ptr_monitor;
}
