
#include "layer.hpp"
#include <typeinfo>

namespace layer {
	
	const char *ErrorNotFound::what() const throw() {
		return message.c_str();
	}
	
	ErrorNotFound::~ErrorNotFound() throw() {}
	
	const char *ErrorKernelBuild::what() const throw() {
		static const char msg[] = "Program build error. (layer::ErrorKernelBuild)";
		return msg;
	}
	ErrorKernelBuild::~ErrorKernelBuild() throw() {}
	
	const Type::type Type::ltp_float;
	const Type::type Type::ltp_color;
	const Type::type Type::ltp_vector2d;
	const Type::type Type::ltp_any;

	std::map<std::string, LayerFactory *> Context::m_factory;
	
	void Argument::set_value(std::shared_ptr<Layer> v) {
		m_value = v;
		v->set_parent(m_owner);
		m_owner->inc_version();
		m_owner->reset_cache();
	}
	
	void Argument::set_owner(Layer *l) {
		if((bool)m_value)
			m_value->set_parent(l);
		m_owner = l;	
	}
	
	std::string Layer::class_name() const {
		return typeid(*this).name();	
	}
	
	void Layer::reset_cache() {
		if(m_parent)
			m_parent->reset_cache();
	}
	
	void Layer::build() {}
	Layer::~Layer() {}
	
	void DeviceLayer::build() {
		
	}
	mcl::Kernel DeviceLayer::kernel(const std::string &nm) {
		std::unique_lock<std::mutex> lk(m_build_mutex);
		if(!m_build_started)
			build();
		m_finish_cond.wait(lk, [&m_build_finished]{ return m_build_finished; });
		auto i = kernels.find(nm);
		if(i==kernels.end())
			throw ErrorNotFound(std::string("kernel:") + nm);
		else {
			if((bool)i->second) {
				
			} else {
				
			}
		}
	}
	
}
