cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)
project(fontweightissue VERSION 1.0)

set(QT_MIN_VERSION "5.2.0")

find_package(ECM 5.42.0  NO_MODULE)
if (ECM_FOUND)
    set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_SOURCE_DIR}/cmake)

    include(KDEInstallDirs)
    include(KDECMakeSettings)
    include(KDECompilerSettings NO_POLICY_SCOPE)
endif()

include(FeatureSummary)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Gui
    Widgets
)

set(CMAKE_AUTOMOC ON)

if (NOT Qt5::GuiPrivate)
    include_directories(
        ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
    )
endif()
if (NOT Qt5::CorePrivate)
    include_directories(
        ${Qt5Core_PRIVATE_INCLUDE_DIRS}
    )
endif()

include_directories(kwidgetsaddons)

set(FWI_SRCS
    dialog.cpp
    main.cpp
    kwidgetsaddons/kfontchooser.cpp
    kwidgetsaddons/kfontchooserdialog.cpp
    kwidgetsaddons/kfontrequester.cpp
    kwidgetsaddons/fonthelpers.cpp)
add_executable(fontweightissue WIN32 MACOSX_BUNDLE
  ${FWI_SRCS})

target_link_libraries(fontweightissue Qt5::Core Qt5::Gui Qt5::Widgets)
if (Qt5::CorePrivate)
    target_link_libraries(fontweightissue
        PRIVATE
            Qt5::CorePrivate
    )
endif()
if (Qt5::GuiPrivate)
    target_link_libraries(fontweightissue
        PRIVATE
            Qt5::GuiPrivate
    )
endif()


feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
