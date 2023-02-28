cmake_minimum_required(VERSION 3.14.0)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13 CACHE STRING "")

set(vst3sdk_SOURCE_DIR "C:/VST_make/VST_SDK/vst3sdk")
if(NOT vst3sdk_SOURCE_DIR)
    message(FATAL_ERROR "Path to VST3 SDK is empty!")
endif()

project(Airwindows_VariMu
    # This is your plug-in version number. Change it here only.
    # Version number symbols usable in C++ can be found in
    # source/version.h and ${PROJECT_BINARY_DIR}/projectversion.h.
    VERSION 1.0.0.0 
    DESCRIPTION "Airwindows_VariMu VST 3 Plug-in"
)

set(SMTG_VSTGUI_ROOT "${vst3sdk_SOURCE_DIR}")

add_subdirectory(${vst3sdk_SOURCE_DIR} ${PROJECT_BINARY_DIR}/vst3sdk)
smtg_enable_vst3_sdk()

smtg_add_vst3plugin(Airwindows_VariMu
    source/version.h
    source/VariMucids.h
    source/VariMuprocessor.h
    source/VariMuprocessor.cpp
    source/VariMuparam.h
    source/VariMucontroller.h
    source/VariMucontroller.cpp
    source/VariMuentry.cpp
    ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/vst2wrapper/vst2wrapper.sdk.cpp
    resource/VariMueditor.uidesc
    resource/VariMueditor.rc
)

#- VSTGUI Wanted ----
if(SMTG_ADD_VSTGUI)
    target_sources(Airwindows_VariMu
        PRIVATE
            resource/VariMueditor.uidesc
            resource/VariMueditor.rc
    )
    target_link_libraries(Airwindows_VariMu
        PRIVATE
            vstgui_support
    )
    smtg_target_add_plugin_resources(Airwindows_VariMu
        RESOURCES
            "resource/VariMueditor.uidesc"
            "resource/VariMueditor.rc"
    )
endif(SMTG_ADD_VSTGUI)
# -------------------

smtg_target_add_plugin_snapshots (Airwindows_VariMu
    RESOURCES
        resource/B9E26604B5CF562A9C6CF9F1CF0116D5_snapshot.png
        resource/B9E26604B5CF562A9C6CF9F1CF0116D5_snapshot_2.0x.png
)

target_link_libraries(Airwindows_VariMu
    PRIVATE
        sdk
)

smtg_target_configure_version_file(Airwindows_VariMu)

if(SMTG_MAC)
    smtg_target_set_bundle(Airwindows_VariMu
        BUNDLE_IDENTIFIER io.github.varimu
        COMPANY_NAME "yg331"
    )
    smtg_target_set_debug_executable(Airwindows_VariMu
        "/Applications/VST3PluginTestHost.app"
        "--pluginfolder;$(BUILT_PRODUCTS_DIR)"
    )
elseif(SMTG_WIN)
    target_sources(Airwindows_VariMu PRIVATE 
        resource/win32resource.rc
        resource/VariMueditor.rc
        resource/VariMueditor.uidesc
    )
    if(MSVC)
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Airwindows_VariMu)

        smtg_target_set_debug_executable(Airwindows_VariMu
            "$(ProgramW6432)/Steinberg/VST3PluginTestHost/VST3PluginTestHost.exe"
            "--pluginfolder \"$(OutDir)/\""
        )
    endif()
endif(SMTG_MAC)

if(SMTG_WIN)
    if(NOT MINGW)
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    endif(NOT MINGW)
endif(SMTG_WIN)