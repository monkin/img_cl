
#ifndef MAY_LAYER_HPP
#define MAY_LAYER_HPP


#include "mcl.hpp"
#include "mclang.hpp"
#include "CL/cl.h"
#include <iostream>
#include <exception>
#include <map>

namespace layer {
	
	class ErrorFactoryNotFound : public std::exception {
	private:
		std::string m_factory_name;
		std::string message;
	public:
		ErrorFactoryNotFound(const std::string &fn) : m_factory_name(fn), message(std::string("Layer factory \"") + fn + "\" not found.") {};
		const std::string &factory_name() const {
			return m_factory_name;
		}
		const char *what() const throw();
		~ErrorFactoryNotFound() throw();
	};
	
	class Type {
	public:
		typedef long type;
		static const type ltp_float = 1;
		static const type ltp_color = 2;
		static const type ltp_vector2d = 4;
		static const type ltp_any = ltp_float | ltp_color | ltp_vector2d;
		static inline bool compatible(type t1, type t2) {
			return t1 & t2;
		}
	};
	
	class Layer;
	class Context;
	
	class LayerFactory {
	public:
		virtual std::shared_ptr<Layer> create(Context &) = 0;
	};
	
	class Context {
	private:
		mcl::Context m_context;
		mcl::Queue m_queue;
		static std::map<std::string, LayerFactory *> m_factory;
	public:
		Context(mcl::Context &c, mcl::Device &d) : m_context(c), m_queue(c, d) {}
		mcl::Queue queue() { return m_queue; }
		mcl::Context mcl_context() { return m_context; }
		mcl::Device device() { return m_queue.device(); }
		std::shared_ptr<Layer> create(const std::string &nm) {
			auto fit = m_factory.find(nm);
			if(fit==m_factory.end())
				throw ErrorFactoryNotFound(nm);
			else
				return fit->second->create(*this);
		}
		static inline void register_factory(const std::string &nm, LayerFactory &f) {
			m_factory.insert(std::pair<std::string, LayerFactory *>(nm, &f));
		}
	};
	
	class Argument {
	private:
		Type::type m_type;
		std::shared_ptr<Layer> m_value;
		Layer *m_owner;
		std::string m_name;
	public:
		Argument(Type::type tp, std::string nm) : m_type(tp), m_name(nm) {}
		const std::string &name() const {
			return m_name;	
		}
		Type::type type() const {
			return m_type;
		}
		const std::shared_ptr<Layer> value() const {
			return m_value;	
		}
		void set_value(std::shared_ptr<Layer> v);
		const Layer &owner() const {
			return *m_owner;
		}
		Layer &owner() {
			return *m_owner;
		}
		void set_owner(Layer *w) {
			m_owner = w;	
		}
	};
	
	class Layer {
	private:
		std::vector<Argument> m_arguments;
		std::map<std::string, std::vector<Argument>::iterator> p_arguments;
		Context m_context;
		mclang::ExpressionRef m_position;
		long long m_version;
	protected:
		mclang::ExpressionRef position() const {
			return m_position;
		}
	public:
		Layer(Context &c, const std::vector<Argument> &args) : m_arguments(args), m_context(c), m_version(1) {
			for(auto i = m_arguments.begin(); i!=m_arguments.end(); i++) {
				i->set_owner(this);
				p_arguments.insert(std::pair<std::string, std::vector<Argument>::iterator>(i->name(), i));
			}
		}
		Context &context() { return m_context; }
		virtual std::string class_name() const;
		const std::vector<Argument> &arguments() const { return m_arguments; }
		std::vector<Argument> &arguments() { return m_arguments; }
		const Argument &argument(const std::string &name) const {
			return *(p_arguments.find(name)->second);
		}
		Argument &argument(const std::string &name) { return *p_arguments[name]; }
		void set_position(const mclang::ExpressionRef &e) { m_position = e;	}
		virtual void reset_cache() = 0;
		virtual mclang::ExpressionRef compute(size_t) = 0;
		long long version() const { return m_version; }
		void inc_version() { m_version++; }
		void set_version(long long v) { m_version = v; }
	};
	
	template<typename T>
	class LayerFactoryRegistrar {
	protected:
		class FactoryInstance : public LayerFactory {
		public:
			virtual std::shared_ptr<Layer> create(Context &);
		};
		static FactoryInstance m_factory;
	public:
		LayerFactoryRegistrar(const std::string &name) {
			Context::register_factory(name, m_factory);
		}
	};
	
	template<typename T>
	typename LayerFactoryRegistrar<T>::FactoryInstance LayerFactoryRegistrar<T>::m_factory;
	
	template<typename T>
	std::shared_ptr<Layer> LayerFactoryRegistrar<T>::FactoryInstance::create(Context &c) {
		return std::shared_ptr<Layer>(new T(c));
	}
	
	
	class HostLayer : public Layer {};
	class DeviceLayer : public Layer {};
}


#endif // MAY_LAYER_HPP
