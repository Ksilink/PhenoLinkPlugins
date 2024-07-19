
include_directories("${CMAKE_SOURCE_DIR}/Core")



add_library(PhenoLinkYokogawaLoader SHARED
                                yokogawaloader.cpp yokogawaloader.h)

target_compile_definitions(PhenoLinkYokogawaLoader PUBLIC  -DGitPluginDate=${plugin_date})


set_target_properties(PhenoLinkYokogawaLoader
                PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins)

add_dependencies (PhenoLinkYokogawaLoader  PhenoLinkCore)
target_link_libraries(PhenoLinkYokogawaLoader  PhenoLinkCore Qt::Gui Qt::Core Qt::Xml Qt::Concurrent )

install(TARGETS PhenoLinkYokogawaLoader COMPONENT PluginLoader DESTINATION bin/plugins)



add_library(PhenolinkInCellLoader SHARED
                                incellloader.cpp incellloader.h)

target_compile_definitions(PhenolinkInCellLoader PUBLIC  -DGitPluginDate=${plugin_date})
set_target_properties(PhenolinkInCellLoader
                PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins)


add_dependencies (PhenolinkInCellLoader  PhenoLinkCore)
target_link_libraries(PhenolinkInCellLoader  PhenoLinkCore Qt::Gui Qt::Core Qt::Xml Qt::Concurrent )


install(TARGETS PhenolinkInCellLoader COMPONENT PluginLoader DESTINATION bin/plugins)

add_library(PhenolinkPerkinElmerLoader SHARED
                                perkinelmerloader.cpp perkinelmerloader.h)

target_compile_definitions(PhenolinkPerkinElmerLoader PUBLIC  -DGitPluginDate=${plugin_date})
add_dependencies (PhenolinkPerkinElmerLoader  PhenoLinkCore)

set_target_properties(PhenolinkPerkinElmerLoader
                PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins)

add_dependencies (PhenolinkPerkinElmerLoader  PhenoLinkCore)
target_link_libraries(PhenolinkPerkinElmerLoader  PhenoLinkCore Qt::Gui Qt::Core Qt::Xml Qt::Concurrent )

install(TARGETS PhenolinkPerkinElmerLoader COMPONENT PluginLoader DESTINATION bin/plugins)



add_library(PhenoLinkArrayScan SHARED
                                PhenoLinkArrayScan.cpp PhenoLinkArrayScan.h)

target_compile_definitions(PhenoLinkArrayScan PUBLIC  -DGitPluginDate=${plugin_date})


set_target_properties(PhenoLinkArrayScan
                PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins)

add_dependencies (PhenoLinkArrayScan  PhenoLinkCore)
target_link_libraries(PhenoLinkArrayScan  PhenoLinkCore Qt::Gui Qt::Core Qt::Xml Qt::Concurrent )

install(TARGETS PhenoLinkArrayScan COMPONENT PluginLoader DESTINATION bin/plugins)

######################

add_library(PhenoLinkMetadataGenericLoader SHARED
                                PhenoLinkMetadataGenericLoader.cpp PhenoLinkMetadataGenericLoader.h)

target_compile_definitions(PhenoLinkMetadataGenericLoader PUBLIC  -DGitPluginDate=${plugin_date})


set_target_properties(PhenoLinkMetadataGenericLoader
                PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins)

add_dependencies (PhenoLinkMetadataGenericLoader  PhenoLinkCore)
target_link_libraries(PhenoLinkMetadataGenericLoader  PhenoLinkCore Qt::Gui Qt::Core Qt::Xml Qt::Concurrent )

install(TARGETS PhenoLinkMetadataGenericLoader COMPONENT PluginLoader DESTINATION bin/plugins)


################ CZI Carl Zeiss Imaging Support


if (BUILD_CARL_ZEISS_IMAGING_SUPPORT)

add_library(PhenoLinkCarlZeissImagingLoader SHARED
                                CarlZeissImaging.cpp CarlZeissImaging.h)

target_compile_definitions(PhenoLinkCarlZeissImagingLoader PUBLIC  -DGitPluginDate=${plugin_date})


set_target_properties(PhenoLinkCarlZeissImagingLoader
                PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins)

add_dependencies (PhenoLinkCarlZeissImagingLoader  PhenoLinkCore)
target_link_libraries(PhenoLinkCarlZeissImagingLoader  PhenoLinkCore ${CZI_LIB} Qt::Gui Qt::Core Qt::Xml Qt::Concurrent  )

install(TARGETS PhenoLinkCarlZeissImagingLoader COMPONENT PluginLoader DESTINATION bin/plugins)

endif(BUILD_CARL_ZEISS_IMAGING_SUPPORT)

