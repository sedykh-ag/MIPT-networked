// initial skeleton is a clone from https://github.com/jpcy/bgfx-minimal-example
//
#include <functional>
#include "raylib.h"
#include <enet/enet.h>
#include <math.h>

#include <vector>
#include <list>
#include <iostream>
#include <cassert>
#include "entity.h"
#include "protocol.h"
#include "snapshot.h"

static std::list<Snapshot> snapshots_buffer;
static std::vector<Entity> entities;
static uint16_t my_entity = invalid_entity;

float lerp(float x, float y, float t)
{
  return (1 - t) * x + t * y;
}

Snapshot get_interpolated_snapshot(uint32_t curTime)
{
start:
  Snapshot interpSnapshot;
  for (auto &eid : interpSnapshot.eid)
    eid = invalid_entity;

  if (snapshots_buffer.size() < 2)
    return interpSnapshot;

  Snapshot from = *snapshots_buffer.begin();
  Snapshot to = *std::next(snapshots_buffer.begin());
  assert(from.time < to.time); 

  if (from.time > curTime)
  {
    return interpSnapshot;
  }

  if (to.time < curTime)
  {
    snapshots_buffer.erase(snapshots_buffer.begin());
    goto start; // lol
  }

  // from.time <= curTime <= to.time should guranteed at this point
  assert(from.time <= curTime && curTime <= to.time);

  float lerpValue = (float)(curTime - from.time) / (float)(to.time - from.time);
  for (size_t i = 0; i < entities.size(); i++)
  {
    if (from.eid[i] != to.eid[i])
    {
      std::cout << "from.eid[i] != to.eid[i]\n";
      continue;
    }
    interpSnapshot.eid[i] = to.eid[i];
    interpSnapshot.x[i] = lerp(from.x[i], to.x[i], lerpValue);
    interpSnapshot.y[i] = lerp(from.y[i], to.y[i], lerpValue);
    interpSnapshot.ori[i] = lerp(from.ori[i], to.ori[i], lerpValue);
  }
  return interpSnapshot;
}

Entity get_interpolated_entity(const Entity &from, const Entity& to, float alpha)
{
  Entity interpEntity = { .eid = invalid_entity };
  if (from.eid != to.eid)
  {
    std::cout << "from.eid != to.eid\n";
    return interpEntity;
  }

  interpEntity = {
    .color = to.color,
    .x = lerp(from.x, to.x, alpha),
    .y = lerp(from.y, to.y, alpha),
    .speed = lerp(from.speed, to.speed, alpha), // this isn't actually needed
    .ori = lerp(from.ori, to.ori, alpha),
    .eid = to.eid,
  };
  return interpEntity;
}

void on_new_entity_packet(ENetPacket *packet)
{
  Entity newEntity;
  deserialize_new_entity(packet, newEntity);
  for (const Entity &e : entities)
    if (e.eid == newEntity.eid)
      return; // don't need to do anything, we already have entity
  entities.push_back(newEntity);
}

Entity on_set_controlled_entity(ENetPacket *packet)
{
  deserialize_set_controlled_entity(packet, my_entity);
  for (const auto& e: entities)
    if (e.eid == my_entity)
      return e;

  std::cout << "ERROR on_set_controlled_entity\n";
  return Entity{ .eid = invalid_entity }; // this should never happen
}

