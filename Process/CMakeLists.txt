
function(add_plugin PluginName PluginFile)

    set(files)
    set(libs)
    set(islib FALSE)

    option(BUILD_${PluginName} "Build Plugin ${PluginName}" ON)

    math(EXPR lastIndex "${ARGC}-1")
    foreach(index RANGE 1 ${lastIndex})
        if ("${ARGV${index}}" STREQUAL  "LIBS")
            set(islib TRUE)
        elseif (${islib})
            list(APPEND libs ${ARGV${index}})
        else()
            list(APPEND files ${ARGV${index}})
        endif()
    endforeach()
    list(APPEND files "PhenoLinkPluginCore/PhenoLinkPluginCore.h")

    include_directories("${PROJECT_BINARY_DIR}/${ONNX_ROOTDIR}/include" "${PROJECT_BINARY_DIR}/${ONNX_ROOTDIR}/include/onnxruntime/core/session")
    link_directories("${PROJECT_BINARY_DIR}/${ONNX_ROOTDIR}/lib")

    if (BUILD_${PluginName})

        add_library(${PluginName} SHARED ${files})
        message("Starting Git command: ${OPENSOURCE_PLUGIN_PATH} Process/${PluginFile}")

        execute_process(
            COMMAND           "${GIT_EXECUTABLE}"
            log
            -1
            --pretty=format:"%h %ai"
            "Process/${PluginFile}"
            WORKING_DIRECTORY "${OPENSOURCE_PLUGIN_PATH}"
            RESULT_VARIABLE   git_return
            OUTPUT_VARIABLE   hash)


        message("Computing hash for ${PluginFile}: ${hash}")
        string(STRIP "${hash}" hash)

        target_compile_definitions(${PluginName} PUBLIC  -DGitHash=${hash})# -DGitPluginDate=${plugin_date})
        if (MSVC)
            target_compile_options(${PluginName} PRIVATE /WX)
        endif()

        add_dependencies (${PluginName}  PhenoLinkCore)



        set_target_properties(${PluginName}
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins)

        target_link_libraries(${PluginName} ${libs} PhenoLinkCore Qt::Gui Qt::Core Qt::Concurrent  ${OpenCV_LIBS} )
        install(TARGETS ${PluginName} COMPONENT PluginProcess DESTINATION bin/plugins)

    endif()

endfunction()


link_directories(${CMAKE_CURRENT_BINARY_DIR}/bin/plugins/)
add_definitions(-DExportPluginCoreSymbols)



add_plugin(ZStackProjection ZStackProjection.cpp LIBS)



