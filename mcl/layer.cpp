
#include "layer.hpp"
#include <typeinfo>

namespace layer {
	
	const char *NotFoundException::what() const throw() {
		return message.c_str();
	}
	
	NotFoundException::~NotFoundException() throw() {}
	
	const char *BuildException::what() const throw() {
		static const char msg[] = "Program build error. (layer::BuildException)";
		return msg;
	}
	
	BuildException::~BuildException() throw() {}
	
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
	
	void Layer::build() {
		for(auto i = arguments().begin(); i!=arguments().end(); i++)
			if(bool(i->value()))
				i->value()->build();
	}
	Layer::~Layer() {}
	
	void DeviceLayer::build() {
		Layer::build();
		boost::unique_lock<boost::mutex> lk(m_build_mutex);
		if(m_build_started)
			return;
		m_build_started = true;
		std::map<std::string, mclang::ExpressionRef> exs = expressions();
		size_t expr_size = exs.size();
		for(auto i = exs.begin(); i!=exs.end(); i++) {
			std::string name = i->first;
			mclang::ExpressionRef expr = i->second;
			mcl::Program program(context().mcl_context(), expr->build());
			auto self = this;
			program.build([name, program, expr, self, expr_size]() mutable {
				self->program_ready(name, program, expr, expr_size);
			});
		}
	}
	mcl::Kernel DeviceLayer::kernel(const std::string &nm) {
		build();
		wait_for_build();
		boost::lock_guard<boost::mutex> lk(m_build_mutex);
		auto i = kernels.find(nm);
		if(i==kernels.end())
			throw NotFoundException(std::string("kernel:") + nm);
		else {
			if((bool)(i->second.second))
				return *(i->second.second);
			else
				throw BuildException(i->first, i->second.first);
		}
	}
	void DeviceLayer::reset_cache() {
		wait_for_build();
		Layer::reset_cache();
		boost::lock_guard<boost::mutex> lk(m_build_mutex);
		m_build_started = m_build_finished = false;
		kernels.clear();
	}
	
}
