#include <QApplication>
#include <QCoreApplication>
#include "include/infrastructure/TxtOrderRepository.h"
#include "include/infrastructure/TxtProductRepository.h"
#include "include/services/OrderService.h"
#include "include/services/ProductService.h"
#include "include/ui/MainWindow.h"
#include <filesystem>
#include <fstream>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    std::filesystem::path appDir = std::filesystem::path(QCoreApplication::applicationDirPath().toStdString());

    std::filesystem::path dbDir       = appDir / "db";
    std::filesystem::path ordersPath  = dbDir / "orders.txt";
    std::filesystem::path productsPath= dbDir / "products.txt";
    std::filesystem::path reportsDir  = appDir / "reports";

    std::error_code ec;
    std::filesystem::create_directories(dbDir, ec);
    std::filesystem::create_directories(reportsDir, ec);

    if (!std::filesystem::exists(ordersPath))   { std::ofstream(ordersPath.string()).close(); }
    if (!std::filesystem::exists(productsPath)) { std::ofstream(productsPath.string()).close(); }

    TxtOrderRepository orderRepo(ordersPath.string());
    TxtProductRepository productRepo(productsPath.string());

    ProductService productSvc(productRepo);
    try { productSvc.load(); } catch (...) {}

    OrderService orderSvc(orderRepo);
    orderSvc.setPrices(productSvc.all());
    try { orderSvc.load(); } catch (...) {}

    MainWindow w(orderSvc, productSvc);
    w.show();

    return app.exec();
}
