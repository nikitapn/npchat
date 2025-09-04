#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <random>
#include <chrono>
#include "npchat_stub/npchat.hpp"

#include <nplib/utils/unordered.hpp>

struct CallInfo {
  std::string callId;
  npchat::ChatId chatId;
  npchat::UserId callerId;
  npchat::UserId calleeId;
  std::string offer;
  std::string answer;
  std::vector<std::string> iceCandidates;
  bool isActive;
  std::chrono::system_clock::time_point createdAt;
};

class WebRTCService {
private:
  std::unordered_map<std::string, CallInfo, nplib::utils::string_hash, std::equal_to<>> activeCalls_;
  mutable std::mutex mutex_;

  std::string generateCallId();

public:
  WebRTCService();
  ~WebRTCService();

  // Call management
  std::string initiateCall(npchat::ChatId chatId, npchat::UserId callerId, npchat::UserId calleeId, std::string_view offer);
  bool answerCall(std::string_view callId, std::string_view answer);
  bool addIceCandidate(std::string_view callId, std::string_view candidate);
  bool endCall(std::string_view callId);

  // Call queries
  std::optional<CallInfo> getCall(std::string_view callId);
  std::vector<CallInfo> getActiveCallsForUser(npchat::UserId userId);
  std::vector<CallInfo> getActiveCallsForChat(npchat::ChatId chatId);

  // Cleanup old calls
  void cleanupExpiredCalls();
};