#ifndef SYSTEMS_H
#define SYSTEMS_H

#include "stdafx.h"

#include "ECS.h"
#include "Renderer.h"

static wchar_t jamak[512] = {0};

class Position : public JT::Component {
public:
  Position() {
    X = (rand() % 1024) - 512;
    Y = 256;
  }
  Position(float x_, float y_) : X(x_), Y(y_) {}

  Position(bool s) {
    X = (rand() % 768) - 256;
    Y = (rand() % 512) - 256;
  }
  void SetPosition(float x_, float y_) {
    X = x_; Y = y_;
  }
  void SetX(float x_) { X = x_; }
  void SetY(float y_) { Y = y_; }
  float GetX() { return X; }
  float GetY() { return Y; }

private:
  float X, Y;
};

class TransForm : public JT::Component {
public:
  TransForm() {
    scale = (rand() & 3) + 1;
  }
  int GetScale() { return scale; }
private:
  int scale;
};
class CreateSnow : public JT::EntitySystem {
public:
  CreateSnow(std::shared_ptr<JT::World> world_) : JT::EntitySystem(world_) {}
  void tick() override {
    if (currenttime >= 0.015f && world_->GetEntity().size() <= 1500 && alltime >= 4.0f)
    {
      currenttime = 0.0f;

      auto newsnow = world_->create();
      newsnow->SetName("SnowBall");
      newsnow->AddComponent(std::make_shared<Position>());
      newsnow->AddComponent(std::make_shared<TransForm>());
      g_snow++;
    }
    else if (currenttime >= 0.015f && alltime <= 4.0f) {
      currenttime = 0.0f;

      auto newsnow = world_->create();
      newsnow->SetName("SnowBall");
      newsnow->AddComponent(std::make_shared<Position>(true));
      newsnow->AddComponent(std::make_shared<TransForm>());

      g_snow++;
    }
    else {
      currenttime += GetDeltaTime();
    }
    alltime += GetDeltaTime();
  }
  std::type_index GetTypeIndex() override {
    return std::type_index(typeid(this));
  }
private:
  float currenttime = 0.0f;
  float alltime = 0.0f;
};

// 중력 엔티티시스템
class Gravity : public JT::EntitySystem {
public:
  Gravity(std::shared_ptr<JT::World> world_) : JT::EntitySystem(world_) {}
  void tick() override {
    //std::cout << "Start Gravity Tick " << std::endl;
    world_.get()->each<Position>([&](std::shared_ptr<JT::Entity> ent) {
      auto pos = ent->GetComponent<Position>().get();
      pos->SetY(pos->GetY() - (GetDeltaTime() * 45));
    });
  }
  std::type_index GetTypeIndex() override {
    return std::type_index(typeid(this));
  }
};

// 바람 엔티티시스템
class Wind : public JT::EntitySystem {
public:
  Wind(std::shared_ptr<JT::World> world_) : JT::EntitySystem(world_) {}
  void tick() override {
    if (currenttime >= (resettime)) {
      goal_ = (rand() % 200) - 50;
      currenttime = 0.0f;
      resettime = (rand() % 5)+3;
    }
    else {
      currenttime += GetDeltaTime();
    }
    //std::cout << "Start Gravity Tick " << std::endl;
    current_ = goal_ - current_;
    world_.get()->each<Position>([&](std::shared_ptr<JT::Entity> ent) {
      auto pos = ent->GetComponent<Position>().get();
      auto trs = ent->GetComponent<TransForm>().get();
      float tempmove = (GetDeltaTime() * current_);
      tempmove /= trs->GetScale();
      pos->SetX(pos->GetX() - tempmove);
    });
  }
  std::type_index GetTypeIndex() override {
    return std::type_index(typeid(this));
  }
private:
  float goal_ = 0.0f;
  float current_ = 0.0f;
  float adll_ = 0.0f;
  float currenttime = 0.0f;
  float resettime = 0.0f;
};

// 시간에 맞추어 시스템을 추가하는 시스템
class AddSystem : public JT::EntitySystem {
public:
  AddSystem(std::shared_ptr<JT::World> world_) : JT::EntitySystem(world_) {
    snow = std::make_shared<CreateSnow>(world_);
    grav = std::make_shared<Gravity>(world_);
    wind = std::make_shared<Wind>(world_);

    snow->SetType(SYSTEM_TYPE_ASYNC);
    grav->SetType(SYSTEM_TYPE_ASYNC);
    wind->SetType(SYSTEM_TYPE_ASYNC);
  }
  void tick() override {
    if(currenttime >= 4.0f && !snowbool) {
      currenttime = 0.0f;

      world_.get()->AddSystem(snow);
      snowbool = true;
    }
    else if (currenttime >= 4.0f && !gravbool) {
      currenttime = 0.0f;

      world_.get()->AddSystem(grav);
      gravbool = true;
    }
    else if (currenttime >= 4.0f && !windbool) {
      currenttime = 0.0f;

      world_.get()->AddSystem(wind);
      windbool = true;
    }
    currenttime += GetDeltaTime();
  }
  std::type_index GetTypeIndex() override {
    return std::type_index(typeid(this));
  }
private:
  std::shared_ptr<CreateSnow> snow;
  std::shared_ptr<Gravity> grav;
  std::shared_ptr<Wind> wind;
  float currenttime = 0.0f;
  bool snowbool = false;
  bool gravbool = false;
  bool windbool = false;

};
// 전체 렌더링 시스템
class RenderSystem : public JT::EntitySystem {
public:
  RenderSystem(std::shared_ptr<JT::World> world_) : JT::EntitySystem(world_) {}
  std::type_index GetTypeIndex() override {
    return std::type_index(typeid(this));
  }

  void tick() override {
    if (currenttime >= 0.008f) {
      currenttime = 0.0f;
      // Buffer Clear
      SetColor(15, 15, 15);
      Clear();
      DrawFrameText();
      DrawSnowText();
      // Draw
      SetColor(245, 240, 250);

      if (jamakTimming <= 2.0f) {
        Drawjamak(L"Start Entity");
      }
      if (jamakTimming >= 4.0f && jamakTimming <= 6.0f) {
        Drawjamak(L"Add Create System");
      }
      if (jamakTimming >= 8.0f && jamakTimming <= 10.0f) {
        Drawjamak(L"Add Gravity System");
      }
      if (jamakTimming >= 12.0f && jamakTimming <= 14.0f) {
        Drawjamak(L"Add Wind System");
      }

      world_.get()->each<Position>([&](std::shared_ptr<JT::Entity> ent) {
        auto pos = ent->GetComponent<Position>().get();
        auto trs = ent->GetComponent<TransForm>().get();
        pos->SetY(pos->GetY() - (GetDeltaTime() * 5));
        for (int x = 0; x < trs->GetScale(); x++) {
          for (int y = 0; y < trs->GetScale(); y++) {
            PutPixel(pos->GetX() + x, pos->GetY() + y);
          }
        }
      });

      // Buffer Swap 
      BufferSwap();
    }
    else {
      currenttime += GetDeltaTime();
    }
    jamakTimming += GetDeltaTime();
  }
private:
  float currenttime = 0.0f;
  float jamakTimming = 0.0f;
};
#endif // !SYSTEMS_H