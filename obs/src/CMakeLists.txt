# greenscreen-plugin/CMakeLists.txt

project(greenscreen-plugin)

set(greenscreen-plugin_SOURCES
      greenscreen-plugin.cpp
      greenscreen-source.cpp)

add_library(greenscreen-plugin MODULE
      ${greenscreen-plugin_SOURCES})

target_link_libraries(greenscreen-plugin PUBLIC
      libobs)

target_link_libraries(greenscreen-plugin PRIVATE
      ${CMAKE_BINARY_DIR}/libAigsEffectApp.a)

target_include_directories(greenscreen-plugin PRIVATE
	    ${CMAKE_SOURCE_DIR}/nvidia/samples/AigsEffectApp)

install_obs_plugin_with_data(greenscreen-plugin data)