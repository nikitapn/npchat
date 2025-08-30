#pragma once

#include <nprpc/nprpc.hpp>
#include <nprpc/serialization/oarchive.h>
#include <spdlog/spdlog.h>

// Simple macro system for HostJson automation

// Basic macros for object handling
#define HOST_OBJECT_FIELD(name) nprpc::ObjectId name;

#define HOST_OBJECT_SERIALIZE(name, ar) ar & NVP(name);

#define HOST_OBJECT_ACTIVATE(name, host_json, poa, obj_var, flags) \
    host_json.objects.name = poa->activate_object(obj_var.get(), flags);

#define HOST_OBJECT_LOG(name, obj_var) \
    spdlog::info("{} - poa: {}, oid: {}", #name, obj_var->poa_index(), obj_var->oid());

// Define your service objects in one place
// To add a new service, just add a line here: MACRO(service_name, ##__VA_ARGS__)
#define NPCHAT_OBJECTS(MACRO, ...) \
    MACRO(authorizator, ##__VA_ARGS__) \
    MACRO(chat, ##__VA_ARGS__) \
    MACRO(calculator, ##__VA_ARGS__)

// Helper macros for activation - these expect your variables to be named like the service
// e.g., if you have MACRO(authorizator, ...), you should have: auto authorizator = ...;
#define ACTIVATE_OBJECT(name, host_json, poa, flags) \
    HOST_OBJECT_ACTIVATE(name, host_json, poa, name, flags)

#define LOG_OBJECT(name) \
    HOST_OBJECT_LOG(name, name)

// Convenience macros for batch operations
#define ACTIVATE_ALL_NPCHAT_OBJECTS(host_json, poa, flags) \
    NPCHAT_OBJECTS(ACTIVATE_OBJECT, host_json, poa, flags)

#define LOG_ALL_NPCHAT_OBJECTS() \
    NPCHAT_OBJECTS(LOG_OBJECT)

/* 
USAGE EXAMPLE:

// 1. Define HostJson structure (add this to your header or main.cpp):
struct HostJson {
  bool secured;
  
  struct {
    NPCHAT_OBJECTS(HOST_OBJECT_FIELD)  // Generates: nprpc::ObjectId authorizator; nprpc::ObjectId chat; etc.
    
    template <typename Archive>
    void serialize(Archive& ar) {
      NPCHAT_OBJECTS(HOST_OBJECT_SERIALIZE, ar)  // Generates: ar & NVP(authorizator); ar & NVP(chat); etc.
    }
  } objects;
  
  template <typename Archive>
  void serialize(Archive& ar) {
    ar & NVP(secured);
    ar & NVP(objects);
  }
};

// 2. In your main function, after creating your service objects:
auto authorizator = injector2.create<std::shared_ptr<AuthorizatorImpl>>();
auto chat = injector2.create<std::shared_ptr<ChatImpl>>();
auto calculator = injector2.create<std::shared_ptr<CalculatorImpl>>();

HostJson host_json;
host_json.secured = use_ssl;

// 3. Activate all objects with one line:
ACTIVATE_ALL_NPCHAT_OBJECTS(host_json, poa, flags);

// 4. Log all objects with one line:
LOG_ALL_NPCHAT_OBJECTS();

// To add a new service:
// 1. Add it to NPCHAT_OBJECTS macro: MACRO(new_service, ##__VA_ARGS__)
// 2. Create the service: auto new_service = injector2.create<std::shared_ptr<NewServiceImpl>>();
// 3. The activation and logging will happen automatically!
*/
