cmake_minimum_required(VERSION 3.20)
project(OrderMS_Qt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

set(HEADERS
        include/Errors/CustomExceptions.h
        include/core/Order.h
        include/core/Product.h
        include/core/IRepository.h
        include/infrastructure/TxtOrderRepository.h
        include/services/OrderService.h
        include/services/ReportService.h
        include/utils/validation_utils.h
        include/ui/UtilsQt.h
        include/ui/MainWindow.h
        include/ui/ProductWindow.h
        include/ui/StatisticsWindow.h
        include/ui/AddProductDialog.h
)

set(SOURCES
        src/core/Order.cpp
        src/infrastructure/TxtOrderRepository.cpp
        src/services/OrderService.cpp
        src/services/ReportService.cpp
        src/ui/MainWindow.cpp
        src/ui/ProductWindow.cpp
        src/ui/StatisticsWindow.cpp
        src/ui/AddProductDialog.cpp
        src/main.cpp
)

set(UI_FILES
        src/ui/mainwindow.ui
        include/infrastructure/TxtProductRepository.h
        src/infrastructure/TxtProductRepository.cpp
        include/utils/SimpleList.h
        include/core/IProductRepository.h
        include/services/ProductService.h
        src/services/ProductService.cpp
        include/ui/FilterDialog.h
        src/ui/FilterDialog.cpp
        include/ui/NumericItem.h
        include/ui/AddOrderDialog.h
        src/ui/AddOrderDialog.cpp
        include/ui/EditOrderDialog.h
        src/ui/EditOrderDialog.cpp
        include/ui/ReportDialog.h
        src/ui/ReportDialog.cpp
)

qt_add_executable(app
        ${SOURCES}
        ${HEADERS}
        ${UI_FILES}
)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(app PRIVATE Qt6::Widgets)
