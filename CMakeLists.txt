cmake_minimum_required(VERSION 3.16)

project(searchDBLP VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets )
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets  )


set(PROJECT_SOURCES
        main.cpp
        widget.cpp
        widget.h
        repo.h
        scan.h
        BplusTree.h
        BplusTree.cpp
        BplusTree_author.h
        BplusTree_author.cpp
        repo.cpp
        scan.cpp
        F1.h
        F1.cpp      # 新增
        F2.h
        F2.cpp      # 新增
        F3.h
        F3.cpp      # 新增
        F4.h
        F4.cpp      # 新增
        F5.h
        F5.cpp      # 新增
        F6.h
        F6.cpp      # 新增
        ResultDialog.h
        ResultDialog.cpp
        widget.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)


qt_add_executable(searchDBLP
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ResultDialog.h


    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET searchDBLP APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(searchDBLP SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(searchDBLP
            ${PROJECT_SOURCES}
        )
    endif()
endif()

# 先包含 expat 的头文件路径
target_include_directories(searchDBLP PRIVATE
    "C:/Users/18313/Desktop/数据结构/expat-2.7.5/lib"
    "C:/Program Files (x86)/igraph/include/igraph"
)

# 合并所有库链接，统一使用 PRIVATE 关键字
target_link_libraries(searchDBLP PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets

    "C:/Users/18313/Desktop/数据结构/expat-2.7.5/build/lib/libexpat.lib"  # 改成具体的 .lib 文件
    "C:/Program Files (x86)/igraph/lib/igraph.lib"
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.searchDBLP)
endif()
set_target_properties(searchDBLP PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS searchDBLP
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(searchDBLP)
endif()
