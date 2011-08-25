
#include "layer.hpp"
#include <typeinfo>

namespace layer {
	
	const char *ErrorFactoryNotFound::what() const throw() {
		return message.c_str();
	}
	
	ErrorFactoryNotFound::~ErrorFactoryNotFound() throw() {}
	
	const Type::type Type::ltp_float;
	const Type::type Type::ltp_color;
	const Type::type Type::ltp_vector2d;
	const Type::type Type::ltp_any;

	std::map<std::string, LayerFactory *> Context::m_factory;
	
	void Argument::set_value(std::shared_ptr<Layer> v) {
		m_value = v;
		m_owner->inc_version();
		m_owner->reset_cache();
	}
	
	std::string Layer::class_name() const {
		return typeid(*this).name();	
	}
	
}
