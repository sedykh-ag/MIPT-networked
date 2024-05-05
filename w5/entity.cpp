#include <cassert>
#include <cmath>
#include <cstdlib>
#include "entity.h"
#include "mathUtils.h"

float noise(float strength)
{
  return strength * ((float)rand() / RAND_MAX - 0.5f);
}

Entity simulate_entity(const Entity &e, InputState &state, float dt, float noise_strength)
{
  Entity result = e;
  result.tick++; // very important

  bool isBraking = sign(state.thr) != 0.f && sign(state.thr) != sign(e.speed);
  float accel = isBraking ? 12.f : 3.f;
  result.speed = move_to(e.speed, clamp(state.thr, -0.3f, 1.f) * 10.f, dt, accel);
  result.ori += state.steer * dt * clamp(e.speed, -2.f, 2.f) * 0.3f;
  result.x += cosf(e.ori) * e.speed * dt + noise(noise_strength);
  result.y += sinf(e.ori) * e.speed * dt + noise(noise_strength);

  return result;
}

bool entities_are_similar(const Entity &serverEntity, const Entity& clientEntity)
{
  assert(serverEntity.tick == clientEntity.tick);
  return std::abs(serverEntity.x - clientEntity.x) < X_TOL &&
         std::abs(serverEntity.y - clientEntity.y) < Y_TOL &&
         std::abs(serverEntity.speed - clientEntity.speed) < SPEED_TOL &&
         std::abs(serverEntity.ori - clientEntity.ori) < ORI_TOL;
}
