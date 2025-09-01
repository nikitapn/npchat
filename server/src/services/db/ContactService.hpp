#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <sqlite3.h>
#include <spdlog/spdlog.h>
#include "Database.hpp"
#include "npchat_stub/npchat.hpp"

class ContactService {
private:
  std::shared_ptr<Database> db_;
  mutable std::mutex mutex_;

  // Prepared statements
  sqlite3_stmt* add_contact_stmt_;
  sqlite3_stmt* get_contacts_stmt_;
  sqlite3_stmt* get_contact_by_id_stmt_;
  sqlite3_stmt* get_contact_by_username_stmt_;
  sqlite3_stmt* remove_contact_stmt_;
  sqlite3_stmt* check_contact_exists_stmt_;
  sqlite3_stmt* block_contact_stmt_;
  sqlite3_stmt* unblock_contact_stmt_;
  sqlite3_stmt* get_blocked_contacts_stmt_;
  sqlite3_stmt* is_blocked_stmt_;
  sqlite3_stmt* search_users_stmt_;

public:
  explicit ContactService(const std::shared_ptr<Database>& database);
  ~ContactService();

  bool addContact(std::uint32_t owner_id, std::uint32_t contact_id);
  bool addContactByUsername(std::uint32_t owner_id, const std::string& username);
  std::vector<npchat::Contact> getContacts(std::uint32_t owner_id);
  std::optional<npchat::Contact> getContact(std::uint32_t owner_id, std::uint32_t contact_id);
  std::optional<npchat::Contact> getContactByUsername(std::uint32_t owner_id, const std::string& username);
  bool removeContact(std::uint32_t owner_id, std::uint32_t contact_id);
  bool blockContact(std::uint32_t owner_id, std::uint32_t contact_id);
  bool unblockContact(std::uint32_t owner_id, std::uint32_t contact_id);
  std::vector<npchat::Contact> getBlockedContacts(std::uint32_t owner_id);
  bool isBlocked(std::uint32_t owner_id, std::uint32_t contact_id);
  std::vector<npchat::Contact> searchUsers(std::uint32_t searcher_id, const std::string& query, std::uint32_t limit = 20);

private:
  struct User {
    std::uint32_t id;
    std::string username;
    std::string email;
  };

  std::optional<User> getUserByUsername(const std::string& username);
};
