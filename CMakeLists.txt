cmake_minimum_required(VERSION 3.5)

project(RepoMan VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core5Compat Xml Concurrent)

set(PROJECT_SOURCES
        src/resources.qrc
        src/global.cpp src/global.h
        src/main.cpp
        src/busystatedisabler.h src/busystatedisabler.cpp
        src/mainwindow.h src/mainwindow.cpp
        src/reftreemodel.h src/reftreemodel.cpp
        src/reftreedelegate.h src/reftreedelegate.cpp
        src/dialogs/cmddialog.h src/dialogs/cmddialog.cpp
        src/dialogs/reposyncdialog.h src/dialogs/reposyncdialog.cpp
        src/dialogs/settingsdialog.h src/dialogs/settingsdialog.cpp
        src/dialogs/pushdialog.h src/dialogs/pushdialog.cpp
        src/dialogs/resetdialog.h src/dialogs/resetdialog.cpp
        src/dialogs/branchdialog.h src/dialogs/branchdialog.cpp
        src/dialogs/checkoutdialog.h src/dialogs/checkoutdialog.cpp
        src/dialogs/fetchdialog.h src/dialogs/fetchdialog.cpp
        src/dialogs/pulldialog.h src/dialogs/pulldialog.cpp
        src/dialogs/cleandialog.h src/dialogs/cleandialog.cpp
        src/dialogs/deleterefdialog.h src/dialogs/deleterefdialog.cpp
        src/dialogs/warningfilelistdialog.h src/dialogs/warningfilelistdialog.cpp
        src/pages/changespage.h src/pages/changespage.cpp
        src/pages/historypage.h src/pages/historypage.cpp
        src/pages/pagehost.h src/pages/pagehost.cpp
        src/pages/historytablemodel.h src/pages/historytablemodel.cpp
        src/pages/historygraphdelegate.h src/pages/historygraphdelegate.cpp
        src/pages/newtabpage.h src/pages/newtabpage.cpp
        src/widgets/QProgressIndicator.h src/widgets/QProgressIndicator.cpp
        src/widgets/qhistorytableview.h src/widgets/qhistorytableview.cpp
        src/widgets/diffscrollarea.h src/widgets/diffscrollarea.cpp
        src/widgets/commitdetailscrollarea.h src/widgets/commitdetailscrollarea.cpp
        src/widgets/reftreeview.h src/widgets/reftreeview.cpp
        src/widgets/tabwidgetex.h src/widgets/tabwidgetex.cpp
)

set(PTY_SOURCES
        src/pty/kprocess.cpp src/pty/kprocess.h
        src/pty/kptydevice.cpp src/pty/kptydevice.h
        src/pty/kptyprocess.cpp src/pty/kptyprocess.h
        src/pty/Pty.cpp src/pty/Pty.h
        src/pty/kpty.cpp

        src/pty/ptydisplay.h src/pty/ptydisplay.cpp
        src/pty/kshell.h src/pty/kshell_unix.cpp
)

qt_add_executable(RepoMan
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
    ${PTY_SOURCES}
   )

target_include_directories(RepoMan
    PRIVATE
        src
)

target_link_libraries(RepoMan PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core5Compat
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Concurrent
)

target_compile_definitions(RepoMan
    PRIVATE
        "HAVE_POSIX_OPENPT"
        "HAVE_SYS_TIME_H"
)

install(TARGETS RepoMan
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})


qt_finalize_executable(RepoMan)



