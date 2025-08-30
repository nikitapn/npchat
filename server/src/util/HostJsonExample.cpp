// Example of how to use the HostJson automation macros
// This file shows the modernized approach to HostJson generation

#include "util/HostJsonMacroSimple.hpp"
#include <nprpc/nprpc.hpp>
#include <nprpc/serialization/oarchive.h>
#include <fstream>

// Define your objects in one place - just add/remove lines here
#define NPCHAT_OBJECTS(MACRO, ...) \
    MACRO(authorizator, ##__VA_ARGS__) \
    MACRO(chat, ##__VA_ARGS__) \
    MACRO(calculator, ##__VA_ARGS__) \
    MACRO(user_service, ##__VA_ARGS__) \
    MACRO(message_service, ##__VA_ARGS__)

// Generate the HostJson structure automatically
struct HostJson {
  bool secured;
  
  struct {
    // This single line generates all object fields:
    // nprpc::ObjectId authorizator;
    // nprpc::ObjectId chat;
    // nprpc::ObjectId calculator;
    // nprpc::ObjectId user_service;
    // nprpc::ObjectId message_service;
    NPCHAT_OBJECTS(HOST_OBJECT_FIELD)
    
    template <typename Archive>
    void serialize(Archive& ar) {
      // This generates all serialization code:
      // ar & NVP(authorizator);
      // ar & NVP(chat);
      // ar & NVP(calculator);
      // ar & NVP(user_service);
      // ar & NVP(message_service);
      NPCHAT_OBJECTS(HOST_OBJECT_SERIALIZE, ar)
    }
  } objects;
  
  template <typename Archive>
  void serialize(Archive& ar) {
    ar & NVP(secured);
    ar & NVP(objects);
  }
};

// Example usage in main function
void example_main() {
  // Your existing object creation code...
  // auto authorizator = injector2.create<std::shared_ptr<AuthorizatorImpl>>();
  // auto chat = injector2.create<std::shared_ptr<ChatImpl>>();
  // auto calculator = injector2.create<std::shared_ptr<CalculatorImpl>>();
  // auto user_service = injector2.create<std::shared_ptr<UserServiceImpl>>();
  // auto message_service = injector2.create<std::shared_ptr<MessageServiceImpl>>();
  
  HostJson host_json;
  // auto poa = ...;
  // const auto flags = ...;
  
  host_json.secured = true; // or false
  
  // This single line replaces all individual activation calls:
  // host_json.objects.authorizator = poa->activate_object(authorizator.get(), flags);
  // host_json.objects.chat = poa->activate_object(chat.get(), flags);
  // host_json.objects.calculator = poa->activate_object(calculator.get(), flags);
  // host_json.objects.user_service = poa->activate_object(user_service.get(), flags);
  // host_json.objects.message_service = poa->activate_object(message_service.get(), flags);
  // NPCHAT_OBJECTS(HOST_OBJECT_ACTIVATE, host_json, poa, flags)
  
  // This single line replaces all individual logging calls:
  // spdlog::info("authorizator - poa: {}, oid: {}", authorizator->poa_index(), authorizator->oid());
  // spdlog::info("chat - poa: {}, oid: {}", chat->poa_index(), chat->oid());
  // spdlog::info("calculator - poa: {}, oid: {}", calculator->poa_index(), calculator->oid());
  // spdlog::info("user_service - poa: {}, oid: {}", user_service->poa_index(), user_service->oid());
  // spdlog::info("message_service - poa: {}, oid: {}", message_service->poa_index(), message_service->oid());
  // NPCHAT_OBJECTS(HOST_OBJECT_LOG)
  
  // Save to file
  // std::ofstream os("host.json");
  // nprpc::serialization::json_oarchive oa(os);
  // oa << host_json;
}

// Benefits of this approach:
// 1. Single point of definition - add/remove objects in NPCHAT_OBJECTS macro only
// 2. No repetitive code - structure, serialization, activation, and logging all automated
// 3. Type safe - compiler will catch mismatches
// 4. Maintainable - easy to add new objects or modify existing ones
// 5. DRY principle - Don't Repeat Yourself
