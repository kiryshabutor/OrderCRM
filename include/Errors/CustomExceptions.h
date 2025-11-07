#pragma once
#include <stdexcept>
#include <string>

class CustomException : public std::runtime_error { public: using std::runtime_error::runtime_error; };
class ValidationException : public CustomException { public: using CustomException::CustomException; };
class NotFoundException : public CustomException { public: using CustomException::CustomException; };
class IoException : public CustomException { public: using CustomException::CustomException; };
