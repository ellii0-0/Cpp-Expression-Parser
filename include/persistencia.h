#ifndef PERSISTENCIA_H
#define PERSISTENCIA_H

#include "eval.h"

#include <condition_variable>
#include <fstream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

// Writes lines to a file from a background thread.
class FunctionSaver {
public:
        explicit FunctionSaver(const std::string &path)
                : file_(path, std::ios::app),             // append: never wipes old data
                  worker_(&FunctionSaver::run, this) {}   // thread starts here

        ~FunctionSaver()
        {
                {
                        std::lock_guard<std::mutex> lock(mutex_);
                        stop_ = true;
                }
                cv_.notify_one();
                worker_.join();         // waits until the queue is fully written
        }

        FunctionSaver(const FunctionSaver &) = delete;
        FunctionSaver &operator=(const FunctionSaver &) = delete;

        // Called from the main thread.
        void save(const std::string &line)
        {
                {
                        std::lock_guard<std::mutex> lock(mutex_);
                        queue_.push(line);
                }
                cv_.notify_one();
        }

private:
        void run()
        {
                std::unique_lock<std::mutex> lock(mutex_);

                while (true) {
                        cv_.wait(lock, [this] { return stop_ || !queue_.empty(); });

                        while (!queue_.empty()) {
                                std::string line = std::move(queue_.front());
                                queue_.pop();

                                lock.unlock();          // no lock held during file I/O
                                file_ << line << '\n';
                                file_.flush();
                                lock.lock();
                        }

                        if (stop_)
                                break;
                }
        }

        std::ofstream file_;
        std::queue<std::string> queue_;
        std::mutex mutex_;
        std::condition_variable cv_;
        bool stop_ = false;
        std::thread worker_;    // must stay LAST: it starts using the members above
};

// Reads the file and registers every function in it.
void load_functions(const std::string &path, FunctionTable &functions,
                    Variables &variables);

#endif