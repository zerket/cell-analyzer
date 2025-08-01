cmake_minimum_required(VERSION 3.16)
project(CellAnalyzer LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)  # Автоматическая генерация MOC для Qt
set(CMAKE_AUTORCC ON)  # Автоматическая обработка ресурсов (.qrc)
set(CMAKE_CXX_STANDARD 17)

# Пути к библиотекам
set(OpenCV_DIR "D:/opencv/build")  # Путь к директории OpenCV с CMake-конфигурацией

# Поиск Qt6
find_package(Qt6 COMPONENTS Widgets Gui REQUIRED)

# Поиск OpenCV с конкретными модулями
find_package(OpenCV REQUIRED
    COMPONENTS
        core
        imgproc
        highgui
        objdetect
)

# Добавление исполняемого файла
add_executable(${PROJECT_NAME}
    main.cpp
    mainwindow.h
    mainwindow.cpp
    previewgrid.h
    previewgrid.cpp
    verificationwidget.h
    verificationwidget.cpp
    imageprocessor.h
    imageprocessor.cpp
    cellitem.h
    cellitem.cpp
    cell.h
    utils.cpp
    utils.h
    cellitemwidget.h
    cellitemwidget.cpp
    parametertuningwidget.h
    parametertuningwidget.cpp
    settingsmanager.h
    settingsmanager.cpp
    logger.h
)

# Подключение библиотек
target_link_libraries(${PROJECT_NAME}
    Qt6::Widgets
    Qt6::Gui
    ${OpenCV_LIBS}  # Подключаем указанные модули OpenCV
)

# Копирование DLL OpenCV в директорию сборки (только для Windows)
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${OpenCV_DIR}/bin"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()
