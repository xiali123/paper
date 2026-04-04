# ============================================================================
# 业务模块构建配置
# ============================================================================()

# 辅助函数：编译业务模块为动态库
function(build_business_module module_name source_file)
    add_library(${module_name} MODULE ${source_file})
    
    set_target_properties(${module_name} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release/lib/modules"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release/lib/modules"
        PREFIX "lib"
    )
    
    target_include_directories(${module_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/../core/include
        ${CMAKE_SOURCE_DIR}/../core/external/curl-8.19.0_4-win64-mingw/include
        ${CMAKE_SOURCE_DIR}/../core/external/spdlog/include
    )
    
    if(WIN32)
        target_compile_definitions(${module_name} PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            _CRT_SECURE_NO_WARNINGS
            CURL_STATIC
        )
    endif()
    
    message(STATUS "  [Module] ${module_name} configured")
endfunction()

# 业务模块列表
set(AVAILABLE_MODULES
    "AuthApiModule:src/business/AuthApiModule.cpp"
    "UserApiModule:src/business/UserApiModule.cpp"
    "SearchApiModule:src/business/SearchApiModule.cpp"
    "ExportApiModule:src/business/ExportApiModule.cpp"
    "AiApiModule:src/business/AiApiModule.cpp"
    "RecommendationApiModule:src/business/RecommendationApiModule.cpp"
)

# 编译模块
set(MODULE_COUNT 0)
foreach(module_spec ${AVAILABLE_MODULES})
    string(REPLACE ":" ";" module_list ${module_spec})
    list(GET module_list 0 module_name)
    list(GET module_list 1 module_source)
    
    if(EXISTS "${CMAKE_SOURCE_DIR}/${module_source}")
        build_business_module(${module_name} ${module_source})
        math(EXPR MODULE_COUNT "${MODULE_COUNT} + 1")
    endif()
endforeach()

message(STATUS "Total business modules: ${MODULE_COUNT}")