void on_snapshot(ENetPacket *packet)
{
  Snapshot snapshot;
  deserialize_snapshot(packet, snapshot);
  // std::cout << "received snapshot with time " << snapshot.time << '\n';  
  snapshots_buffer.push_back(snapshot);
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  ENetHost *client = enet_host_create(nullptr, 1, 2, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress address;
  enet_address_set_host(&address, "localhost");
  address.port = 10131;

  ENetPeer *serverPeer = enet_host_connect(client, &address, 2, 0);
  if (!serverPeer)
  {
    printf("Cannot connect to server");
    return 1;
  }

  int width = 600;
  int height = 600;

  InitWindow(width, height, "w5 networked MIPT");

  const int scrWidth = GetMonitorWidth(0);
  const int scrHeight = GetMonitorHeight(0);
  if (scrWidth < width || scrHeight < height)
  {
    width = std::min(scrWidth, width);
    height = std::min(scrHeight - 150, height);
    SetWindowSize(width, height);
  }

  Camera2D camera = { {0, 0}, {0, 0}, 0.f, 1.f };
  camera.target = Vector2{ 0.f, 0.f };
  camera.offset = Vector2{ width * 0.5f, height * 0.5f };
  camera.rotation = 0.f;
  camera.zoom = 10.f;


  SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

  bool connected = false;
  constexpr uint32_t offsetTime = 400; // [ms]

  constexpr float fixedDt = 0.1f; // [s]
  float accumulatorTime = 0.0f;
  Entity prevEntityState = { .eid = my_entity };
  Entity curEntityState = { .eid = my_entity };

  while (!WindowShouldClose())
  {
    // std::cout << "time: " << enet_time_get() << '\n';
    ENetEvent event;
    while (enet_host_service(client, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        send_join(serverPeer);
        connected = true;
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
        case E_SERVER_TO_CLIENT_NEW_ENTITY:
          on_new_entity_packet(event.packet);
          break;
        case E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY:
          // initialize controlled entity states
          prevEntityState = on_set_controlled_entity(event.packet);
          curEntityState = prevEntityState;          
          break;
        case E_SERVER_TO_CLIENT_SNAPSHOT:
          on_snapshot(event.packet);
          break;
        };
        break;
      default:
        break;
      };
    }

    uint32_t timeCurrentOffset = enet_time_get() - offsetTime;

    Snapshot interpSnapshot = get_interpolated_snapshot(timeCurrentOffset);
    for (size_t i = 0; i < entities.size(); i++)
    {
      for (Entity &e : entities)
      {
        if (e.eid == interpSnapshot.eid[i])
        {
          e.x = interpSnapshot.x[i];
          e.y = interpSnapshot.y[i];
          e.ori = interpSnapshot.ori[i];
        }
      }
    }

    InputState inputState = { .eid = my_entity };
    if (my_entity != invalid_entity)
    {
      bool left = IsKeyDown(KEY_LEFT);
      bool right = IsKeyDown(KEY_RIGHT);
      bool up = IsKeyDown(KEY_UP);
      bool down = IsKeyDown(KEY_DOWN);

      // Update input state
      inputState = 
      {
        .eid = my_entity,
        .thr = (up ? 1.f : 0.f) + (down ? -1.f : 0.f),
        .steer = (left ? -1.f : 0.f) + (right ? 1.f : 0.f)
      };
      // Send input state
      send_input_state(serverPeer, inputState);
    }

    // local simulation of controlled entity
    float frameTime = GetFrameTime();
    accumulatorTime += frameTime;
    while (accumulatorTime >= fixedDt)
    {
      prevEntityState = curEntityState;
      simulate_entity(curEntityState, inputState, fixedDt);
      accumulatorTime -= fixedDt;
    }
    const float alpha = accumulatorTime / fixedDt;
    Entity interpEntityState = get_interpolated_entity(prevEntityState, curEntityState, alpha);

    // replace entity in vector with the interpolated one
    for (auto &e: entities)
      if (e.eid == interpEntityState.eid)
        e = interpEntityState;

    // render
    BeginDrawing();
      ClearBackground(GRAY);
      DrawText(TextFormat("RTT to server: %d [ms]", serverPeer->roundTripTime), 10, 10, 20, BLACK);
      DrawText(TextFormat("time offset: %d [ms]", offsetTime), 10, 40, 20, BLACK);
      DrawText(TextFormat("current buffer size: %d", snapshots_buffer.size()), 10, 70, 20, BLACK);
      BeginMode2D(camera);
        for (const Entity &e : entities)
        {
          const Rectangle rect = {e.x, e.y, 3.f, 1.f};
          DrawRectanglePro(rect, {0.f, 0.5f}, e.ori * 180.f / PI, GetColor(e.color));
        }

      EndMode2D();
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
