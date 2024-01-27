/*!
 * @file GLog.cpp 类GLog的定义文件
 **/

#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include "GLog.h"

static const char *LOG_TYPE_STR[] = {
	"",
	"WARN: ",
	"ERROR: ",
};

GLog::GLog(FILE *out) {
	dayOld_ = 0;
	if ((fd_ = out) == NULL) fd_ = stderr;
}

GLog::GLog(const char* dirName, const char* filePrefix) {
	dayOld_  = 0;
	fd_      = NULL;
	if (dirName) dirName_ = dirName;
	if (filePrefix) prefix_  = filePrefix;

	if (dirName_.empty()) {
		char cwd[MAXPATHLEN];
		dirName_ = getcwd(cwd, MAXPATHLEN);
	}
	char& cLast = dirName_.back();
	if (cLast != '/' && cLast != '\\') dirName_ += '/';
	if (prefix_.empty()) prefix_ = "gLog";
}

GLog::~GLog() {
	if (fd_ && fd_ != stdout && fd_ != stderr)
		fclose(fd_);
}

void GLog::Write(const char *format, ...) {
	if (format) {
		mutex_lock lck(mtx_);
		std::tm loctm;

		if (valid_file(loctm)) {
			va_list vl;

			fprintf (fd_, "%02d:%02d:%02d >> ", loctm.tm_hour, loctm.tm_min, loctm.tm_sec);
			va_start(vl, format);
			vfprintf(fd_, format, vl);
			va_end(vl);
			fprintf(fd_, "\n");
			fflush(fd_);
		}
	}
}

void GLog::Write(LOG_TYPE type, const char *format, ...) {
	if (format) {
		mutex_lock lck(mtx_);
		std::tm loctm;

		if (valid_file(loctm)) {
			va_list vl;

			fprintf (fd_, "%02d:%02d:%02d >> ", loctm.tm_hour, loctm.tm_min, loctm.tm_sec);
			fprintf (fd_, "%s", LOG_TYPE_STR[type]);
			va_start(vl, format);
			vfprintf(fd_, format, vl);
			va_end(vl);
			fprintf(fd_, "\n");
			fflush(fd_);
		}
	}
}

void GLog::Write(const char *where, LOG_TYPE type, const char *format, ...) {
	if (format) {
		mutex_lock lck(mtx_);
		std::tm loctm;

		if (valid_file(loctm)) {
			va_list vl;

			fprintf (fd_, "%02d:%02d:%02d >> ", loctm.tm_hour, loctm.tm_min, loctm.tm_sec);
			fprintf (fd_, "%s", LOG_TYPE_STR[type]);
			if (where) fprintf (fd_, "%s, ", where);
			va_start(vl, format);
			vfprintf(fd_, format, vl);
			va_end(vl);
			fprintf(fd_, "\n");
			fflush(fd_);
		}
	}
}

bool GLog::valid_file(std::tm &loctm) {
	std::time_t now = std::time(nullptr);
	memcpy(&loctm, std::localtime(&now), sizeof(std::tm));	// 本地时

	if (fd_ == stdout || fd_ == stderr)
		return true;

	if (dayOld_ != loctm.tm_mday) {
		dayOld_ = loctm.tm_mday;
		if (fd_) {// 关闭已打开的日志文件
			fprintf(fd_, "%s continue\n", std::string(69, '>').c_str());
			fclose(fd_);
			fd_ = NULL;
		}
	}

	if (!fd_) {
		if (access(dirName_.c_str(), F_OK)) mkdir(dirName_.c_str(), 0755);	// 创建目录
		if (!access(dirName_.c_str(), W_OK | X_OK)) {
			char filepath[MAXPATHLEN];
			snprintf(filepath, MAXPATHLEN, "%s%s_%d%02d%02d.log",
					 dirName_.c_str(), prefix_.c_str(),
					 loctm.tm_year + 1900, loctm.tm_mon + 1, loctm.tm_mday);
			fd_ = fopen(filepath, "a+");
			fprintf(fd_, "%s\n", std::string(79, '-').c_str());
		}
	}
	return fd_ != NULL;
}
