/*!
 * @file globaldef.h 声明全局唯一参数
 * @version 1.0
 */

#ifndef GLOBALDEF_H_
#define GLOBALDEF_H_

// 软件名称、版本与版权
#define DAEMON_NAME			"gtoaes"
#define DAEMON_AUTHORITY	"© ARTD Group, NAOC"
#define DAEMON_VERSION		"v2.0 @ Jan, 2024"

// 日志文件路径与文件名前缀
#define LOG_DIR			"/var/log/"  DAEMON_NAME
#define LOG_PREFIX		DAEMON_NAME

// 软件配置文件
#define CONFIG_NAME		DAEMON_NAME  ".xml"
#define CONFIG_PATH		"/usr/local/etc/"  CONFIG_NAME

// 文件锁位置
#define DAEMON_PID		"/var/run/"  DAEMON_NAME  ".pid"

#endif /* GLOBALDEF_H_ */
