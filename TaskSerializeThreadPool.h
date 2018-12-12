#ifndef TaskSerializeThreadPool_H
#define TaskSerializeThreadPool_H

#include <algorithm>
#include <condition_variable>
#include <future>
#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include <iostream>
#include <typeindex>
#include <typeinfo>

namespace JT {
  typedef std::multimap<std::type_index, std::function<void()>> taskmap;
  class SerializeThreadPool {
  public:
    SerializeThreadPool() : stop(false) { this->MakeThread(8); }
    SerializeThreadPool(size_t _s) :stop(false) { this->MakeThread(_s); };

    void enqueue(std::function<void()>, std::type_index);

    ~SerializeThreadPool();

    void AddThreadTaskMap(std::thread::id, std::shared_ptr<taskmap>);
    void join();

  private:
    void MakeThread(size_t);
    // 일을 할 쓰레드들
    std::vector<std::thread> workers_;
    // 타스크
    //std::queue<std::function<void()>> task_;

    // Thread들의 타스크
    std::map<std::thread::id, std::shared_ptr<taskmap>> tasks_;

    std::mutex task_mutex;
    std::condition_variable worker_condition;

    bool stop;
  };
  inline void SerializeThreadPool::MakeThread(size_t _s) {
    for (size_t i = 0; i < _s; i++) {
      workers_.emplace_back([this] {
        std::shared_ptr<taskmap> mytask = std::make_shared<taskmap>();
        this->AddThreadTaskMap(std::this_thread::get_id(), mytask);
        //bool threadwait;
        for (;;)
        {
          {
            /*
            std::unique_lock<std::mutex> lock(this->task_mutex);

            threadwait = this->stop || !mytask->empty();
            this->worker_condition.wait(lock,threadwait);
            */
            // 멈추지 않았는데 자신의 타스크가 비였다면 기다린다.
            if (!this->stop && mytask->empty()) continue;

            if (this->stop && mytask->empty()) {
              // 삭제될때
              mytask->clear();
              return;
            }

          }
          // 실행 후 삭제
          mytask->begin()->second();
          mytask->erase(mytask->begin());
        }
      });
    }
  }

  void SerializeThreadPool::enqueue(std::function<void()> _t, std::type_index _in) {
    std::thread::id minthread;
    size_t mincnt = -1;
    for (auto taskqueue : tasks_) {
      taskmap::iterator itr = taskqueue.second->find(_in);
      if (itr != taskqueue.second->end()) {
        minthread = taskqueue.first;
        break;
      }
      // 쓰레드가 가진 값이 없다면 가장 일어 적는쓰레드에게 일을 넣는다.
      if (mincnt > taskqueue.second->size() || mincnt == -1) {
        mincnt = taskqueue.second->size();
        minthread = taskqueue.first;
      }
    }
    {
      std::unique_lock<std::mutex> lock(task_mutex);
      tasks_.find(minthread)->second->insert(std::pair<std::type_index, std::function<void()>>(_in, _t));
    }
    worker_condition.notify_one();
  }

  inline void SerializeThreadPool::AddThreadTaskMap(std::thread::id _id, std::shared_ptr<taskmap> _map) {
    tasks_.insert(std::pair<std::thread::id, std::shared_ptr<taskmap>>(_id, _map));
  }
  inline SerializeThreadPool::~SerializeThreadPool() {

    {
      std::unique_lock<std::mutex> lock(task_mutex);
      stop = true;
    }
    worker_condition.notify_all();
    for (std::thread &worker : workers_)
      worker.join();
  }
  inline void SerializeThreadPool::join() {
    while (true) {
      bool notend = false;
      for (auto taskqueue : tasks_) {
        if (!taskqueue.second->empty()) {
          notend = true;
          break;
        }
      }
      if (!notend) {
        return;
      }
    }
  }
}

#endif // !TaskSerializeThreadPool_H