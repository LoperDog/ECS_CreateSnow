#ifndef _ECS_H
#define _ECS_H

#include <iostream>

#include <chrono>
#include <unordered_map>
#include <functional>
#include <vector>
#include <map>
#include <algorithm>


#include <string>
#include <memory>

#include <typeindex>
#include <typeinfo>

//#include "ThreadPool.h"
#include "TaskSerializeThreadPool.h"

#define SYSTEM_TYPE_SYNCWORLD 0
#define SYSTEM_TYPE_SYNC 1
#define SYSTEM_TYPE_ASYNC 2

namespace JT {

  class World;
  class EntitySystem;
  class Entity;
  class Component;
  class ThreadPool;

  typedef std::type_index TypeIndex;

  template<typename T>
  TypeIndex getTypeIndex()
  {
    return TypeIndex(typeid(T));
  }

  typedef std::unordered_map<std::type_index, std::shared_ptr<Component>> Components;
  //typedef std::vector<std::shared_ptr<Component>> Components;
  typedef std::vector<std::shared_ptr<Entity>> Entites;
  typedef std::vector<std::shared_ptr<EntitySystem>> Systems;

  // ������Ʈ ����
  class Component : public std::enable_shared_from_this<Component> {
  public:
    virtual ~Component() {}
  };

  // �ý��� ����
  class EntitySystem : public std::enable_shared_from_this<EntitySystem> {
  public:
    EntitySystem() {}
    EntitySystem(std::shared_ptr<World> _world) : world_(_world) {}

    virtual void tick() {}

    void SetType(size_t _type) { system_type_ = _type; }
    size_t GetType() { return system_type_; }

    void SetTime() { systemtime_ = std::chrono::system_clock::now(); }
    void SetDeltaTime() {
      deltatime_ = std::chrono::system_clock::now() - systemtime_;
      SetTime();
    }
    float GetDeltaTime() { return deltatime_.count(); }

    virtual std::type_index GetTypeIndex() { return std::type_index(typeid(this)); }

  protected:
    size_t system_type_ = SYSTEM_TYPE_SYNCWORLD;

    std::shared_ptr<World> world_;
    std::chrono::system_clock::time_point systemtime_;
    std::chrono::duration<double> deltatime_;
  };

  class Entity : public std::enable_shared_from_this<Entity> {
  public:
    //��ƼƼ�� �ƹ��͵� ����. ������Ʈ�� ������ ������.
    Entity(std::shared_ptr<World> _world) : world(_world) {}
    Entity(std::shared_ptr<World> _world, std::string _name) : world(_world), name(_name) {}

    void SetName(std::string n) { name = n; }
    std::string GetName() { return name; }

    // ��ƼƼ�� ������Ʈ�� ������ �ִ��� Ȯ���Ѵ�.
    template<class T>
    bool searchComponent() {
      std::unique_lock<std::mutex> lock(component_mutex);
      Components::const_iterator got = components.find(std::type_index(typeid(T)));
      if (got == components.end()) {
        return false;
      }
      else {
        return true;
      }

      return false;
    }

    // ������ ������Ʈ ��ȯ �Լ�
    template <class T>
    std::shared_ptr<T> GetComponent() {

      std::shared_ptr<T> reComponent = std::make_shared<T>();
      {
        std::unique_lock<std::mutex> lock(component_mutex);
        auto com = components.begin();
        for (com = components.begin(); com != components.end(); ++com) {
          if (std::type_index(typeid(T)) == (*com).first) {
            return std::static_pointer_cast<T>((*com).second);
          }
        }
      }
      return std::shared_ptr<T>(nullptr);

    }

    // ������Ʈ�� �߰� �Ǹ� ���忡 �ִ� �ý��ۿ� �߰��Ѵ�.
    template<typename T>
    int AddComponent(T _c) {

      //std::cout << GetName() << " add Component : " << typeid(_c.get()).name() << std::endl;
      {
        std::unique_lock<std::mutex> lock(component_mutex);
        
        components.insert(std::make_pair(std::type_index(typeid(*(_c.get()))), _c));
      }
      return 0;
    }

    template <typename T>
    bool DeleteComponent() {
      components.erase(std::type_index(typeid(T)));
    }
  private:
    std::string name;
    std::shared_ptr<World> world;
    Components components;
    std::mutex component_mutex;

  };

  class ThreadPool {
  public:
    ThreadPool() : stop(false) { this->MakeThread(4); }
    ThreadPool(size_t _s) :stop(false) { this->MakeThread(_s); }

    void enqueue(std::function<void()>);

    template<typename T>
    void enqueuetick(T _sys);

    ~ThreadPool();

    void join();

  private:
    void MakeThread(size_t);
    // ���� �� �������
    std::vector<std::thread> workers_;
    // Ÿ��ũ
    std::queue<std::function<void()>> task_;
    // �ý��۵�
    std::vector<std::shared_ptr<EntitySystem>> sys_;

    std::condition_variable worker_condition;
    std::mutex task_mutex;

    bool stop;
  };

  inline void ThreadPool::MakeThread(size_t _s) {
    for (size_t i = 0; i < _s; i++) {
      workers_.emplace_back([this] {
        for (;;)
        {
          std::shared_ptr<EntitySystem> sys;
          {
            std::unique_lock<std::mutex> lock(this->task_mutex);

            this->worker_condition.wait(lock,
              [this] { return this->stop || !this->sys_.empty(); });

            if (this->stop && this->sys_.empty()) {
              return;
            }
            sys = std::move(this->sys_.front());
            this->sys_.erase(this->sys_.begin());
          }
          sys->tick();
        }
      });
    }
  }
  void ThreadPool::enqueue(std::function<void()> _func) {
    {
      std::unique_lock<std::mutex> lock(task_mutex);
      task_.emplace(_func);
    }
    worker_condition.notify_one();
  }

