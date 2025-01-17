
#ifndef LOG_MYLOG_H
#define LOG_MYLOG_H

#include <iostream>
#include <any>
#include <mutex>
#include <string>
#include <thread>
#include <condition_variable>
#include <queue>
#include <sstream>

namespace AsyncLog {

    enum LogLv {
        DEBUGS = 0,
        INFO = 1,
        WARN = 2,
        ERRORS = 3,
    };

    class LogTask {
    public:
        LogTask() {}

        LogTask(const LogTask &task) : _datas(task._datas), _level(task._level) {}

        LogTask(LogTask &&task) : _datas(std::move(task._datas)), _level(task._level) {}

        std::queue<std::any> _datas;
        LogLv _level;

    };

    class MyLog {
    public:
        static MyLog &Instance() {
            static MyLog mylog;
            return mylog;
        }

        MyLog(const MyLog &) = delete;
        MyLog &operator=(const MyLog &) = delete;

        template <typename... Args>
        void AsyncWrite(LogLv level,Args&&... args){

            auto task = std::make_shared<LogTask>();

            //c++17 折叠表达式依次将可变参数写入队列
            (task->_datas.push(args),...);

            task->_level=level;
            std::unique_lock<std::mutex> lock(_mutex);
            _queue.push(task);
            bool notify = (_queue.size() == 1)? true:false;
            lock.unlock();

            if(notify){
                _cond.notify_one();
            }

        }

        ~MyLog() {
            Stop();
            _work_thread.join();
            std::cout << "exit success" << std::endl;
        }

        void Stop(){
            _b_stop = true;
            _cond.notify_one();
        }



    private:
        MyLog() : _b_stop(false) {
            _work_thread = std::thread([this]() {
                std::unique_lock<std::mutex> lock(_mutex);
                _cond.wait(lock, [this] {
                    if (_b_stop) return true;

                    return !_queue.empty();
                });

                if (_b_stop) return;

                auto logTask = _queue.front();
                _queue.pop();
                lock.unlock();
                processTask(logTask);
            });

        }

        bool convertStr(const std::any &data, std::string &str) {
            std::ostringstream ss;
            if (data.type() == typeid(int)) {
                ss << std::any_cast<int>(data);
            } else if (data.type() == typeid(float)) {
                ss << std::any_cast<float>(data);

            } else if (data.type() == typeid(double)) {
                ss << std::any_cast<double>(data);
            } else if (data.type() == typeid(std::string)) {
                ss << std::any_cast<std::string>(data);
            } else if (data.type() == typeid(char *)) {
                ss << std::any_cast<char *>(data);
            } else if (data.type() == typeid(const char *)) {
                ss << std::any_cast<const char *>(data);
            } else {
                return false;
            }
            str = ss.str();
            return true;
        }

        template<typename ...Args>
        std::string formatString(const std::string &format, Args... args) {
            std::string result = format;
            size_t pos = 0;

            auto replacePlaceholder = [&](const std::string &placeholder, const std::any &replacement) {
                std::string str_replement = "";
                bool bSuccess = convertStr(replacement, str_replement);
                if (!bSuccess) {
                    return;
                }

                size_t placeholderPos = result.find(placeholder, pos);
                if (placeholderPos != std::string::npos) {
                    result.replace(placeholderPos, placeholder.length(), str_replement);
                    pos = placeholderPos + str_replement.length();
                } else {
                    result += result + " " + str_replement;
                }
            };

            (replacePlaceholder("{}", args), ...);
            return result;
        }


        void processTask(std::shared_ptr<LogTask> task) {
            std::cout << " log level is " <<task->_level<< std::endl;

            if (task->_datas.empty()) return;

            //队列首元素
            auto head = task->_datas.front();
            task->_datas.pop();

            std::string formatstr = "";
            bool bSuccess = convertStr(head, formatstr);

            if (!bSuccess) {
                return;
            }

            for (; !(task->_datas.empty());) {
                auto data = task->_datas.front();
                formatstr = formatString(formatstr, data);
                task->_datas.pop();
            }
            std::cout << "log string is " << formatstr << std::endl;
        }



        std::mutex _mutex;
        std::condition_variable _cond;
        std::queue<std::shared_ptr<LogTask>> _queue;
        std::thread _work_thread;
        std::atomic<bool> _b_stop;
    };


    template<typename ...Args>
    void ELog(Args &&... args) {
        MyLog::Instance().AsyncWrite(INFO,std::forward<Args>(args)...);
    }


    template<typename ...Args>
    void DLog(Args &&... args) {
        MyLog::Instance().AsyncWrite(INFO,std::forward<Args>(args)...);
    }

    template<typename ...Args>
    void ILog(Args &&... args) {
        MyLog::Instance().AsyncWrite(INFO,std::forward<Args>(args)...);
    }

    template<typename ...Args>
    void WLog(Args &&... args) {
        MyLog::Instance().AsyncWrite(INFO,std::forward<Args>(args)...);
    }

}


#endif
