#include "include/infrastructure/TxtOrderRepository.h"
#include <fstream>
#include <iomanip>
#include "include/Errors/CustomExceptions.h"

void TxtOrderRepository::save(const std::vector<Order>& data) {
    std::ofstream o(file_);
    if (!o) throw IoException("cannot open file for write: " + file_);
    o.setf(std::ios::fixed);
    o << std::setprecision(2);
    for (const auto& e : data) o << e.toLine() << '\n';
}

std::vector<Order> TxtOrderRepository::load() {
    std::vector<Order> v;
    std::ifstream i(file_);
    if (!i) return v;
    std::string L;
    while (std::getline(i, L)) {
        if (auto oo = Order::fromLine(L)) v.push_back(*oo);
    }
    return v;
}
