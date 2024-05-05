#pragma once
#include <cstdint>

constexpr float fixedDt = 0.1f; // [s], duration of one tick
constexpr uint32_t snapshotsInterval = 5; // [ticks], interval between server's sent snapshots
constexpr uint32_t offsetTime = 1000; // [ms], offset for on-client interpolation between server's snapshots
                                     // (only concerns entities controlled by other users)
constexpr float serverNoise = 0.0f; // used to get noisy simulation on server to test physics rerollouts on client (who is simulating without noise)

// constants for physics resimulation criterion
constexpr float X_TOL = 0.1f; 
constexpr float Y_TOL = 0.1f;
constexpr float SPEED_TOL = 0.1f;
constexpr float ORI_TOL = 0.1f;