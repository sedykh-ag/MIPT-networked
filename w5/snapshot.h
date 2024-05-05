#pragma once
#include <cstdint>
#include "entity.h"

#define MAX_PLAYERS 16

struct Snapshot
{
  uint32_t time;
  uint32_t tick;
  uint16_t eid[MAX_PLAYERS];
  float x[MAX_PLAYERS];
  float y[MAX_PLAYERS];
  float speed[MAX_PLAYERS];
  float ori[MAX_PLAYERS];
};

Entity get_entity_with_id(const Snapshot &snapshot, uint16_t eid);
