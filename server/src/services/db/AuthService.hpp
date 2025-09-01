#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <optional>
#include <sqlite3.h>
#include <openssl/sha.h>
#include <boost/uuid/random_generator.hpp>
#include <openssl/rand.h>
#include <spdlog/spdlog.h>
#include "Database.hpp"
#include "npchat_stub/npchat.hpp"

class AuthService {
public:
  struct User {
    std::uint32_t id;
    std::string username;
    std::string email;
    std::string password_hash;
    std::string session_id;
    std::uint64_t session_expires;
    bool is_verified;
    std::uint32_t verification_code;
    std::uint64_t verification_expires;
  };

  struct PendingRegistration {
    std::string username;
    std::string email;
    std::string password_hash;
    std::uint32_t verification_code;
    std::uint64_t expires_at;
  };

private:
  std::shared_ptr<Database> db_;
  mutable std::mutex mutex_;

  // Prepared statements
  sqlite3_stmt* insert_user_stmt_;
  sqlite3_stmt* get_user_by_login_stmt_;
  sqlite3_stmt* get_user_by_session_stmt_;
  sqlite3_stmt* insert_session_stmt_;
  sqlite3_stmt* update_session_stmt_;
  sqlite3_stmt* delete_session_stmt_;
  sqlite3_stmt* check_username_stmt_;
  sqlite3_stmt* check_email_stmt_;
  sqlite3_stmt* insert_pending_stmt_;
  sqlite3_stmt* get_pending_stmt_;
  sqlite3_stmt* delete_pending_stmt_;
  sqlite3_stmt* cleanup_expired_stmt_;

  // In-memory cache for sessions
  std::unordered_map<std::string, std::uint32_t> active_sessions_;

  static std::string generateSessionId() {
    // Use cryptographically secure random bytes (32 bytes = 256 bits)
    // This provides much stronger security than UUIDs for session tokens
    constexpr size_t session_id_bytes = 32;
    unsigned char random_bytes[session_id_bytes];

    // Generate cryptographically secure random bytes using OpenSSL
    if (RAND_bytes(random_bytes, session_id_bytes) != 1) {
      spdlog::error("Failed to generate secure random bytes for session ID");
      throw std::runtime_error("Failed to generate secure session ID");
    }

    // Convert to hex string (64 characters)
    std::stringstream ss;
    for (size_t i = 0; i < session_id_bytes; ++i) {
      ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(random_bytes[i]);
    }

    return ss.str();
  }

  static std::string hashPassword(std::string_view password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, password.data(), password.size());
    SHA256_Final(hash, &sha256);
    return std::string(reinterpret_cast<char*>(hash), SHA256_DIGEST_LENGTH);
  }

  static std::uint32_t generateVerificationCode() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<std::uint32_t> dis(100000, 999999);
    return dis(gen);
  }

  static std::uint64_t currentTimestamp() {
    return std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
  }

