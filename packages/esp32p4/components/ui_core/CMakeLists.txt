set(UI_XML "${CMAKE_CURRENT_LIST_DIR}/../../ui_xml")
set(UI_REPO "${CMAKE_CURRENT_LIST_DIR}/../../../..")
set(UI_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${UI_GENERATED}")
set(UI_SOURCES ui_core.cpp declarative_ui.cpp ui_bindings.cpp)
if(COMMAND idf_component_register)
  idf_component_register(SRCS ${UI_SOURCES} INCLUDE_DIRS "include" PRIV_INCLUDE_DIRS "${UI_GENERATED}" REQUIRES lvgl)
  set(UI_TARGET ${COMPONENT_LIB})
  idf_component_get_property(LVGL_TARGET lvgl__lvgl COMPONENT_LIB)
  if(TARGET ${LVGL_TARGET})
    target_compile_options(${LVGL_TARGET} PRIVATE -Wno-error=type-limits)
  endif()
else()
  list(APPEND UI_SOURCES "${UI_GENERATED}/font_asset.cpp" "${UI_GENERATED}/ionicons_assets.cpp")
  add_library(ui_core STATIC ${UI_SOURCES})
  target_include_directories(ui_core PUBLIC include PRIVATE "${UI_GENERATED}")
  target_link_libraries(ui_core PUBLIC lvgl)
  set(UI_TARGET ui_core)
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
file(GLOB_RECURSE UI_XML_FILES CONFIGURE_DEPENDS "${UI_XML}/*.xml")
file(GLOB UI_ICON_FILES CONFIGURE_DEPENDS "${UI_REPO}/assets/ionicons5-weather/*.svg")
if(COMMAND idf_component_register)
  add_custom_command(OUTPUT "${UI_GENERATED}/ui_scene.inc"
    COMMAND ${Python3_EXECUTABLE} "${UI_REPO}/packages/ui-compiler/compile_ui.py"
            "${UI_XML}" "${UI_GENERATED}"
    DEPENDS ${UI_XML_FILES} "${UI_REPO}/packages/ui-compiler/compile_ui.py"
    VERBATIM)
else()
  add_custom_command(OUTPUT "${UI_GENERATED}/font_asset.cpp" "${UI_GENERATED}/ui_scene.inc" "${UI_GENERATED}/ionicons_assets.cpp" "${UI_GENERATED}/ionicons_assets.hpp"
    COMMAND ${Python3_EXECUTABLE} "${UI_REPO}/packages/ui-compiler/compile_ui.py"
            "${UI_XML}" "${UI_GENERATED}"
            --font "${UI_REPO}/assets/HYWenHei-65W-3.ttf"
            --assets "${UI_REPO}/assets/ionicons5-weather"
    DEPENDS ${UI_XML_FILES} "${UI_REPO}/packages/ui-compiler/compile_ui.py" "${UI_REPO}/assets/HYWenHei-65W-3.ttf" ${UI_ICON_FILES}
    VERBATIM)
endif()
add_custom_target(ui_scene DEPENDS "${UI_GENERATED}/ui_scene.inc")
add_dependencies(${UI_TARGET} ui_scene)
