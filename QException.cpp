#include "QException.h"

QException::QException(const std::string message):message(message)
{
}

QException::QException(const std::string source, const std::string operation,
	const std::string description):source(source),operation(operation)
{
	message = source+":: "+operation+": "+description;
}