#pragma once
#include <cstdint>
#include "constants.h"

constexpr uint16_t invalid_entity = -1;
struct Entity
{
  uint32_t tick = 0;
  uint32_t color = 0xff00ffff;
  float x = 0.f;
  float y = 0.f;
  float speed = 0.f;
  float ori = 0.f;

  uint16_t eid = invalid_entity;
};

struct InputState
{
  uint32_t tick = 0;
  uint16_t eid = invalid_entity;
  float thr = 0.f;
  float steer = 0.f;
};

Entity simulate_entity(const Entity &e, InputState &state, float dt, float noise_strength);
bool entities_are_similar(const Entity &serverEntity, const Entity& clientEntity);
