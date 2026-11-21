#pragma once
#include <stdint.h>

// Milliseconds from first completed telegram, separately for each direction.
// up: first lift / half height / fully open; down: half height / bottom / slats closed.
struct PositionCalibration {
  uint32_t up[3] = {};
  uint32_t down[3] = {};
  bool valid() const {
    return up[0] < up[1] && up[1] < up[2] && up[2] - up[1] >= 500 && up[1] - up[0] >= 500 &&
           down[0] >= 500 && down[0] < down[1] && down[1] - down[0] >= 500 && down[1] < down[2] &&
           up[2] >= 2000 && down[2] >= 2000 && up[2] <= 180000 && down[2] <= 180000;
  }
  uint32_t timeTo(uint8_t percent, bool opening) const {
    if (opening) {
      if (percent == 0) return 0;
      if (percent <= 50) return up[0] + uint64_t(up[1] - up[0]) * percent / 50;
      return up[1] + uint64_t(up[2] - up[1]) * (percent - 50) / 50;
    }
    if (percent == 0) return down[2];
    if (percent >= 50) return uint64_t(down[0]) * (100 - percent) / 50;
    return down[0] + uint64_t(down[1] - down[0]) * (50 - percent) / 50;
  }
  uint32_t referenceTime(bool opening) const {
    const uint32_t t = opening ? up[2] : down[2];
    const uint32_t margin = t / 20 < 1000 ? 1000 : (t / 20 > 3000 ? 3000 : t / 20);
    return t + margin;
  }
};

class PositionMove {
public:
  enum Phase : uint8_t { Idle, Reference, Settle, Target };
  enum Action : uint8_t { None, MoveToTarget, Stop, Complete };
  Phase phase = Idle;
  uint8_t channel = 0, target = 0;
  uint32_t since = 0, duration = 0;
  bool referenceUp() const { return target >= 50; }
  bool opening = false;
  bool targetUp() const { return opening; }
  bool active() const { return phase != Idle; }
  uint32_t stopSendAt() const { return since + (duration > 110 ? duration - 110 : 0); }
  void begin(uint8_t ch, uint8_t goal, const PositionCalibration &cal, uint32_t sentAt) {
    channel = ch; target = goal; since = sentAt; opening = !referenceUp();
    duration = cal.referenceTime(referenceUp()); phase = Reference;
  }
  void targetStarted(const PositionCalibration &cal, uint32_t sentAt) {
    since = sentAt; duration = cal.timeTo(target, targetUp()); phase = Target;
  }
  void beginDirect(uint8_t ch, uint8_t goal, uint8_t current, const PositionCalibration &cal, uint32_t sentAt) {
    channel = ch; target = goal; since = sentAt; opening = goal > current;
    const uint32_t from = cal.timeTo(current, opening), to = cal.timeTo(goal, opening);
    duration = to - from;
    phase = Target;
  }
  Action next(uint32_t now) {
    const uint32_t elapsed = now - since; // wrap-safe for the bounded durations
    if (phase == Reference && elapsed >= duration) {
      if (target == 0 || target == 100) { phase = Idle; return Complete; }
      phase = Settle; since = now;
    } else if (phase == Settle && elapsed >= 600) {
      return MoveToTarget;
    } else if (phase == Target && elapsed >= (duration > 110 ? duration - 110 : 0)) {
      return Stop; // allow one ~101 ms telegram for the receiver to recognize STOP
    }
    return None;
  }
  void cancel() { phase = Idle; }
};
