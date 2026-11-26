#ifndef PERSISTENCIA_H
#define PERSISTENCIA_H

#include "eval.h"

#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <system_error>
#include <thread>

inline const std::string save_path = "functions.txt";

class FunctionSaver {
public:
        // Única instância (criada no primeiro uso, gravando em save_path)
        static FunctionSaver &instance()
        {
                static FunctionSaver saver(save_path);
                return saver;
        }

        // Singleton: proíbe cópia e movimentação
        FunctionSaver(const FunctionSaver &) = delete;
        FunctionSaver &operator=(const FunctionSaver &) = delete;
        FunctionSaver(FunctionSaver &&) = delete;
        FunctionSaver &operator=(FunctionSaver &&) = delete;

        void save(const std::string &line)
        {
                {
                        std::lock_guard<std::mutex> lock(mutex_);
                        queue_.push(line);
                }
                cv_.notify_one();
        }

        // Grava o que está pendente e apaga o arquivo.
        // Devolve true se o arquivo foi apagado.
        bool erase()
        {
                namespace fs = std::filesystem;

                std::unique_lock<std::mutex> lock(mutex_);

                idle_cv_.wait(lock, [this] { return queue_.empty() && !writing_; });

                if (file_.is_open())
                        file_.close();
                file_.clear();

                std::error_code ec;

                if (!fs::is_regular_file(path_, ec)) {
                        std::cerr << "Arquivo não encontrado: '" << path_ << "'\n";
                        return false;
                }

                if (!fs::remove(path_, ec)) {
                        std::cerr << "Erro ao apagar '" << path_ << "': "
                                  << ec.message() << '\n';
                        return false;
                }

                return true;
        }

private:
        explicit FunctionSaver(const std::string &path)
                : path_(path),
                  file_(path, std::ios::app),
                  worker_(&FunctionSaver::run, this) {}

        ~FunctionSaver()
        {
                {
                        std::lock_guard<std::mutex> lock(mutex_);
                        stop_ = true;
                }
                cv_.notify_one();
                worker_.join();
        }

        void run()
        {
                std::unique_lock<std::mutex> lock(mutex_);

                while (true) {
                        cv_.wait(lock, [this] { return stop_ || !queue_.empty(); });

                        while (!queue_.empty()) {
                                std::string line = std::move(queue_.front());
                                queue_.pop();

                                writing_ = true;
                                lock.unlock();

                                if (!file_.is_open())
                                        file_.open(path_, std::ios::app);

                                file_ << line << '\n';
                                file_.flush();

                                lock.lock();
                                writing_ = false;
                                idle_cv_.notify_all();
                        }

                        if (stop_)
                                break;
                }
        }

        std::string path_;
        std::ofstream file_;
        std::queue<std::string> queue_;
        std::mutex mutex_;
        std::condition_variable cv_;
        std::condition_variable idle_cv_;
        bool stop_ = false;
        bool writing_ = false;
        std::thread worker_;                    // Sempre o último membro
};

void load_functions(const std::string &path);

#endif