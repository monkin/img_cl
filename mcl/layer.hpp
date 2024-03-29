
#ifndef MAY_LAYER_HPP
#define MAY_LAYER_HPP


#include "mcl.hpp"
#include "mclang.hpp"
#include "CL/cl.h"
#include <iostream>
#include <exception>
#include <map>
#include <boost/thread.hpp>

namespace layer {
	
	class NotFoundException : public std::exception {
	private:
		std::string m_name;
		std::string message;
	public:
		NotFoundException(const std::string &fn) : m_name(fn), message(std::string("Layer factory \"") + fn + "\" not found.") {};
		const std::string &name() const {
			return m_name;
		}
		const char *what() const throw();
		~NotFoundException() throw();
	};
	
	class BuildException : public std::exception {
	private:
		const mcl::Program m_program;
		const std::string m_expression_name;
	public:
		BuildException(const std::string &exp_name, const mcl::Program &p) : m_program(p), m_expression_name(exp_name) {}
		const mcl::Program program() const { return m_program; }
		const std::string expression_name() const { return m_expression_name; }
		const char *what() const throw();
		~BuildException() throw();
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
				throw NotFoundException(std::string("layer:") + nm);
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
		const Layer *owner() const {
			return m_owner;
		}
		Layer *owner() {
			return m_owner;
		}
		void set_owner(Layer *);
	};
	
	class Layer {
	private:
		std::vector<Argument> m_arguments;
		std::map<std::string, std::vector<Argument>::iterator> p_arguments;
		Context m_context;
		mclang::ExpressionRef m_position;
		long long m_version;
		Layer *m_parent;
	protected:
		mclang::ExpressionRef position() const {
			return m_position;
		}
	public:
		Layer(Context &c, const std::vector<Argument> &args) : m_arguments(args), m_context(c), m_version(1), m_parent(0) {
			for(auto i = m_arguments.begin(); i!=m_arguments.end(); i++) {
				i->set_owner(this);
				p_arguments.insert(std::pair<std::string, std::vector<Argument>::iterator>(i->name(), i));
			}
		}
		
		virtual void build();
		virtual void reset_cache();
		
		Context &context() { return m_context; }
		virtual std::string class_name() const;
		const std::vector<Argument> &arguments() const { return m_arguments; }
		std::vector<Argument> &arguments() { return m_arguments; }
		const Argument &argument(const std::string &name) const {
			return *(p_arguments.find(name)->second);
		}
		Argument &argument(const std::string &name) { return *p_arguments[name]; }
		void set_position(const mclang::ExpressionRef &e) { reset_cache(); m_position = e;	}
		
		Layer *parent() { return m_parent; }
		const Layer *parent() const { return m_parent; }
		void set_parent(Layer *p) { m_parent = p; }
		
		virtual mclang::ExpressionRef compute(size_t) = 0;
		
		// version
		long long version() const { return m_version; }
		void inc_version() { m_version++; }
		void set_version(long long v) { m_version = v; }
		
		virtual ~Layer();
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

	class HostLayer : public Layer {
	
	};
	
	class DeviceLayer : public Layer {
	private:
		std::map<std::string, std::pair<mcl::Program, std::shared_ptr<mcl::Kernel>>> kernels;
		volatile bool m_build_started;
		volatile bool m_build_finished;
		boost::mutex m_build_mutex;
		boost::condition_variable m_finish_cond;
		void wait_for_build() {
			boost::unique_lock<boost::mutex> lk(m_build_mutex);
			if(m_build_started && !m_build_finished)
				m_finish_cond.wait(lk, [&m_build_finished]{ return m_build_finished; });
		}
		void program_ready(const std::string &name, mcl::Program &program, mclang::ExpressionRef expr, size_t sz) {
			boost::lock_guard<boost::mutex> lk(m_build_mutex);
			if(program.build_status(context().device())!=CL_BUILD_SUCCESS)
				kernels.insert(std::make_pair(name, std::make_pair(program, std::shared_ptr<mcl::Kernel>(new mcl::Kernel(program.kernel("main_kernel"))))));
			else
				kernels.insert(std::make_pair(name, std::make_pair(program, std::shared_ptr<mcl::Kernel>())));
			if(kernels.size()==sz) {
				m_build_finished = true;
				m_finish_cond.notify_all();
			}
		}
	protected:
		mcl::Kernel kernel(const std::string &);
	public:
		virtual std::map<std::string, mclang::ExpressionRef> expressions() = 0;
		void build();
		virtual void reset_cache();
	};
}


#endif // MAY_LAYER_HPP
