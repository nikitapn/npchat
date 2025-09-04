
set(SOCIAL_SRC ${CMAKE_BINARY_DIR}/npchat_stub/src/gen/js/npchat.ts)
set(SOCIAL_DST ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/rpc/npchat.ts)

# Define build output paths
set(CLIENT_DIST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dist)
set(CLIENT_BUILD_STAMP ${CMAKE_CURRENT_BINARY_DIR}/npchat_js.stamp)
set(DEPS_STAMP ${CMAKE_CURRENT_BINARY_DIR}/deps.stamp)

# Package dependencies
set(PACKAGE_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/package.json"
  "${CMAKE_CURRENT_SOURCE_DIR}/package-lock.json"
  "${CMAKE_CURRENT_SOURCE_DIR}/vite.config.ts"
  "${CMAKE_CURRENT_SOURCE_DIR}/svelte.config.js"
  "${CMAKE_CURRENT_SOURCE_DIR}/tsconfig.json"
  # "${CMAKE_CURRENT_SOURCE_DIR}/tsconfig.app.json"
)

# Collect all source files for proper dependency tracking
file(GLOB_RECURSE CLIENT_SOURCE_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.ts"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.svelte"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.js"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.css"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.html"
)

add_custom_command(
  OUTPUT ${SOCIAL_DST}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/rpc
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SOCIAL_SRC} ${SOCIAL_DST}
  # touch the destination file to update its timestamp
  COMMAND ${CMAKE_COMMAND} -E touch ${SOCIAL_DST}
  DEPENDS ${SOCIAL_SRC}
  COMMENT "Copying npchat generated TypeScript headers to ${SOCIAL_DST}"
  VERBATIM
)

# Dependencies installation step
add_custom_command(
  OUTPUT ${DEPS_STAMP}
  COMMAND npm ci
  COMMAND ${CMAKE_COMMAND} -E touch ${DEPS_STAMP}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${PACKAGE_FILES}
  COMMENT "Installing NPChat TypeScript dependencies"
  VERBATIM
)

# Build step with better dependency tracking
add_custom_command(
  OUTPUT ${CLIENT_BUILD_STAMP}
  COMMAND npm run build
  # Only touch tsconfig when we actually rebuild
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_SOURCE_DIR}/tsconfig.app.json
  COMMAND ${CMAKE_COMMAND} -E touch ${CLIENT_BUILD_STAMP}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS
    ${SOCIAL_DST}
    ${DEPS_STAMP}
    ${CLIENT_SOURCE_FILES}
  COMMENT "Building NPChat TypeScript client"
  VERBATIM
)

add_custom_target(npchat_js ALL
  DEPENDS
    ${CLIENT_BUILD_STAMP}
    npchat_stub_gen
)

# Help target to generate stubs and copy them to the client source directory
add_custom_target(gen_stubs ALL
  DEPENDS
    npchat_stub_gen
    ${SOCIAL_DST}
)