public:
  explicit AuthService(const std::shared_ptr<Database>& database) : db_(database) {
    spdlog::info("Initializing AuthService");
    // Prepare all statements
    insert_user_stmt_ = db_->prepareStatement(
      "INSERT INTO users (username, email, password_hash, created_at, is_active) VALUES (?, ?, ?, ?, 1)");

    get_user_by_login_stmt_ = db_->prepareStatement(
      "SELECT id, username, email, password_hash FROM users WHERE (username = ? OR email = ?) AND is_active = 1");

    get_user_by_session_stmt_ = db_->prepareStatement(
      "SELECT u.id, u.username, u.email FROM users u "
      "JOIN user_sessions s ON u.id = s.user_id "
      "WHERE s.session_token = ? AND s.expires_at > ? AND u.is_active = 1");

    insert_session_stmt_ = db_->prepareStatement(
      "INSERT INTO user_sessions (user_id, session_token, created_at, expires_at, last_activity) VALUES (?, ?, ?, ?, ?)");

    update_session_stmt_ = db_->prepareStatement(
      "UPDATE user_sessions SET last_activity = ? WHERE session_token = ?");

    delete_session_stmt_ = db_->prepareStatement(
      "DELETE FROM user_sessions WHERE session_token = ?");

    check_username_stmt_ = db_->prepareStatement(
      "SELECT COUNT(*) FROM users WHERE LOWER(username) = LOWER(?) "
      "UNION ALL "
      "SELECT COUNT(*) FROM pending_registrations WHERE LOWER(username) = LOWER(?)");

    check_email_stmt_ = db_->prepareStatement(
      "SELECT COUNT(*) FROM users WHERE LOWER(email) = LOWER(?) "
      "UNION ALL "
      "SELECT COUNT(*) FROM pending_registrations WHERE LOWER(email) = LOWER(?)");

    insert_pending_stmt_ = db_->prepareStatement(
      "INSERT OR REPLACE INTO pending_registrations (username, email, password_hash, verification_code, created_at, expires_at) "
      "VALUES (?, ?, ?, ?, ?, ?)");

    get_pending_stmt_ = db_->prepareStatement(
      "SELECT username, email, password_hash FROM pending_registrations "
      "WHERE username = ? AND verification_code = ? AND expires_at > ?");

    delete_pending_stmt_ = db_->prepareStatement(
      "DELETE FROM pending_registrations WHERE username = ?");

    cleanup_expired_stmt_ = db_->prepareStatement(
      "DELETE FROM pending_registrations WHERE expires_at <= ?");
  }

  ~AuthService() {
    sqlite3_finalize(insert_user_stmt_);
    sqlite3_finalize(get_user_by_login_stmt_);
    sqlite3_finalize(get_user_by_session_stmt_);
    sqlite3_finalize(insert_session_stmt_);
    sqlite3_finalize(update_session_stmt_);
    sqlite3_finalize(delete_session_stmt_);
    sqlite3_finalize(check_username_stmt_);
    sqlite3_finalize(check_email_stmt_);
    sqlite3_finalize(insert_pending_stmt_);
    sqlite3_finalize(get_pending_stmt_);
    sqlite3_finalize(delete_pending_stmt_);
    sqlite3_finalize(cleanup_expired_stmt_);
  }

  // Authentication methods
  npchat::UserData logIn(std::string_view login, std::string_view password) {
    std::lock_guard<std::mutex> lock(mutex_);

    sqlite3_bind_text(get_user_by_login_stmt_, 1, login.data(), login.size(), SQLITE_STATIC);
    sqlite3_bind_text(get_user_by_login_stmt_, 2, login.data(), login.size(), SQLITE_STATIC);

    if (sqlite3_step(get_user_by_login_stmt_) == SQLITE_ROW) {
      std::uint32_t user_id = sqlite3_column_int(get_user_by_login_stmt_, 0);
      std::string username = reinterpret_cast<const char*>(sqlite3_column_text(get_user_by_login_stmt_, 1));
      std::string stored_hash = std::string(
        reinterpret_cast<const char*>(sqlite3_column_blob(get_user_by_login_stmt_, 3)),
        sqlite3_column_bytes(get_user_by_login_stmt_, 3));

      sqlite3_reset(get_user_by_login_stmt_);

      if (stored_hash == hashPassword(password)) {
        // Generate new session
        std::string session_id = generateSessionId();
        std::uint64_t current_time = currentTimestamp();
        std::uint64_t expires = current_time + (30 * 24 * 60 * 60); // 30 days

        sqlite3_bind_int(insert_session_stmt_, 1, user_id);
        sqlite3_bind_text(insert_session_stmt_, 2, session_id.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(insert_session_stmt_, 3, current_time);
        sqlite3_bind_int64(insert_session_stmt_, 4, expires);
        sqlite3_bind_int64(insert_session_stmt_, 5, current_time);
        sqlite3_step(insert_session_stmt_);
        sqlite3_reset(insert_session_stmt_);

        active_sessions_[session_id] = user_id;

        npchat::UserData user_data;
        user_data.name = username;
        user_data.sessionId = session_id;
        // user_data.registeredUser will be set by the caller
        return user_data;
      }
    }

    sqlite3_reset(get_user_by_login_stmt_);
    throw npchat::AuthorizationFailed{npchat::AuthorizationError::InvalidCredentials};
  }

  npchat::UserData logInWithSessionId(std::string_view session_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check cache first
    std::string session_str(session_id); // Need string for map lookup
    auto it = active_sessions_.find(session_str);
    if (it != active_sessions_.end()) {
      sqlite3_bind_text(get_user_by_session_stmt_, 1, session_id.data(), session_id.size(), SQLITE_STATIC);
      sqlite3_bind_int64(get_user_by_session_stmt_, 2, currentTimestamp());

      if (sqlite3_step(get_user_by_session_stmt_) == SQLITE_ROW) {
        std::string username = reinterpret_cast<const char*>(sqlite3_column_text(get_user_by_session_stmt_, 1));
        sqlite3_reset(get_user_by_session_stmt_);

        npchat::UserData user_data;
        user_data.name = username;
        user_data.sessionId = session_str;
        return user_data;
      }
      sqlite3_reset(get_user_by_session_stmt_);

      // Remove from cache if DB lookup failed
      active_sessions_.erase(it);
    }

    throw npchat::AuthorizationFailed{npchat::AuthorizationError::AccessDenied};
  }

  std::uint32_t getUserIdFromSession(std::string_view session_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string session_str(session_id);

    // Check cache first
    if (auto it = active_sessions_.find(session_str); it != active_sessions_.end()) {
      return it->second;
    }

    sqlite3_bind_text(get_user_by_session_stmt_, 1, session_id.data(), session_id.size(), SQLITE_STATIC);

    if (sqlite3_step(get_user_by_session_stmt_) == SQLITE_ROW) {
      std::uint32_t user_id = sqlite3_column_int(get_user_by_session_stmt_, 0);
      sqlite3_reset(get_user_by_session_stmt_);
      return user_id;
    }

    sqlite3_reset(get_user_by_session_stmt_);
    throw npchat::AuthorizationFailed{npchat::AuthorizationError::AccessDenied};
  }

  std::uint32_t getUserIdFromLogin(std::string_view login) {
    std::lock_guard<std::mutex> lock(mutex_);

    sqlite3_bind_text(get_user_by_login_stmt_, 1, login.data(), login.size(), SQLITE_STATIC);
    sqlite3_bind_text(get_user_by_login_stmt_, 2, login.data(), login.size(), SQLITE_STATIC);

    if (sqlite3_step(get_user_by_login_stmt_) == SQLITE_ROW) {
      std::uint32_t user_id = sqlite3_column_int(get_user_by_login_stmt_, 0);
      sqlite3_reset(get_user_by_login_stmt_);
      return user_id;
    }

    sqlite3_reset(get_user_by_login_stmt_);
    throw npchat::AuthorizationFailed{npchat::AuthorizationError::InvalidCredentials};
  }

  bool logOut(std::string_view session_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    sqlite3_bind_text(delete_session_stmt_, 1, session_id.data(), session_id.size(), SQLITE_STATIC);
    bool success = sqlite3_step(delete_session_stmt_) == SQLITE_DONE;
    sqlite3_reset(delete_session_stmt_);

    std::string session_str(session_id); // Convert for map operations
    active_sessions_.erase(session_str);
    return success;
  }

  // Registration methods
  // Internal helpers that don't acquire mutex (called when mutex is already held)
  bool checkUsernameInternal(std::string_view username) {
    sqlite3_bind_text(check_username_stmt_, 1, username.data(), username.size(), SQLITE_STATIC);
    sqlite3_bind_text(check_username_stmt_, 2, username.data(), username.size(), SQLITE_STATIC);

    bool available = true;
    while (sqlite3_step(check_username_stmt_) == SQLITE_ROW) {
      if (sqlite3_column_int(check_username_stmt_, 0) > 0) {
        available = false;
        break;
      }
    }
    sqlite3_reset(check_username_stmt_);
    return available;
  }

  bool checkEmailInternal(std::string_view email) {
    sqlite3_bind_text(check_email_stmt_, 1, email.data(), email.size(), SQLITE_STATIC);
    sqlite3_bind_text(check_email_stmt_, 2, email.data(), email.size(), SQLITE_STATIC);

    bool available = true;
    while (sqlite3_step(check_email_stmt_) == SQLITE_ROW) {
      if (sqlite3_column_int(check_email_stmt_, 0) > 0) {
        available = false;
        break;
      }
    }
    sqlite3_reset(check_email_stmt_);
    return available;
  }

  bool checkUsername(std::string_view username) {
    std::lock_guard<std::mutex> lock(mutex_);
    return checkUsernameInternal(username);
  }

  bool checkEmail(std::string_view email) {
    std::lock_guard<std::mutex> lock(mutex_);
    return checkEmailInternal(email);
  }

  void registerStepOne(std::string_view username, std::string_view email, std::string_view password) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!checkUsernameInternal(username)) {
      throw npchat::RegistrationFailed{npchat::RegistrationError::UsernameAlreadyTaken};
    }

    if (!checkEmailInternal(email)) {
      throw npchat::RegistrationFailed{npchat::RegistrationError::EmailAlreadyTaken};
    }

    // Clean up expired registrations
    sqlite3_bind_int64(cleanup_expired_stmt_, 1, currentTimestamp());
    sqlite3_step(cleanup_expired_stmt_);
    sqlite3_reset(cleanup_expired_stmt_);

    std::uint32_t verification_code = generateVerificationCode();
    std::uint64_t current_time = currentTimestamp();
    std::uint64_t expires_at = current_time + (15 * 60); // 15 minutes

    sqlite3_bind_text(insert_pending_stmt_, 1, username.data(), username.size(), SQLITE_STATIC);
    sqlite3_bind_text(insert_pending_stmt_, 2, email.data(), email.size(), SQLITE_STATIC);
    auto password_hash = hashPassword(password);
    sqlite3_bind_blob(insert_pending_stmt_, 3, password_hash.c_str(), password_hash.length(), SQLITE_STATIC);
    sqlite3_bind_int(insert_pending_stmt_, 4, verification_code);
    sqlite3_bind_int64(insert_pending_stmt_, 5, current_time);
    sqlite3_bind_int64(insert_pending_stmt_, 6, expires_at);

    if (sqlite3_step(insert_pending_stmt_) != SQLITE_DONE) {
      sqlite3_reset(insert_pending_stmt_);
      throw npchat::RegistrationFailed{npchat::RegistrationError::UsernameAlreadyTaken};
    }
    sqlite3_reset(insert_pending_stmt_);

    // TODO: Send verification code via email
    spdlog::info("Verification code for {}: {}", std::string(username), verification_code);
  }

  void registerStepTwo(std::string_view username, std::uint32_t code) {
    std::lock_guard<std::mutex> lock(mutex_);

    sqlite3_bind_text(get_pending_stmt_, 1, username.data(), username.size(), SQLITE_STATIC);
    sqlite3_bind_int(get_pending_stmt_, 2, code);
    sqlite3_bind_int64(get_pending_stmt_, 3, currentTimestamp());

    if (sqlite3_step(get_pending_stmt_) == SQLITE_ROW) {
      std::string email = reinterpret_cast<const char*>(sqlite3_column_text(get_pending_stmt_, 1));
      std::string password_hash = std::string(
        reinterpret_cast<const char*>(sqlite3_column_blob(get_pending_stmt_, 2)),
        sqlite3_column_bytes(get_pending_stmt_, 2));
      sqlite3_reset(get_pending_stmt_);

      // Create the user
      std::uint64_t current_time = currentTimestamp();
      sqlite3_bind_text(insert_user_stmt_, 1, username.data(), username.size(), SQLITE_STATIC);
      sqlite3_bind_text(insert_user_stmt_, 2, email.c_str(), -1, SQLITE_STATIC);
      sqlite3_bind_blob(insert_user_stmt_, 3, password_hash.c_str(), password_hash.length(), SQLITE_STATIC);
      sqlite3_bind_int64(insert_user_stmt_, 4, current_time);

      if (sqlite3_step(insert_user_stmt_) == SQLITE_DONE) {
        sqlite3_reset(insert_user_stmt_);

        // Clean up pending registration
        sqlite3_bind_text(delete_pending_stmt_, 1, username.data(), username.size(), SQLITE_STATIC);
        sqlite3_step(delete_pending_stmt_);
        sqlite3_reset(delete_pending_stmt_);
        return;
      }
      sqlite3_reset(insert_user_stmt_);
    }

    sqlite3_reset(get_pending_stmt_);
    throw npchat::RegistrationFailed{npchat::RegistrationError::InvorrectCode};
  }

  std::uint32_t getUserIdBySession(std::string_view session_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string session_str(session_id); // Convert for map lookup
    auto it = active_sessions_.find(session_str);
    if (it != active_sessions_.end()) {
      return it->second;
    }

    sqlite3_bind_text(get_user_by_session_stmt_, 1, session_id.data(), session_id.size(), SQLITE_STATIC);
    sqlite3_bind_int64(get_user_by_session_stmt_, 2, currentTimestamp());

    if (sqlite3_step(get_user_by_session_stmt_) == SQLITE_ROW) {
      std::uint32_t user_id = sqlite3_column_int(get_user_by_session_stmt_, 0);
      sqlite3_reset(get_user_by_session_stmt_);
      active_sessions_[session_str] = user_id;
      return user_id;
    }

    sqlite3_reset(get_user_by_session_stmt_);
    throw npchat::AuthorizationFailed{npchat::AuthorizationError::AccessDenied};
  }
};
