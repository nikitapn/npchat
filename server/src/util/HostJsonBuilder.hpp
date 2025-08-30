#pragma once

#include <nprpc/nprpc.hpp>
#include <nprpc/serialization/oarchive.h>
#include <memory>
#include <tuple>
#include <string_view>

namespace nprpc::util {

// Template-based HostJson generator
template<typename... ObjectTypes>
class HostJsonBuilder {
private:
  std::tuple<std::shared_ptr<ObjectTypes>...> objects_;
  std::tuple<std::string_view...> object_names_;
  bool secured_ = false;

public:
  // Constructor takes object names as template arguments
  template<std::size_t... Names>
  explicit HostJsonBuilder(bool secured, std::string_view (&names)[sizeof...(ObjectTypes)])
    : secured_(secured) {
    static_assert(sizeof...(ObjectTypes) == sizeof...(Names), "Number of objects must match number of names");
    object_names_ = std::make_tuple(names...);
  }

  // Set objects
  template<std::size_t Index>
  void set_object(std::shared_ptr<std::tuple_element_t<Index, std::tuple<ObjectTypes...>>> obj) {
    std::get<Index>(objects_) = obj;
  }

  // Generate and activate all objects
  std::string generate_host_json(nprpc::Poa* poa, uint32_t flags) {
    std::ostringstream oss;
    nprpc::serialization::json_oarchive oa(oss);
    
    // Create the JSON structure
    oa.start_object();
    oa.key("secured");
    oa.value(secured_);
    oa.key("objects");
    oa.start_object();
    
    // Activate and serialize each object
    activate_objects<0>(oa, poa, flags);
    
    oa.end_object();
    oa.end_object();
    
    return oss.str();
  }

private:
  template<std::size_t Index>
  void activate_objects(nprpc::serialization::json_oarchive& oa, nprpc::Poa* poa, uint32_t flags) {
    if constexpr (Index < sizeof...(ObjectTypes)) {
      auto& obj = std::get<Index>(objects_);
      auto& name = std::get<Index>(object_names_);
      
      if (obj) {
        auto oid = poa->activate_object(obj.get(), flags);
        oa.key(name);
        oa << oid;
      }
      
      activate_objects<Index + 1>(oa, poa, flags);
    }
  }
};

// Factory function for easier usage
template<typename... ObjectTypes>
auto make_host_json_builder(bool secured, std::array<std::string_view, sizeof...(ObjectTypes)> names) {
  return HostJsonBuilder<ObjectTypes...>(secured, names);
}

// Simplified macro for common usage patterns
#define NPRPC_HOST_JSON_BUILDER(secured, ...) \
  nprpc::util::make_host_json_builder<__VA_ARGS__>(secured, {#__VA_ARGS__})

} // namespace nprpc::util
