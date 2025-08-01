#pragma once

#include "Observer.hpp"
#include "npchat_stub/npchat.hpp"

class DataObservers : public ObserversT<npchat::DataObserver> {
  uint32_t alarm_id_ = 0;

  npchat::Alarm make_alarm(npchat::AlarmType type, std::string&& msg) {
    return { alarm_id_++, type, msg };
  }

  void alarm_impl(npchat::AlarmType type, std::string msg) {
    broadcast(&npchat::DataObserver::OnAlarm, no_condition, make_alarm(type, std::move(msg)));
  }

public:
  void alarm(npchat::AlarmType type, std::string&& msg) {
    nplib::async<false>(executor(), &DataObservers::alarm_impl, this, type, std::move(msg));
  }

  void footstep(npchat::Footstep&& footstep, const nprpc::EndPoint& endpoint) {
    nplib::async<false>(executor(), [this, footstep = std::move(footstep), endpoint] {
      broadcast(&npchat::DataObserver::OnFootstep, not_equal_to_endpoint_t{ endpoint }, footstep);
    });
  }
};