#pragma once

#include <spdlog/spdlog.h>

#include "npchat_stub/npchat.hpp"
#include "services/client/Observer.hpp"

class ChatImpl
  : public npchat::IChat_Servant
  , public ObserversT<npchat::ChatParticipant> 
{
  void send_to_all_impl(npchat::ChatMessage msg, nprpc::EndPoint endpoint) {
    broadcast(&npchat::ChatParticipant::OnMessage, not_equal_to_endpoint_t{ endpoint }, std::ref(msg));
  }

  void send_to_all(npchat::ChatMessage&& msg, nprpc::EndPoint endpoint) {
    nplib::async<false>(executor(), &ChatImpl::send_to_all_impl, this, std::move(msg), std::move(endpoint));
  }
public:
  virtual void Connect(nprpc::Object *obj) {
    if (auto participant = nprpc::narrow<npchat::ChatParticipant>(obj); participant) {
      participant->add_ref();
      participant->set_timeout(250);
      add(participant);
    }
  }

  virtual bool Send(npchat::flat::ChatMessage_Direct msg) {
    auto timestamp = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::minutes>(
      std::chrono::system_clock::now().time_since_epoch()).count());
    send_to_all(npchat::ChatMessage{timestamp, (std::string)msg.str()}, nprpc::get_context().remote_endpoint);
    return true;
  }

  ~ChatImpl() {
    spdlog::info("ChatImpl destroyed");
  }
};
