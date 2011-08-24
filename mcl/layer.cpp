
#include "layer.hpp"

namespace layer {
	
	const char *ErrorFactoryNotFound::what() const throw() {
		return message.c_str();
	}

	std::map<std::string, LayerFactory *> Factory::m_factory;
	
}
