#pragma once
#include <stdexcept>
#include <string>

struct CustomException : std::runtime_error { using std::runtime_error::runtime_error; };
struct ValidationException : CustomException { using CustomException::CustomException; };
struct NotFoundException : CustomException { using CustomException::CustomException; };
struct IoException : CustomException { using CustomException::CustomException; };
