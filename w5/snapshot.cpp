#include "snapshot.h"
#include "entity.h"

Entity get_entity_with_id(const Snapshot &snapshot, uint16_t eid)
{
  for (size_t i = 0; i < MAX_PLAYERS; i++)
  {
    if (snapshot.eid[i] == eid)
    {
      return Entity{
        .tick = snapshot.tick,
        .x = snapshot.x[i],
        .y = snapshot.y[i],
        .speed = snapshot.speed[i],
        .ori = snapshot.ori[i],
        .eid = snapshot.eid[i],
      };
    }
  }
  return Entity{ .eid = invalid_entity };
}
