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
#include "constants.h"

static std::vector<Entity> entity_state_history;
static std::vector<InputState> input_state_history;
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
      // std::cout << "from.eid[i] != to.eid[i]\n";
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
    // std::cout << "from.eid != to.eid\n";
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

void on_set_controlled_entity(ENetPacket *packet, uint32_t tick)
{
  deserialize_set_controlled_entity(packet, my_entity);
  for (const Entity &e : entities)
  {
    if (e.eid == my_entity)
    {
      Entity ent = e;
      ent.tick = tick;
      // we need tick for physics resimulations
      entity_state_history.push_back(ent);
      input_state_history.push_back(InputState{});
    }
  }
  std::cout << "received control over entity with eid: " << my_entity << '\n';
}

void on_snapshot(ENetPacket *packet)
{
  Snapshot snapshot;
  deserialize_snapshot(packet, snapshot);
  snapshots_buffer.push_back(snapshot);
}

void validate_physics()
{
  if (snapshots_buffer.empty())
    return;
  
  Snapshot snapshot = snapshots_buffer.back();

  // compare server and client entities
  assert (entity_state_history.size() == input_state_history.size());
  Entity serverEntityState = get_entity_with_id(snapshot, my_entity);
  assert(serverEntityState.eid == my_entity);
  Entity clientEntityState;

  size_t idx = -1;
  for (size_t i = 0; i < entity_state_history.size(); i++)
  {
    if (entity_state_history[i].tick == serverEntityState.tick)
    {
      clientEntityState = entity_state_history[i];
      idx = i;
    }
  }
  serverEntityState.color = clientEntityState.color; // server snapshot does not store color
  if (idx == -1) { return; } // this should not normally happen if the server is not ahead of the client
  assert(clientEntityState.eid == my_entity);
  assert(serverEntityState.tick == clientEntityState.tick);

  // validation
  if (entities_are_similar(serverEntityState, clientEntityState))
  {
    std::cout << "validation successful at tick " << serverEntityState.tick << '\n';
    return;
  }

  // physics resimulation
  std::cout << "resimulating at tick " << clientEntityState.tick << '\n';
  entity_state_history[idx] = serverEntityState;
  for (size_t i = idx; i + 1 < input_state_history.size(); i++)
  {
    entity_state_history[i+1] = simulate_entity(entity_state_history[i], input_state_history[i], fixedDt, 0.0f);
  }
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
  float accumulatorTime = 0.0f;
  uint32_t tick = 0;

  while (!WindowShouldClose())
  {
    float curTime = enet_time_get() * 0.001f; // current time in seconds
    uint32_t timeCurrentOffset = enet_time_get() - offsetTime;
    tick = (uint32_t)ceil(curTime / fixedDt);

    ENetEvent event;
    while (enet_host_service(client, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        send_join(serverPeer);
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
        case E_SERVER_TO_CLIENT_NEW_ENTITY:
          on_new_entity_packet(event.packet);
          break;
        case E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY:
          // initialize controlled entity states
          on_set_controlled_entity(event.packet, tick);
          connected = true;
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

    if (!connected) // don't do anything until connected
      continue;

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
    }

    // local simulation of controlled entity
    float frameTime = GetFrameTime();
    accumulatorTime += frameTime;
    while (accumulatorTime >= fixedDt)
    {
      // time and tick
      accumulatorTime -= fixedDt;
      // std::cout << "tick: " << tick << '\n';

      // input state
      inputState.tick = tick;
      input_state_history.push_back(inputState);
      send_input_state(serverPeer, inputState);
      // std::cout << "sent input with tick " << tick << '\n';

      // simulation
      Entity currentEntityState = simulate_entity(entity_state_history.back(), inputState, fixedDt, 0.0f);
      currentEntityState.tick = tick;
      entity_state_history.push_back(currentEntityState);

      // validation
      validate_physics();

      // buffers cleanup
      if (entity_state_history.size() > 200)
      {
        entity_state_history.erase(entity_state_history.begin(), entity_state_history.begin() + 100);
        input_state_history.erase(input_state_history.begin(), input_state_history.begin() + 100);
      }
    }
    // interpolation of controlled entity BETWEEN TICKS
    const float alpha = accumulatorTime / fixedDt;
    Entity interpEntityState = { .eid = invalid_entity };
    if (entity_state_history.size() > 1)
    {
      interpEntityState = get_interpolated_entity(
        entity_state_history.end()[-2], entity_state_history.end()[-1], alpha);
    }
    // replace entity in vector with the interpolated one
    for (auto &e: entities)
      if (e.eid == interpEntityState.eid)
        e = interpEntityState;

    // render
    BeginDrawing();
      ClearBackground(GRAY);
      DrawText(TextFormat("RTT to server: %d [ms]", serverPeer->roundTripTime), 10, 10, 20, BLACK);
      DrawText(TextFormat("time offset: %d [ms]", offsetTime), 10, 40, 20, BLACK);
      DrawText(TextFormat("snapshots buffer size: %d", snapshots_buffer.size()), 10, 70, 20, BLACK);
      DrawText(TextFormat("local inputs history size: %d", input_state_history.size()), 10, 100, 20, BLACK);
      DrawText(TextFormat("local states history size: %d", entity_state_history.size()), 10, 130, 20, BLACK);
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
