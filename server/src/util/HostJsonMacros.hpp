#pragma once

#include <nprpc/nprpc.hpp>
#include <nprpc/serialization/oarchive.h>

// Helper macros for variadic argument processing
#define FOR_EACH(macro, ...) FOR_EACH_IMPL(macro, __VA_ARGS__)
#define FOR_EACH_IMPL(macro, x, ...) macro(x) __VA_OPT__(FOR_EACH_IMPL(macro, __VA_ARGS__))

#define FOR_EACH_PARAM(macro, param, ...) FOR_EACH_PARAM_IMPL(macro, param, __VA_ARGS__)
#define FOR_EACH_PARAM_IMPL(macro, param, x, ...) macro(param, x) __VA_OPT__(FOR_EACH_PARAM_IMPL(macro, param, __VA_ARGS__))

// Macro to declare an object in HostJson structure
#define DECLARE_HOST_OBJECT(name) nprpc::ObjectId name;

// Macro to generate serialization for an object
#define SERIALIZE_HOST_OBJECT(ar, name) ar & NVP(name);

// Macro to activate and register an object
#define ACTIVATE_HOST_OBJECT(host_json, poa, name, flags) \
    host_json.objects.name = poa->activate_object(name.get(), flags); \
    spdlog::info(#name " - poa: {}, oid: {}", name->poa_index(), name->oid());

#define ACTIVATE_HOST_OBJECT_PTR(host_json, poa, name, flags) \
    host_json.objects.name = poa->activate_object(name, flags); \
    spdlog::info(#name " - poa: {}, oid: {}", name->poa_index(), name->oid());

#define ACTIVATE_HOST_OBJECT_REF(host_json, poa, name, flags) \
    host_json.objects.name = poa->activate_object(&name, flags); \
    spdlog::info(#name " - poa: {}, oid: {}", name->poa_index(), name->oid());

// Macro to generate the complete HostJson structure with multiple objects
#define DEFINE_HOST_JSON_STRUCT(...) \
struct HostJson { \
  bool secured; \
  \
  struct { \
    FOR_EACH(DECLARE_HOST_OBJECT, __VA_ARGS__) \
    \
    template <typename Archive> \
    void serialize(Archive& ar) { \
      FOR_EACH_PARAM(SERIALIZE_HOST_OBJECT, ar, __VA_ARGS__) \
    } \
  } objects; \
  \
  template <typename Archive> \
  void serialize(Archive& ar) { \
    ar & NVP(secured); \
    ar & NVP(objects); \
  } \
};

#define SAVE_HOST_JSON_TO_FILE(host_json, http_dir) do { \
    std::ofstream os(fs::path(http_dir) / "host.json"); \
    nprpc::serialization::json_oarchive oa(os); \
    oa << host_json; \
  } while(0)
