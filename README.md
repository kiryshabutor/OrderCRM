# Order Management System (OrderMS)

> Система управления заказами, разработанная с использованием **Qt6** и **C++23**.

[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=kiryshabutor_OrderCRM&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=kiryshabutor_OrderCRM)
[![C++](https://img.shields.io/badge/STD-C%2B%2B23-blue.svg)](https://isocpp.org/)
[![Qt](https://img.shields.io/badge/Qt-6.0%2B-green.svg)](https://www.qt.io/)

---

## Описание проекта

**OrderMS** — десктопное приложение для автоматизации управления заказами. Проект реализует принципы **Clean Architecture**, обеспечивая масштабируемость и простоту поддержки.

### Функциональные возможности

*   **Учет товаров**: Добавление, редактирование и контроль складских остатков.
*   **Управление заказами**: Оформление заказов, расчет стоимости и отслеживание статусов.
*   **Аналитика**: Статистика продаж и отчетность по выручке.
*   **Поиск и фильтрация**: Гибкая система фильтрации данных по клиенту, дате и другим параметрам.
*   **Экспорт**: Генерация отчетов в формате CSV.

---

## Технологический стек

*   **Язык**: C++23
*   **Фреймворк**: Qt6 Widgets
*   **Сборка**: CMake 3.20+
*   **Архитектура**: Layered (Core -> Services -> UI)

---

## Установка и запуск

### 1. Клонирование репозитория
```bash
git clone https://github.com/kiryshabutor/OrderCRM.git
cd OrderCRM
```

### 2. Сборка проекта
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 3. Запуск
```bash
./app
```

> *Примечание: При первом запуске приложение автоматически инициализирует структуру директорий для базы данных и отчетов.*

---

## Качество кода

Проект использует статический анализатор **SonarCloud** для контроля качества кода.

[Перейти к отчету SonarCloud](https://sonarcloud.io/project/overview?id=kiryshabutor_OrderCRM)

---
Разработано в рамках курсового проекта БГУИР.
