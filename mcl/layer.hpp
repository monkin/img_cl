
#ifndef MAY_LAYER_HPP
#define MAY_LAYER_HPP


#include "mcl.hpp"
#include "mclang.hpp"
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
	}; 
	
	namespace type {
		Bool
		Float
		Color
	}
	
	class Layer;
	
	class LayerFactory {
	public:
		virtual std::shared_ptr<Layer> create(mcl::Context &) = 0;
	};
	
	class Layer {
	private:
		static std::map<std::string, LayerFactory *> m_factory; 
	public:
		Layer(mcl::Context &c) {}
		virtual const std::vector<std::string> &args() = 0;
		virtual const std::shared_ptr<mclang::Expression> get_arg(const std::string &) = 0;
		virtual void set_arg(const std::string &, const std::shared_ptr<mclang::Expression> &) = 0;
		virtual void set_position(const std::shared_ptr<mclang::Expression> &, size_t) = 0;
		virtual void set_position(const mcl::Buffer &) = 0;
		virtual void set_position(const std::vector<cl_float2> &) = 0;
		virtual std::shared_ptr<mclang::Expression> compute(mcl::Queue &, size_t work_size) = 0;
		static inline std::shared_ptr<Layer> create(const std::string &nm, mcl::Context &c) {
			auto fit = m_factory.find(nm);
			if(fit==m_factory.end())
				throw ErrorFactoryNotFound(nm);
			else
				return fit->second->create(c);
		}
		static inline void register_factory(const std::string &nm, const LayerFactory &f) {
			m_factory.insert(std::pair<std::string, LayerFactory *>(nm, &f));
		}
	};
	
	template<typename T>
	class LayerFactoryRegistrar {
	protected:
		class FactoryInstance : public LayerFactory {
		public:
			virtual std::shared_ptr<Layer> create(mcl::Context &);
		}
		static FactoryInstance m_factory;
	public:
		LayerFactoryRegistrar(const std::string &name) {
			Layer::register_factory(name, m_factory);
		}
	};
	
	template<typename T>
	LayerFactoryRegistrar<T>::FactoryInstance LayerFactoryRegistrar<T>::m_factory;
	
	template<typename T>
	std::shared_ptr<Layer> LayerFactoryRegistrar<T>::FactoryInstance::create(mcl::Context &c) {
		return std::shared_ptr<Layer>(new T(c));
	}
	
	
	class HostLayer : public Layer {}
	class DeviceLayer : public Layer {}
}


#endif // MAY_LAYER_HPP
