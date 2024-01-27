/*!
 Name        : gtoaes.cpp
 Author      : Xiaomeng Lu
 Copyright   : ARTD, NAOC
 Description : 观测控制服务器/GWAC
 Version     : 2.0
 Date        : Jan 2, 2024
 */

#include <boost/asio/signal_set.hpp>
#include <boost/bind/bind.hpp>
#include "globaldef.h"
#include "daemon.h"
#include "GLog.h"
#include "Parameter.h"
#include "GeneralControl.h"

#ifdef NDEBUG
GLog _gLog(stdout);
#else
GLog _gLog(LOG_DIR, LOG_PREFIX);		/// 工作日志
#endif

int main(int argc, char **argv) {
	if (argc >= 2) {// 处理命令行参数
		if (strcmp(argv[1], "-d") == 0) {
			Parameter param;
			param.Init(CONFIG_NAME);
		}
		else printf("Usage: gtoaes <-d>\n");
	}
	else {// 常规工作模式
		Parameter param;
#ifdef NDEBUG
		if (!param.Load(CONFIG_NAME)) return 1;
#else
		if (!param.Load(CONFIG_PATH)) return 1;
#endif
		boost::asio::io_service ios;
		boost::asio::signal_set signals(ios, SIGINT, SIGTERM);  // interrupt signal
		signals.async_wait(boost::bind(&boost::asio::io_service::stop, &ios));
		if (!MakeItDaemon(ios)) return 1;
		if (!isProcSingleton(DAEMON_PID)) {
			_gLog.Write("%s is already running or failed to access PID file", DAEMON_NAME);
			return 2;
		}
		_gLog.Write("Try to launch %s %s %s as daemon", DAEMON_NAME, DAEMON_VERSION, DAEMON_AUTHORITY);
		// 主程序入口
		GeneralControl gc(&param);
		if (gc.Start()) {
			_gLog.Write("Daemon goes running");
			ios.run();
			gc.Stop();
			_gLog.Write("Daemon stopped");
		}
		else {
			_gLog.Write(LOG_FAULT, NULL, "Fail to launch %s", DAEMON_NAME);
		}
	}
	return 0;
}
