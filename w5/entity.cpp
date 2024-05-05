#include "entity.h"
#include "mathUtils.h"

void simulate_entity(Entity &e, InputState &state, float dt)
{
  bool isBraking = sign(state.thr) != 0.f && sign(state.thr) != sign(e.speed);
  float accel = isBraking ? 12.f : 3.f;
  e.speed = move_to(e.speed, clamp(state.thr, -0.3f, 1.f) * 10.f, dt, accel);
  e.ori += state.steer * dt * clamp(e.speed, -2.f, 2.f) * 0.3f;
  e.x += cosf(e.ori) * e.speed * dt;
  e.y += sinf(e.ori) * e.speed * dt;
}
