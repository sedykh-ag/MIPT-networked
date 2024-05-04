#pragma once

#include <cstdint>

#define MAX_PLAYERS 16


struct Snapshot
{
  uint32_t time;
  uint16_t eid[MAX_PLAYERS];
  float x[MAX_PLAYERS];
  float y[MAX_PLAYERS];
  float ori[MAX_PLAYERS];
};
