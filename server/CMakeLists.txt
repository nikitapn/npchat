
find_package(SQLite3 REQUIRED)

include_directories(
  ${npchat_stub_INCLUDE_DIR}
  src
)

add_executable(npchat
  ${npchat_stub_GENERATED_HEADERS}
  ${npchat_stub_GENERATED_SOURCES}
  src/main.cpp
)

target_link_libraries(npchat PRIVATE
  pthread
  crypto
  nprpc
  boost_program_options
  boost_serialization
  SQLite::SQLite3
  spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>
)

# Ensure that the IDL files are generated before building the executable
add_dependencies(npchat npchat_stub_gen)
