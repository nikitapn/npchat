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
  explicit ContactService(const std::shared_ptr<Database>& database) : db_(database) {
    add_contact_stmt_ = db_->prepareStatement(
      "INSERT INTO contacts (owner_id, contact_id, added_at) VALUES (?, ?, ?)");

    get_contacts_stmt_ = db_->prepareStatement(
      "SELECT c.contact_id, u.username, u.email, c.added_at, c.blocked "
      "FROM contacts c "
      "JOIN users u ON c.contact_id = u.id "
      "WHERE c.owner_id = ? AND c.blocked = 0 "
      "ORDER BY u.username ASC");

    get_contact_by_id_stmt_ = db_->prepareStatement(
      "SELECT c.contact_id, u.username, u.email, c.added_at, c.blocked "
      "FROM contacts c "
      "JOIN users u ON c.contact_id = u.id "
      "WHERE c.owner_id = ? AND c.contact_id = ?");

    get_contact_by_username_stmt_ = db_->prepareStatement(
      "SELECT c.contact_id, u.username, u.email, c.added_at, c.blocked "
      "FROM contacts c "
      "JOIN users u ON c.contact_id = u.id "
      "WHERE c.owner_id = ? AND u.username = ?");

    remove_contact_stmt_ = db_->prepareStatement(
      "DELETE FROM contacts WHERE owner_id = ? AND contact_id = ?");

    check_contact_exists_stmt_ = db_->prepareStatement(
      "SELECT 1 FROM contacts WHERE owner_id = ? AND contact_id = ?");

    block_contact_stmt_ = db_->prepareStatement(
      "UPDATE contacts SET blocked = 1 WHERE owner_id = ? AND contact_id = ?");

    unblock_contact_stmt_ = db_->prepareStatement(
      "UPDATE contacts SET blocked = 0 WHERE owner_id = ? AND contact_id = ?");

    get_blocked_contacts_stmt_ = db_->prepareStatement(
      "SELECT c.contact_id, u.username, u.email, c.added_at "
      "FROM contacts c "
      "JOIN users u ON c.contact_id = u.id "
      "WHERE c.owner_id = ? AND c.blocked = 1 "
      "ORDER BY u.username ASC");

    is_blocked_stmt_ = db_->prepareStatement(
      "SELECT blocked FROM contacts WHERE owner_id = ? AND contact_id = ?");

    search_users_stmt_ = db_->prepareStatement(
      "SELECT id, username, email FROM users "
      "WHERE (username LIKE ? OR email LIKE ?) AND id != ? "
      "ORDER BY username ASC LIMIT ?");
  }

  ~ContactService() {
    sqlite3_finalize(add_contact_stmt_);
    sqlite3_finalize(get_contacts_stmt_);
    sqlite3_finalize(get_contact_by_id_stmt_);
    sqlite3_finalize(get_contact_by_username_stmt_);
    sqlite3_finalize(remove_contact_stmt_);
    sqlite3_finalize(check_contact_exists_stmt_);
    sqlite3_finalize(block_contact_stmt_);
    sqlite3_finalize(unblock_contact_stmt_);
    sqlite3_finalize(get_blocked_contacts_stmt_);
    sqlite3_finalize(is_blocked_stmt_);
    sqlite3_finalize(search_users_stmt_);
  }

  bool addContact(std::uint32_t owner_id, std::uint32_t contact_id) {
    if (owner_id == contact_id) {
      return false; // Cannot add self as contact
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // Check if contact already exists
    sqlite3_bind_int(check_contact_exists_stmt_, 1, owner_id);
    sqlite3_bind_int(check_contact_exists_stmt_, 2, contact_id);

    bool exists = (sqlite3_step(check_contact_exists_stmt_) == SQLITE_ROW);
    sqlite3_reset(check_contact_exists_stmt_);

    if (exists) {
      return false; // Contact already exists
    }

    // Add contact
    std::uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();

    sqlite3_bind_int(add_contact_stmt_, 1, owner_id);
    sqlite3_bind_int(add_contact_stmt_, 2, contact_id);
    sqlite3_bind_int64(add_contact_stmt_, 3, timestamp);

    bool success = (sqlite3_step(add_contact_stmt_) == SQLITE_DONE);
    sqlite3_reset(add_contact_stmt_);

    return success;
  }

  bool addContactByUsername(std::uint32_t owner_id, const std::string& username) {
    // First find the user by username
    auto user_opt = getUserByUsername(username);
    if (!user_opt.has_value()) {
      return false; // User not found
    }

    return addContact(owner_id, user_opt->id);
  }

  std::vector<npchat::Contact> getContacts(std::uint32_t owner_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<npchat::Contact> contacts;

    sqlite3_bind_int(get_contacts_stmt_, 1, owner_id);

    while (sqlite3_step(get_contacts_stmt_) == SQLITE_ROW) {
      npchat::Contact contact;
      contact.id = sqlite3_column_int(get_contacts_stmt_, 0);
      contact.username = reinterpret_cast<const char*>(sqlite3_column_text(get_contacts_stmt_, 1));
      // contact.email = reinterpret_cast<const char*>(sqlite3_column_text(get_contacts_stmt_, 2));

      contacts.push_back(std::move(contact));
    }

    sqlite3_reset(get_contacts_stmt_);
    return contacts;
  }

  std::optional<npchat::Contact> getContact(std::uint32_t owner_id, std::uint32_t contact_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    sqlite3_bind_int(get_contact_by_id_stmt_, 1, owner_id);
    sqlite3_bind_int(get_contact_by_id_stmt_, 2, contact_id);

    if (sqlite3_step(get_contact_by_id_stmt_) == SQLITE_ROW) {
      npchat::Contact contact;
      contact.id = sqlite3_column_int(get_contact_by_id_stmt_, 0);
      contact.username = reinterpret_cast<const char*>(sqlite3_column_text(get_contact_by_id_stmt_, 1));
      // contact.email = reinterpret_cast<const char*>(sqlite3_column_text(get_contact_by_id_stmt_, 2));

      sqlite3_reset(get_contact_by_id_stmt_);
      return contact;
    }

    sqlite3_reset(get_contact_by_id_stmt_);
    return std::nullopt;
  }

  std::optional<npchat::Contact> getContactByUsername(std::uint32_t owner_id, const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);

    sqlite3_bind_int(get_contact_by_username_stmt_, 1, owner_id);
    sqlite3_bind_text(get_contact_by_username_stmt_, 2, username.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(get_contact_by_username_stmt_) == SQLITE_ROW) {
      npchat::Contact contact;
      contact.id = sqlite3_column_int(get_contact_by_username_stmt_, 0);
      contact.username = reinterpret_cast<const char*>(sqlite3_column_text(get_contact_by_username_stmt_, 1));
      // contact.email = reinterpret_cast<const char*>(sqlite3_column_text(get_contact_by_username_stmt_, 2));

      sqlite3_reset(get_contact_by_username_stmt_);
      return contact;
    }

    sqlite3_reset(get_contact_by_username_stmt_);
    return std::nullopt;
  }

  bool removeContact(std::uint32_t owner_id, std::uint32_t contact_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    sqlite3_bind_int(remove_contact_stmt_, 1, owner_id);
    sqlite3_bind_int(remove_contact_stmt_, 2, contact_id);

    bool success = (sqlite3_step(remove_contact_stmt_) == SQLITE_DONE);
    sqlite3_reset(remove_contact_stmt_);

    return success && sqlite3_changes(db_->getConnection()) > 0;
  }

  bool blockContact(std::uint32_t owner_id, std::uint32_t contact_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    sqlite3_bind_int(block_contact_stmt_, 1, owner_id);
    sqlite3_bind_int(block_contact_stmt_, 2, contact_id);

    bool success = (sqlite3_step(block_contact_stmt_) == SQLITE_DONE);
    sqlite3_reset(block_contact_stmt_);

    return success && sqlite3_changes(db_->getConnection()) > 0;
  }

  bool unblockContact(std::uint32_t owner_id, std::uint32_t contact_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    sqlite3_bind_int(unblock_contact_stmt_, 1, owner_id);
    sqlite3_bind_int(unblock_contact_stmt_, 2, contact_id);

    bool success = (sqlite3_step(unblock_contact_stmt_) == SQLITE_DONE);
    sqlite3_reset(unblock_contact_stmt_);

    return success && sqlite3_changes(db_->getConnection()) > 0;
  }

  std::vector<npchat::Contact> getBlockedContacts(std::uint32_t owner_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<npchat::Contact> contacts;

    sqlite3_bind_int(get_blocked_contacts_stmt_, 1, owner_id);

    while (sqlite3_step(get_blocked_contacts_stmt_) == SQLITE_ROW) {
      npchat::Contact contact;
      contact.id = sqlite3_column_int(get_blocked_contacts_stmt_, 0);
      contact.username = reinterpret_cast<const char*>(sqlite3_column_text(get_blocked_contacts_stmt_, 1));
      // contact.email = reinterpret_cast<const char*>(sqlite3_column_text(get_blocked_contacts_stmt_, 2));

      contacts.push_back(std::move(contact));
    }

    sqlite3_reset(get_blocked_contacts_stmt_);
    return contacts;
  }

  bool isBlocked(std::uint32_t owner_id, std::uint32_t contact_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    sqlite3_bind_int(is_blocked_stmt_, 1, owner_id);
    sqlite3_bind_int(is_blocked_stmt_, 2, contact_id);

    bool blocked = false;
    if (sqlite3_step(is_blocked_stmt_) == SQLITE_ROW) {
      blocked = sqlite3_column_int(is_blocked_stmt_, 0) == 1;
    }

    sqlite3_reset(is_blocked_stmt_);
    return blocked;
  }

  std::vector<npchat::Contact> searchUsers(std::uint32_t searcher_id, const std::string& query, std::uint32_t limit = 20) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<npchat::Contact> users;

    std::string search_pattern = "%" + query + "%";

    sqlite3_bind_text(search_users_stmt_, 1, search_pattern.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(search_users_stmt_, 2, search_pattern.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(search_users_stmt_, 3, searcher_id);
    sqlite3_bind_int(search_users_stmt_, 4, limit);

    while (sqlite3_step(search_users_stmt_) == SQLITE_ROW) {
      npchat::Contact user;
      user.id = sqlite3_column_int(search_users_stmt_, 0);
      user.username = reinterpret_cast<const char*>(sqlite3_column_text(search_users_stmt_, 1));
      // user.email = reinterpret_cast<const char*>(sqlite3_column_text(search_users_stmt_, 2));

      users.push_back(std::move(user));
    }

    sqlite3_reset(search_users_stmt_);
    return users;
  }

private:
  struct User {
    std::uint32_t id;
    std::string username;
    std::string email;
  };

  std::optional<User> getUserByUsername(const std::string& username) {
    sqlite3_stmt* stmt = db_->prepareStatement(
      "SELECT id, username, email FROM users WHERE username = ?");

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
      User user;
      user.id = sqlite3_column_int(stmt, 0);
      user.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
      user.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

      sqlite3_finalize(stmt);
      return user;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
  }
};
