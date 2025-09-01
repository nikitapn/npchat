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

  static std::string generateSessionId();
  static std::string hashPassword(std::string_view password);
  static std::uint32_t generateVerificationCode();
  static std::uint64_t currentTimestamp();

public:
  explicit AuthService(const std::shared_ptr<Database>& database);
  ~AuthService();

  // Authentication methods
  npchat::UserData logIn(std::string_view login, std::string_view password);
  npchat::UserData logInWithSessionId(std::string_view session_id);
  std::uint32_t getUserIdFromSession(std::string_view session_id);
  std::uint32_t getUserIdFromLogin(std::string_view login);
  bool logOut(std::string_view session_id);

  // Registration methods
  bool checkUsername(std::string_view username);
  bool checkEmail(std::string_view email);
  void registerStepOne(std::string_view username, std::string_view email, std::string_view password);
  void registerStepTwo(std::string_view username, std::uint32_t code);

private:
  // Internal helpers that don't acquire mutex (called when mutex is already held)
  bool checkUsernameInternal(std::string_view username);
  bool checkEmailInternal(std::string_view email);
};
