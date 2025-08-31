
set(SOCIAL_SRC ${CMAKE_BINARY_DIR}/npchat_stub/src/gen/js/npchat.ts)
set(SOCIAL_DST ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/rpc/npchat.ts)

add_custom_command(
  OUTPUT ${SOCIAL_DST}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/rpc
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SOCIAL_SRC} ${SOCIAL_DST}
  DEPENDS ${SOCIAL_SRC}
  COMMENT "Copying npchat generated TypeScript headers to ${SOCIAL_DST}"
  VERBATIM
)

# add_custom_command(
#   OUTPUT npchat_js.stamp
#   COMMAND  npm ci && npm run build
#   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#   DEPENDS ${SOCIAL_DST}
#   COMMENT "Building NPChat TypeScript client"
#   VERBATIM
# )

add_custom_target(npchat_js ALL
  DEPENDS
    # npchat_js.stamp
    npchat_stub_gen
)