  template<typename T>
  void ThreadPool::enqueuetick(T _sys) {
    {
      std::unique_lock<std::mutex> lock(task_mutex);
      sys_.push_back(_sys);
    }
    worker_condition.notify_one();
  }


  inline ThreadPool::~ThreadPool() {
    {
      std::unique_lock<std::mutex> lock(task_mutex);
      stop = true;
    }
    worker_condition.notify_all();
    for (std::thread &worker : workers_)
      worker.join();
  }
  inline void ThreadPool::join() {
    while (!task_.empty()) {}
    return;
  }
  // ����
  class World : public std::enable_shared_from_this<World> {
  public:
    //World() {
      //c_time = std::chrono::system_clock::now();
    //}
      // ��ƼƼ�� ����� �Ѱ� �ش�.
    std::shared_ptr<Entity> create() {
      std::shared_ptr<Entity> ent = std::make_shared<Entity>(shared_from_this());
      //std::cout << "World Create Entity" << std::endl;

      std::unique_lock<std::mutex> lock(entitymutex);
      entites.push_back(ent);

      return ent;
    }
    Entites GetEntity() { return entites; }

    // ����� ���� ���ͷ� ���ϴ� Ÿ���� ������Ʈ�� ������� �����Ѵ�.
    template<typename T>
    void each(std::function<void(std::shared_ptr<Entity>)> _func) {

      std::unique_lock<std::mutex> lock(entitymutex);
      auto ent = entites.begin();
      for (ent = entites.begin(); ent != entites.end(); ++ent) {
        if (ent->get()->searchComponent<T>()) {
          _func((*ent));
        }
      }
    }

    template<typename T>
    void AddSystem(T _sys) {
      //std::cout << "Add System : " << typeid(T).name() << std::endl;
      systems.push_back(_sys);
      //systems.emplace_back(_sys);
      _sys->SetTime();
    }
    // ���尡 �����δ�.
    void tick() {
      // tick�ѹ��� ���� �ý����� �ӽ÷� �����Ѵ�.
      // tick�� �����Ҷ� �ý����� �߰��� ���� �ֱ� �����̴�.
      Systems tickSystems = systems;

      // threadǮ���� system�� �Ѱ��ش�.
      for (auto sys : tickSystems) {
        sys->SetDeltaTime();
        if (sys->GetType() == SYSTEM_TYPE_ASYNC) {
          sys->tick();
          bs_threadpool_.enqueuetick(sys);
        }
      }

      for (auto sys : tickSystems)
      {
        //sys->SetDeltaTime();

        if (sys->GetType() == SYSTEM_TYPE_SYNCWORLD) {
          sys->tick();
        }
        /*
        switch (sys->GetType())
        {
        case SYSTEM_TYPE_SYNCWORLD:
          sys->tick();
          break;
        case SYSTEM_TYPE_SYNC:
          //ts_threadpool_.enqueue([&]() {
            //std::cout << "Running Task Sync" << std::endl;
          //  sys->tick();
          //}, sys->GetTypeIndex());
          //system("pause");
          break;
        case SYSTEM_TYPE_ASYNC:
          //bs_threadpool_.enqueue([&]() {
          //  sys->tick();
          //});
          break;
        default:
          break;
        }*/
      }
      bs_threadpool_.join();
      //ts_threadpool_.join();
    }
    std::shared_ptr<Entity> SearchEntity(std::string _enname) {
      for (auto &en : entites) {
        if (en.get()->GetName() == _enname)
          return en;
      }
      return std::shared_ptr<Entity>(nullptr);
    }
    // ���� ��ɾ�
#pragma region DeleteAll

    bool DeleteEntitys(std::string _enname) {
      if (entites.empty()) return false;

      bool isdelete = false;
      for (auto &en : entites) {
        if (en.get()->GetName() == _enname) {
          auto findentity = std::find(entites.begin(), entites.end(), en);
          entites.erase(findentity);
          isdelete = true;
        }
      }

      return isdelete;
    }
    bool DeleteEntity(std::string _enname) {
      if (entites.empty()) return false;

      for (auto &en : entites) {
        if (en.get()->GetName() == _enname) {
          auto findentity = std::find(entites.begin(), entites.end(), en);
          entites.erase(findentity);
          return true;
        }
      }
      return false;
    }
    bool DeleteEntity(std::string _enname, bool _all) {
      if (_all) {
        return DeleteEntitys(_enname);
      }
      else {
        return DeleteEntity(_enname);
      }
    }
    bool DeleteEntity(std::shared_ptr<Entity> _en) {
      // ��ƼƼ�� �񿴴ٸ� �����Ұ�.
      if (entites.empty()) return false;

      auto findentity = std::find(entites.begin(), entites.end(), _en);
      if (findentity != entites.end()) {
        entites.erase(findentity);
        return true;
      }
      else {
        return false;
      }
    }

#pragma endregion

  private:
    Systems systems;
    Entites entites;

    ThreadPool bs_threadpool_;
    //SerializeThreadPool ts_threadpool_;

    std::mutex systemmutex;
    std::mutex entitymutex;
    //std::chrono::system_clock::time_point c_time;
  };

}

#endif // _ECS_H

