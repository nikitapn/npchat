#pragma once

#include "Observer.hpp"
#include "npchat_stub/npchat.hpp"
#include <unordered_map>
#include <unordered_set>

class ChatObservers : public ObserversT<npchat::ChatListener> {
private:
  // Map user ID to their chat listeners
  std::unordered_map<std::uint32_t, std::vector<npchat::ChatListener*>> user_listeners_;

  // Map chat ID to participating user IDs
  std::unordered_map<npchat::ChatId, std::unordered_set<std::uint32_t>> chat_participants_;

  void on_message_received_impl(npchat::MessageId messageId, npchat::ChatMessage message, npchat::ChatId chatId, std::uint32_t senderId) {
    // Find all participants of this chat
    auto chat_it = chat_participants_.find(chatId);
    if (chat_it == chat_participants_.end()) {
      return; // Chat not found
    }

    // Notify all participants except the sender
    for (auto userId : chat_it->second) {
      if (userId == senderId) continue; // Don't notify sender

      auto user_it = user_listeners_.find(userId);
      if (user_it != user_listeners_.end()) {
        for (auto* listener : user_it->second) {
          try {
            listener->OnMessageReceived({}, messageId, message);
          } catch (nprpc::Exception&) {
            // Remove disconnected listener - will be handled by cleanup
          }
        }
      }
    }
  }

  void on_message_delivered_impl(npchat::ChatId chatId, npchat::MessageId messageId, std::uint32_t senderId) {
    // Notify only the sender about delivery confirmation
    auto user_it = user_listeners_.find(senderId);
    if (user_it != user_listeners_.end()) {
      for (auto* listener : user_it->second) {
        try {
          listener->OnMessageDelivered({}, chatId, messageId);
        } catch (nprpc::Exception&) {
          // Remove disconnected listener - will be handled by cleanup
        }
      }
    }
  }

  void on_contact_list_updated_impl(std::uint32_t userId, npchat::ContactList contacts) {
    auto user_it = user_listeners_.find(userId);
    if (user_it != user_listeners_.end()) {
      for (auto* listener : user_it->second) {
        try {
          listener->OnContactListUpdated({}, contacts);
        } catch (nprpc::Exception&) {
          // Remove disconnected listener - will be handled by cleanup
        }
      }
    }
  }

public:
  ChatObservers() : ObserversT<npchat::ChatListener>() {}

  // Subscribe a user's listener to chat events
  void subscribe_user(std::uint32_t userId, npchat::ChatListener* listener) {
    nplib::async<false>(executor(), [this, userId, listener] {
      user_listeners_[userId].push_back(listener);
      add_impl(listener); // Add to base observer list
    });
  }

  // Unsubscribe a user's listener
  void unsubscribe_user(std::uint32_t userId, npchat::ChatListener* listener) {
    nplib::async<false>(executor(), [this, userId, listener] {
      auto user_it = user_listeners_.find(userId);
      if (user_it != user_listeners_.end()) {
        auto& listeners = user_it->second;
        listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
        if (listeners.empty()) {
          user_listeners_.erase(user_it);
        }
      }
    });
  }

  // Add chat participants mapping
  void add_chat_participants(npchat::ChatId chatId, const std::vector<std::uint32_t>& participants) {
    nplib::async<false>(executor(), [this, chatId, participants] {
      auto& chat_users = chat_participants_[chatId];
      for (auto userId : participants) {
        chat_users.insert(userId);
      }
    });
  }

  // Remove user from chat
  void remove_chat_participant(npchat::ChatId chatId, std::uint32_t userId) {
    nplib::async<false>(executor(), [this, chatId, userId] {
      auto chat_it = chat_participants_.find(chatId);
      if (chat_it != chat_participants_.end()) {
        chat_it->second.erase(userId);
        if (chat_it->second.empty()) {
          chat_participants_.erase(chat_it);
        }
      }
    });
  }

  // Broadcast new message to chat participants
  void notify_message_received(npchat::MessageId messageId, const npchat::ChatMessage& message, std::uint32_t senderId) {
    nplib::async<false>(executor(), &ChatObservers::on_message_received_impl, this, messageId, message, message.chatId, senderId);
  }

  // Notify sender about message delivery
  void notify_message_delivered(npchat::ChatId chatId, npchat::MessageId messageId, std::uint32_t senderId) {
    nplib::async<false>(executor(), &ChatObservers::on_message_delivered_impl, this, chatId, messageId, senderId);
  }

  // Notify user about contact list changes
  void notify_contact_list_updated(std::uint32_t userId, const npchat::ContactList& contacts) {
    nplib::async<false>(executor(), &ChatObservers::on_contact_list_updated_impl, this, userId, contacts);
  }
};
