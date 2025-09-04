#include "WebRTCService.hpp"
#include <spdlog/spdlog.h>
#include <sstream>
#include <iomanip>

WebRTCService::WebRTCService() {
  spdlog::info("WebRTCService initialized");
}

WebRTCService::~WebRTCService() {
  spdlog::info("WebRTCService destroyed");
}

std::string WebRTCService::generateCallId() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 15);

  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  for (int i = 0; i < 32; ++i) {
    ss << dis(gen);
  }
  return ss.str();
}

std::string WebRTCService::initiateCall(npchat::ChatId chatId, npchat::UserId callerId, npchat::UserId calleeId, std::string_view offer) {
  std::lock_guard<std::mutex> lock(mutex_);

  std::string callId = generateCallId();

  CallInfo callInfo{
    callId,
    chatId,
    callerId,
    calleeId,
    std::string(offer),
    "",
    {},
    true,
    std::chrono::system_clock::now()
  };

  activeCalls_[callId] = callInfo;

  spdlog::info("Call initiated: {} in chat {} from {} to {}", callId, chatId, callerId, calleeId);
  return callId;
}

bool WebRTCService::answerCall(std::string_view callId, std::string_view answer) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = activeCalls_.find(callId);
  if (it == activeCalls_.end()) {
    spdlog::warn("Call not found for answering: {}", callId);
    return false;
  }

  it->second.answer = answer;
  spdlog::info("Call answered: {}", callId);
  return true;
}

bool WebRTCService::addIceCandidate(std::string_view callId, std::string_view candidate) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = activeCalls_.find(callId);
  if (it == activeCalls_.end()) {
    spdlog::warn("Call not found for ICE candidate: {}", callId);
    return false;
  }

  it->second.iceCandidates.emplace_back(candidate);
  spdlog::debug("ICE candidate added to call: {}", callId);
  return true;
}

bool WebRTCService::endCall(std::string_view callId) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = activeCalls_.find(callId);
  if (it == activeCalls_.end()) {
    spdlog::warn("Call not found for ending: {}", callId);
    return false;
  }

  it->second.isActive = false;
  spdlog::info("Call ended: {}", callId);
  return true;
}

std::optional<CallInfo> WebRTCService::getCall(std::string_view callId) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = activeCalls_.find(callId);
  if (it == activeCalls_.end()) {
    return std::nullopt;
  }

  return it->second;
}

std::vector<CallInfo> WebRTCService::getActiveCallsForUser(npchat::UserId userId) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<CallInfo> userCalls;

  for (const auto& [callId, callInfo] : activeCalls_) {
    if (callInfo.isActive && (callInfo.callerId == userId || callInfo.calleeId == userId)) {
      userCalls.push_back(callInfo);
    }
  }

  return userCalls;
}

std::vector<CallInfo> WebRTCService::getActiveCallsForChat(npchat::ChatId chatId) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<CallInfo> chatCalls;

  for (const auto& [callId, callInfo] : activeCalls_) {
    if (callInfo.isActive && callInfo.chatId == chatId) {
      chatCalls.push_back(callInfo);
    }
  }

  return chatCalls;
}

void WebRTCService::cleanupExpiredCalls() {
  std::lock_guard<std::mutex> lock(mutex_);

  auto now = std::chrono::system_clock::now();
  auto cutoff = now - std::chrono::hours(24); // Calls older than 24 hours

  for (auto it = activeCalls_.begin(); it != activeCalls_.end();) {
    if (it->second.createdAt < cutoff) {
      spdlog::info("Cleaning up expired call: {}", it->first);
      it = activeCalls_.erase(it);
    } else {
      ++it;
    }
  }
}