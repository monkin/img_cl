
#ifndef MAY_MCLANG_HPP
#define MAY_MCLANG_HPP

#include "mcl.hpp"
#include <string>
#include <list>
#include <ostream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <memory>
#include <algorithm>
#include <iterator>
#include <array>
#include <set>
#include <assert.h>

/*
	cnst
	arg

*/

namespace mclang {
	class Type {
	private:
		int type_id;
	public:
		static const int UNSIGNED_FLAG = 0x10;
		static const int POINTER_FLAG = 0x20;
		static const int WRITE_FLAG = 0x40;
		static const int READ_FLAG = 0x80;
		static const int VECTOR_MASK = 0xFF00;
		static const int tp_void = 0;
		static const int tp_bool = 1;
		static const int tp_char = 2;
		static const int tp_uchar = tp_char | UNSIGNED_FLAG;
		static const int tp_short = 4;
		static const int tp_ushort = tp_short | UNSIGNED_FLAG;
		static const int tp_int = 5;
		static const int tp_uint = tp_int | UNSIGNED_FLAG;
		static const int tp_long = 6;
		static const int tp_ulong = tp_long | UNSIGNED_FLAG;
		static const int tp_ptrdiff_t = 7;
		static const int tp_size_t = tp_ptrdiff_t | UNSIGNED_FLAG;
		static const int tp_float = 8;
		static const int tp_image = 9 | READ_FLAG;
		static const int tp_image_r = 9 | READ_FLAG;
		static const int tp_image_w = 9 | WRITE_FLAG;
		static const int tp_sampler = 10;
		Type() : type_id(tp_void) {}
		Type(int tid) : type_id(tid) {}
		int id() const {
			return type_id;
		}
		const std::string &name() const;
		bool is_compatible() const { // true if variable of this type can be copied from host 
			switch(type_id) {
			case tp_void:
			case tp_bool:
			case tp_ptrdiff_t:
			case tp_size_t:
				return false;
			default:
				return true;
			}
		}
		bool is_pointer() const {
			return type_id & POINTER_FLAG;
		}
		Type pointer_to() const {
			assert(is_pointer());
			return type_id & ~(POINTER_FLAG | READ_FLAG | WRITE_FLAG);
		}
		bool is_vector() const {
			return !is_pointer() && (type_id & VECTOR_MASK);
		}
		bool is_scalar() const {
			return is_numeric();
		}
		size_t vector_size() const {
			assert(is_vector());
			return type_id >> 8;
		}
		Type vector_of() const {
			assert(is_vector());
			return type_id & (~VECTOR_MASK);
		}
		bool is_image() const {
			return (type_id & 0x0F) == 9;	
		}
		bool is_readable() const {
			assert(is_image());
			return type_id & READ_FLAG;
		}
		bool is_writable() const {
			assert(is_image());
			return type_id & WRITE_FLAG;
		}
		bool is_numeric() const {
			switch(type_id & (~UNSIGNED_FLAG)) {
			case tp_char:
			case tp_short:
			case tp_int:
			case tp_float:
			case tp_ptrdiff_t:
				return true;
			default:
				return false;
			}
		}
		bool is_integer() const {
			switch(type_id &(~UNSIGNED_FLAG)) {
			case tp_char:
			case tp_short:
			case tp_int:
			case tp_ptrdiff_t:
				return true;
			default:
				return false;
			}
		}
		bool is_float() const {
			return type_id==tp_float;
		}
		bool is_signed() const {
			assert(is_integer() || is_vector() || is_float());
			return !(type_id & UNSIGNED_FLAG) || is_float();
		}
		bool operator==(const Type &t) const {
			return id()==t.id();
		}
		bool operator!=(const Type &t) const {
			return id()!=t.id();
		}
		static inline Type vector(int size, Type t) {
			assert(size==2 /*|| size==3*/ || size==4 || size==8 || size==16);
			assert(t.is_numeric() && t.type_id!=tp_ptrdiff_t && t.type_id!=tp_size_t);
			return (size << 8) | t.id();
		}
		static inline Type pointer(Type t) {
			assert(t.is_numeric() || t.is_vector());
			return POINTER_FLAG | t.id();	
		}
		static inline Type to_signed(Type t) {
			assert(t.is_integer() || (t.is_vector() && t.vector_of().is_integer()));
			return t.id() & (~UNSIGNED_FLAG);
		}
		static inline Type to_unsigned(Type t) {
			assert(t.is_integer() || (t.is_vector() && t.vector_of().is_integer()));
			return t.id() | UNSIGNED_FLAG;
		}
		template<typename T>
		static int type() {
			assert(false && "Invalid type.");
		}
		static inline Type max(const Type &t1, const Type &t2) {
			assert(t1.is_numeric() || t1.is_vector() || t1==tp_bool);
			assert(t2.is_numeric() || t2.is_vector() || t2==tp_bool);
			if(t1==t2)
				return t1;
			else if(t1.is_vector() && t2.is_vector()) {
				assert(t1==t2);
				return t1;
			} else if(t1.is_vector())
				return t1;
			else if(t2.is_vector())
				return t2;
			else if(t1==Type::tp_float)
				return t1;
			else if(t2==Type::tp_float)
				return t2;
			else {
				bool s1 = (t1!=tp_bool) ? t1.is_signed() : false;
				bool s2 = (t2!=tp_bool) ? t2.is_signed() : false;
				if((s1 && s2) || (!s1 && !s2))
					return t1.type_id>t2.type_id ? t1 : t2;
				else if(s1) {
					auto t2s = t2.type_id & (~UNSIGNED_FLAG);
					return t1.type_id>t2s ? t1 : t2s;
				} else if(s2) {
					auto t1s = t1.type_id & (~UNSIGNED_FLAG);
					return t1s>t2.type_id ? t1s : t2;
				}
			}
		}
	};
	
	template<> inline int Type::type<cl_char>() { return Type::tp_char; }
	template<> inline int Type::type<cl_uchar>() { return Type::tp_uchar; }
	template<> inline int Type::type<cl_short>() { return Type::tp_short; }
	template<> inline int Type::type<cl_ushort>() { return Type::tp_ushort; }
	template<> inline int Type::type<cl_int>() { return Type::tp_int; }
	template<> inline int Type::type<cl_uint>() { return Type::tp_uint; }
	template<> inline int Type::type<cl_long>() { return Type::tp_long; }
	template<> inline int Type::type<cl_ulong>() { return Type::tp_ulong; }
	template<> inline int Type::type<cl_float>() { return Type::tp_float; }

#define MCLANG_TYPE_V(tp, sz) template<> inline int Type::type<cl_ ## tp ## sz>() { return Type::tp_ ## tp | (sz << 8); }
#define MCLANG_TYPE_V2(tp) MCLANG_TYPE_V(tp, 2); /* MCLANG_TYPE_V(tp, 3); */ MCLANG_TYPE_V(tp, 4); MCLANG_TYPE_V(tp, 8); MCLANG_TYPE_V(tp, 16)
	MCLANG_TYPE_V2(char);
	MCLANG_TYPE_V2(uchar);
	MCLANG_TYPE_V2(short);
	MCLANG_TYPE_V2(ushort);
	MCLANG_TYPE_V2(int);
	MCLANG_TYPE_V2(uint);
	MCLANG_TYPE_V2(long);
	MCLANG_TYPE_V2(ulong);
	MCLANG_TYPE_V2(float);
#undef MCLANG_TYPE_V2
#undef MCLANG_TYPE_V
	
	template<int TP>
	class HostType {};

#define MCLANG_HOST_TYPE_V(tp, sz) template<> class HostType<Type::tp_ ## tp | (((long)sz) << 8)> { typedef cl_ ## tp ## sz type; };	
#define MCLANG_HOST_TYPE(tp) template<> class HostType<Type::tp_ ## tp> { typedef cl_ ## tp type; }; MCLANG_HOST_TYPE_V(tp, 2); /*MCLANG_HOST_TYPE_V(tp, 3);*/ MCLANG_HOST_TYPE_V(tp, 4); MCLANG_HOST_TYPE_V(tp, 8); MCLANG_HOST_TYPE_V(tp, 16)
	MCLANG_HOST_TYPE(char);
	MCLANG_HOST_TYPE(uchar);
	MCLANG_HOST_TYPE(short);
	MCLANG_HOST_TYPE(ushort);
	MCLANG_HOST_TYPE(int);
	MCLANG_HOST_TYPE(uint);
	MCLANG_HOST_TYPE(long);
	MCLANG_HOST_TYPE(ulong);
	MCLANG_HOST_TYPE(float);
#undef MCLANG_HOST_TYPE
#undef MCLANG_HOST_TYPE_V
	
	class Expression;
	
	typedef std::set<const Expression *> ExpressionsSet;
	
	class Expression {
	public:
		class ValuesStream {
		private:
			mcl::Kernel kernel;
			cl_uint position;
			ExpressionsSet expessions;
		public:
			ValuesStream(const mcl::Kernel &k) : kernel(k), position(0) {}
			ValuesStream(mcl::Kernel &&k) : kernel(k), position(0) {}
			template<typename T>
			void append(const Expression *e, const T &v) {
				if(expessions.find(e)==expessions.end()) {
					expessions.insert(e);
					kernel.set_arg(position, v);
					position++;
				}
			}
		};
		class ArgumentsStream {
		public:
			typedef std::list<std::pair<Type, std::string>> items_type;
		private:
			bool first;
			items_type m_items;
			ExpressionsSet expessions;
		public:
			ArgumentsStream() : first(true) {}
			void append(const Expression *e, const std::string &nm) {
				if(expessions.find(e)==expessions.end()) {
					expessions.insert(e);
					m_items.push_back(make_pair(e->type(), nm));
				}	
			}
			const items_type &items() const {
				return m_items;
			}
		};
		virtual void global_source(std::ostream &, ExpressionsSet &) const;
		virtual void local_source(std::ostream &, ExpressionsSet &) const;
		virtual void value_source(std::ostream &) const;
		virtual void push_arguments(ArgumentsStream &) const;
		virtual void set_arguments(ValuesStream &vs) const;
		virtual bool is_lvalue() const;
		Expression() {}
		virtual std::string id() const;
		virtual Type type() const;
		bool equals(const Expression &e) const {
			return this==&e;
		}
		Expression(const Expression &) = delete;
		Expression &operator=(const Expression &e) = delete;
		void set_arguments(mcl::Kernel &k) const {
			ValuesStream vs(k);
			set_arguments(vs);
		}
		std::string build() {
			ExpressionsSet c;
			std::stringstream sout;
			global_source(sout, c);
			ArgumentsStream args;
			push_arguments(args);
			sout << "kernel void main_kernel(";
			for(auto i=args.items().begin(); i!=args.items().end(); i++) {
				if(i!=args.items().begin())
					sout << ", ";
				sout << i->first.name() << " " << i->second;
			}
			sout << ") {\n";
			local_source(sout, c);
			value_source(sout);
			sout << ";\n};\n";
			return sout.str();
		}
	};
	
	inline void format_const(std::ostream &s, cl_char c) { s << std::hex << std::showbase << c; }
	inline void format_const(std::ostream &s, cl_uchar c) { s << std::hex << std::showbase << c << "u"; }
	inline void format_const(std::ostream &s, cl_short c) { s << std::hex << std::showbase << c; }
	inline void format_const(std::ostream &s, cl_ushort c) { s << std::hex << std::showbase << c << "u"; }
	inline void format_const(std::ostream &s, cl_int c) { s << std::hex << std::showbase << c; }
	inline void format_const(std::ostream &s, cl_uint c) { s << std::hex << std::showbase << c << "u"; }
	inline void format_const(std::ostream &s, cl_long c) { s << std::hex << std::showbase << c << "l"; }
	inline void format_const(std::ostream &s, cl_ulong c) { s << std::hex << std::showbase << c << "ul"; }
	inline void format_const(std::ostream &s, cl_float c) { s << std::scientific << c << "f"; }

#define MCLANG_FORMAT_V(tp, sz) \
	inline void format_const(std::ostream &s, const cl_ ## tp ## sz &c) {  \
		bool first = true;                                                 \
		s << "(";                                                          \
		for(int i=0; i<sz; i++) {                                          \
			if(!first)                                                     \
				s << ", ";                                                 \
			first = false;                                                 \
			format_const(s, reinterpret_cast<const cl_ ## tp *>(&c)[i]);   \
		}                                                                  \
		s << ")";                                                          \
	}
#define MCLANG_FORMAT_VS(tp) MCLANG_FORMAT_V(tp, 2); /* MCLANG_FORMAT_V(tp, 3); */ MCLANG_FORMAT_V(tp, 4); MCLANG_FORMAT_V(tp, 8); MCLANG_FORMAT_V(tp, 16)
	MCLANG_FORMAT_VS(char);
	MCLANG_FORMAT_VS(uchar);
	MCLANG_FORMAT_VS(short);
	MCLANG_FORMAT_VS(ushort);
	MCLANG_FORMAT_VS(int);
	MCLANG_FORMAT_VS(uint);
	MCLANG_FORMAT_VS(long);
	MCLANG_FORMAT_VS(ulong);
	MCLANG_FORMAT_VS(float);
#undef MCLANG_FORMAT_VS
#undef MCLANG_FORMAT_V
	
	template<typename T>
	class Const : public Expression {
	private:
		static std::string name;
		const T m_value;
	public:
		void value_source(std::ostream &s) const {
			if(!name.size())
				name = Type(Type::type<T>()).name();
			format_const(s, m_value);
		}
		Const(T x) : m_value(x) {}
		Type type() const { return Type::type<T>();	 }
		const T &value() const { return m_value; } 
	};

	template<typename T>
	class Argument : public Expression {
	private:
		T m_value;
		std::string name;
	public:
		void value_source(std::ostream &s) const { s << name; }
		void push_arguments(ArgumentsStream &as) const {
			as.append(this, name);
		}
		void set_arguments(ValuesStream &vs) const {
			vs.append(this, m_value);
		}
		Argument() : name(id()) {}
		Argument(const T &v) : name(id()), m_value(v) {}
		Type type() const {
			return Type::type<T>();	
		}
		const T &value() const { return value; }
		T &value() { return m_value; }
		T &set(const T &v) {
			m_value = v;
			return m_value;
		}
	};
	
	template<typename T>
	std::string Const<T>::name;
	
	template<class T>
	class BuffArgument : public Expression {
	private:
		std::string name;
		std::shared_ptr<mcl::Buffer> m_value;
	public:
		void value_source(std::ostream &s) const { s << name; }
		void push_arguments(ArgumentsStream &as) const {
			as.append(this, name);
		}
		void set_arguments(ValuesStream &vs) const {
			vs.append(this, *m_value);
		}
		BuffArgument() : name(id()) {}
		BuffArgument(const mcl::Buffer &b) : name(id()), m_value(new mcl::Buffer(b)) {}
		mcl::Buffer &value() { return *m_value; }
		mcl::Buffer &set(const mcl::Buffer &v) {
			value = new mcl::Buffer(v);
			return *value;
		}
		Type type() const {
			return Type::pointer(Type::type<T>());	
		}
	};
	
	template<char mode='r'> /* or 'w' */
	class ImageArgument : public Expression {
	private:
		static_assert(mode=='r' || mode=='w', "Invalid image access mode ('r' or 'w').");
		std::string name;
		std::shared_ptr<mcl::Image> m_value;
	public:
		void value_source(std::ostream &s) const { s << name; }
		void push_arguments(ArgumentsStream &as) const {
			as.append(this, name);
		}
		void set_arguments(ValuesStream &vs) const {
			vs.append(this, *m_value);
		}
		ImageArgument() : name(id()) {}
		ImageArgument(const mcl::Image &b) : name(id()), m_value(new mcl::Image(b)) {}
		mcl::Image &value() { return *m_value; }
		mcl::Image &set(const mcl::Image &v) {
			m_value = new mcl::Image(v);
			return *m_value;
		}
		Type type() const {
			return mode=='r' ? Type::tp_image_r : Type::tp_image_w;	
		}
	};
	
	template<typename T>
	class ArrayConst : public Expression {
	private:
		std::vector<T> data;
		std::vector<size_t> dims;
		std::string name;
		template<class IT>
		void output_range(int pos, std::ostream &s, const IT &b, const IT &e) const {
			s << "{";
			if(dims.size()<=1 || dims.size()==pos) {
				bool first = true;
				for(IT i=b; i!=e; i++) {
					if(!first)
						s << ", ";
					first = false;
					format_const(s, *i);
				}
			} else {
				bool first = true;
				size_t incr = std::accumulate(dims.begin()+pos, dims.end(), 1, [](size_t r, size_t v) -> size_t {
						return r*v;
					});
				for(IT i=b; i!=e; i+=incr) {
					if(!first)
						s << ", ";
					first = false;
					output_range(pos+1, s, i, i+incr);
				}
			}
			s << "}";
		}
	public:
		void global_source(std::ostream &s, ExpressionsSet &es) const {
			if(es.find(this)==es.end()) {
				es.insert(this);
				s << "__local " << Type(Type::type<T>).name();
				if(dims.size()<=1) {
					s << "[] = ";	
				} else {
					for(size_t i=0; i<dims.size(); i++)
						s << "[]";
					
				}
				for(size_t i=0; i<dims.size() || i==0; i++)
					s << "[]";
				s << " = ";
				output_range(1, s, data.begin(), data.end());
				s << ";\n";
			}
		}
		void value_source(std::ostream &s) const { s << name; }
		const std::vector<T> &value() const {
			return data;	
		}
		template<typename IT>
		inline const std::vector<T> &set(const IT &b, const IT &e) {
			assert(dims.empty() || std::distance(b, e)==std::accumulate(dims.begin(), dims.end(), 1, [](size_t r, size_t v) -> size_t { return r*v; }));
			data.clear();
			data.reserve(std::distance(b, e));
			std::copy(b, e, std::back_inserter(data));
			return data;
		}
		template<typename C>
		inline const std::vector<T> &set(const C &c) {
			assert(dims.empty() || c.size()==std::accumulate(dims.begin(), dims.end(), 1, [](size_t r, size_t v) -> size_t { return r*v; }));
			data.clear();
			data.reserve(c.size());
			std::copy(c.begin(), c.end(), std::back_inserter(data));
			return data;
		}
		ArrayConst(const std::vector<size_t> d = std::vector<size_t>()) : name(id()), dims(d) {
			std::copy(d.begin(), d.end(), std::back_inserter(dims));
		}
		template<typename IT>
		ArrayConst(const IT &b, const IT &e, std::vector<size_t> d = std::vector<size_t>()) : name(id()), dims(d) {
			set(b, e);
		}
		template<typename C>
		ArrayConst(const C &c, std::vector<size_t> d = std::vector<size_t>()) : name(id()), dims(d) {
			set(c);
		}
		const std::vector<size_t> &dimensions() const {
			return dims;
		}
	};
	
	template<class T>
	class SelectBuff : public Expression {
	private:
		const std::shared_ptr<BuffArgument<T>> expr;
		const std::shared_ptr<Expression> index;
	public:
		SelectBuff(const std::shared_ptr<BuffArgument<T>> &e, const std::shared_ptr<Expression> &i) : expr(e), index(i) {
			assert(i->type().is_integer());
		}
		void global_source(std::ostream &s, ExpressionsSet &es) const {
			expr->global_source(s, es);
			index->global_source(s, es);
		}
		void local_source(std::ostream &s, ExpressionsSet &es) const {
			expr->local_source(s, es);
			index->local_source(s, es);
		}
		void value_source(std::ostream &s) const {
			s << "(";
			expr->value_source(s);
			s << "[";
			index->value_source(s);
			s << "])";
		}
		void push_arguments(ArgumentsStream &as) const {
			expr->push_arguments(as);
			index->push_arguments(as);
		}
		void set_arguments(ValuesStream &vs) const {
			expr->set_arguments(vs);
			index->set_arguments(vs);
		}
		Type type() const {
			return expr->type().pointer_to();	
		}
		bool is_lvalue() const {
			return true;	
		}
	};
	
	class SelectVector : public Expression {
	private:
		const std::shared_ptr<Expression> expr;
		unsigned index;
	public:
		SelectVector(const std::shared_ptr<Expression> &e, unsigned i) : expr(e), index(i) {
			assert(e->type().is_vector());
			assert(e->type().vector_size()>i);
		}
		void global_source(std::ostream &s, ExpressionsSet &es) const;
		void local_source(std::ostream &s, ExpressionsSet &es) const;
		void value_source(std::ostream &s) const;
		void push_arguments(ArgumentsStream &as) const;
		void set_arguments(ValuesStream &vs) const;
		Type type() const;
		bool is_lvalue() const;
	};
	
	class Sampler : public Expression {
	private:
		Sampler() {};
		static Sampler self;
	public:
		Sampler(const Sampler &) = delete;
		Sampler &operator=(const Sampler &) = delete;
		void global_source(std::ostream &, ExpressionsSet &) const;
		const Sampler &instance() const {
			return self;
		}
	};
	
	template<bool INP = true, bool NORM = false>
	class SelectImage : public Expression {
	private:
		const std::shared_ptr<ImageArgument<'r'>> img;
		const std::shared_ptr<Expression> pos;
	public:
		SelectImage(const std::shared_ptr<ImageArgument<'r'>> &i, const std::shared_ptr<Expression> &p) : img(i), pos(p) {
			assert(p->type()==Type::vector(2, Type::tp_float) || p->type()==Type::vector(2, Type::tp_int));	
		}
		void global_source(std::ostream &s, ExpressionsSet &es) const {
			img->global_source(s, es);
			pos->global_source(s, es);
		}
		void local_source(std::ostream &s, ExpressionsSet &es) const {
			img->local_source(s, es);
			pos->local_source(s, es);
		}
		void value_source(std::ostream &s) const {
			s << "read_imagef(";
			img->value_source(s);
			s << ", smp_" << (NORM ? "t_" : "f_") << (INP ? "l, ": "n, ");
			pos->value_source(s);
			s << ")";
		}
		void push_arguments(ArgumentsStream &as) const {
			img->push_arguments(as);
			pos->push_arguments(as);
		}
		void set_arguments(ValuesStream &vs) const {
			img->set_arguments(vs);
			pos->set_arguments(vs);
		}
		Type type() const {
			Type::vector(4, Type::tp_float);
		}
	};
	
	template<class T>
	class SelectArray : public Expression {
	private:
		const std::shared_ptr<ArrayConst<T>> arr;
		const std::vector<std::shared_ptr<Expression>> idx;
	public:
		SelectArray(const std::shared_ptr<ArrayConst<T>> &a, const std::vector<std::shared_ptr<Expression>> &i) : arr(a), idx(i) {
			assert((idx.size()==1 && arr.dimensions().size()==0) || idx.size()==arr.dimensions().size());
			for(auto i=idx.begin(); i!=idx.end(); i++) {
				assert((*i)->type().is_integer());
			}
		}
		void global_source(std::ostream &s, ExpressionsSet &es) const {
			arr->global_source(s, es);
			for(auto i=idx.begin(); i!=idx.end(); i++)
				(*i)->global_source(s, es);
		}
		void local_source(std::ostream &s, ExpressionsSet &es) const {
			arr->local_source(s, es);
			for(auto i=idx.begin(); i!=idx.end(); i++)
				(*i)->local_source(s, es);
		}
		void value_source(std::ostream &s) const {
			arr->value_source(s);
			for(auto i=idx.begin(); i!=idx.end(); i++) {
				s << "[";
				(*i)->value_source(s);
				s << "]";
			}
		}
		void push_arguments(ArgumentsStream &as) const {
			arr->push_arguments(as);
			for(auto i=idx.begin(); i!=idx.end(); i++)
				(*i)->push_arguments(as);
		}
		void set_arguments(ValuesStream &vs) const {
			arr->set_arguments(vs);
			for(auto i=idx.begin(); i!=idx.end(); i++)
				(*i)->set_arguments(vs);
		}
		Type type() const {
			return Type::type<T>();	
		}
	};
	
	template<const char *OP>
	class BinaryOp : public Expression {
	private:
		const std::shared_ptr<Expression> op1, op2;
	public:
		BinaryOp(const std::shared_ptr<Expression> &o1, const std::shared_ptr<Expression> &o2) : op1(o1), op2(o2) {
			auto t1 = op1->type();
			auto t2 = op2->type();
			assert(t1.is_vector() || t1.is_numeric());
			assert(t2.is_vector() || t2.is_numeric());
		}
		void global_source(std::ostream &s, ExpressionsSet &es) const {
			op1->global_source(s, es);
			op2->global_source(s, es);
		}
		void local_source(std::ostream &s, ExpressionsSet &es) const {
			op1->local_source(s, es);
			op2->local_source(s, es);
		}
		void value_source(std::ostream &s) const {
			s << "(";
			op1->value_source(s);
			s << " " << OP << " ";
			op2->value_source(s);
			s << ")";
		}
		void push_arguments(ArgumentsStream &as) const {
			op1->push_arguments(as);
			op2->push_arguments(as);
		}
		void set_arguments(ValuesStream &vs) const {
			op1->set_arguments(vs);
			op2->set_arguments(vs);
		}
		Type type() const {
			return Type::max(op1->type(), op2->type());	
		}
	};
	
	template<const char *OP>
	class UnaryOp : public Expression {
	private:
		const std::shared_ptr<Expression> op1;
	public:
		UnaryOp(const std::shared_ptr<Expression> &o1) : op1(o1) {}
		void global_source(std::ostream &s, ExpressionsSet &es) const {
			op1->global_source(s, es);
		}
		void local_source(std::ostream &s, ExpressionsSet &es) const {
			op1->local_source(s, es);
		}
		void value_source(std::ostream &s) const {
			s << "(" << OP;
			op1->value_source(s);
			s << ")";
		}
		void push_arguments(ArgumentsStream &as) const {
			op1->push_arguments(as);
		}
		void set_arguments(ValuesStream &vs) const {
			op1->set_arguments(vs);
		}
		Type type() const {
			return op1->type();	
		}
	};
	
	class TernaryOp : public Expression {
	private:
		const std::shared_ptr<Expression> op1, op2, op3;
	public:
		TernaryOp(const std::shared_ptr<Expression> &o1, const std::shared_ptr<Expression> &o2, const std::shared_ptr<Expression> &o3) : op1(o1), op2(o2), op3(o3) {}
		void global_source(std::ostream &s, ExpressionsSet &es) const;
		void local_source(std::ostream &s, ExpressionsSet &es) const;
		void value_source(std::ostream &s) const;
		void push_arguments(ArgumentsStream &as) const;
		void set_arguments(ValuesStream &vs) const;
		Type type() const;
	};
	
	class ConditionalOp : public Expression {
	private:
		const std::shared_ptr<Expression> op1, op2, op3;
	public:
		ConditionalOp(const std::shared_ptr<Expression> &o1, const std::shared_ptr<Expression> &o2, const std::shared_ptr<Expression> &o3) : op1(o1), op2(o2), op3(o3) {
			assert(op1->type().is_numeric() || op1->type()==Type::tp_bool);
		}
		void global_source(std::ostream &s, ExpressionsSet &es) const;
		void local_source(std::ostream &s, ExpressionsSet &es) const;
		void value_source(std::ostream &s) const;
		void push_arguments(ArgumentsStream &as) const;
		void set_arguments(ValuesStream &vs) const;
	};
	
	template<typename T>
	class Variable : public Expression {
	private:
		const std::shared_ptr<Expression> initializer;
		std::string name;
	public:
		Variable(const std::shared_ptr<Expression> &e) : name(id()), initializer(e) {
			assert(((bool)e) ? e->type()==Type::type<T>() : true);
		}
		void local_source(std::ostream &s, ExpressionsSet &es) const {
			s << type().name() << " " << name;
			if((bool) initializer) {
				s << " = ";
				initializer->value_source(s);
			}
			s << ";\n";
		}
		void value_source(std::ostream &s) const {
			s << name;
		}
		void push_arguments(ArgumentsStream &as) const {
			if((bool) initializer)
				initializer->push_arguments(as);
		}
		void set_arguments(ValuesStream &vs) const {
			if((bool) initializer)
				initializer->set_arguments(vs);
		}
		Type type() const {
			return Type::type<T>();
		}
		bool is_lvalue() const {
			return true;
		}
	};
	
	class Set : public Expression {
	private:
		const std::shared_ptr<Expression> e1, e2;
	public:
		Set(const std::shared_ptr<Expression> &ex1, const std::shared_ptr<Expression> &ex2) : e1(ex1), e2(ex2) {
			assert(e1->is_lvalue());
		}
		void global_source(std::ostream &s, ExpressionsSet &es) const;
		void local_source(std::ostream &s, ExpressionsSet &es) const;
		void value_source(std::ostream &s) const;
		void push_arguments(ArgumentsStream &as) const;
		void set_arguments(ValuesStream &vs) const;
		Type type() const;
	};
	
	class SetImage : public Expression {
	private:
		const std::shared_ptr<ImageArgument<'w'>> image;
		const std::shared_ptr<Expression> position;
		const std::shared_ptr<Expression> color;
	public:
		SetImage(const std::shared_ptr<ImageArgument<'w'>> &img, const std::shared_ptr<Expression> &pos, const std::shared_ptr<Expression> &clr) : image(img), position(pos), color(clr) {
			assert(position->type()==Type::vector(2, Type::tp_int));
			assert(color->type()==Type::vector(4, Type::tp_float));
		}
		void global_source(std::ostream &s, ExpressionsSet &es) const;
		void local_source(std::ostream &s, ExpressionsSet &es) const;
		void value_source(std::ostream &s) const;
		void push_arguments(ArgumentsStream &as) const;
		void set_arguments(ValuesStream &vs) const;
	};
	
	class Sequence : public Expression {
	private:
		std::vector<std::shared_ptr<Expression>> children;
	public:
		Sequence(const std::vector<std::shared_ptr<Expression>> &el) : children(el) {}
		void global_source(std::ostream &s, ExpressionsSet &es) const;
		void local_source(std::ostream &s, ExpressionsSet &es) const;
		void value_source(std::ostream &s) const;
		void push_arguments(ArgumentsStream &as) const;
		void set_arguments(ValuesStream &vs) const;
	};
	
	class ForRange : public Expression {
	private:
		const std::shared_ptr<Expression> index, begin, end, expression;
	public:
		ForRange(const std::shared_ptr<Expression> &i, const std::shared_ptr<Expression> &b, const std::shared_ptr<Expression> &e, const std::shared_ptr<Expression> &ex) : index(i), begin(b), end(e), expression(ex) {}
		void global_source(std::ostream &s, ExpressionsSet &es) const;
		void local_source(std::ostream &s, ExpressionsSet &es) const;
		void value_source(std::ostream &s) const;
		void push_arguments(ArgumentsStream &as) const;
		void set_arguments(ValuesStream &vs) const;
	};
	
	template<const char *NM, unsigned ARGC>
	class CallFunction : public Expression {
	private:
		std::array<std::shared_ptr<Expression>, ARGC> arguments;
		void test_arguments(const std::array<Type, ARGC> &tps) const {
			for(size_t i=0; i<ARGC; i++) {
				assert(tps[i]==Type::tp_void || Type(tps[i])==arguments[i]->type());
			}
		}
		Type result_type;
	public:
		CallFunction(int ret_type) : result_type(ret_type) {
			assert(ARGC==0);
		}
		CallFunction(Type ret_type, const std::array<Type, ARGC> &arg_types, const std::array<std::shared_ptr<Expression>, ARGC> &args) : result_type(ret_type), arguments(args) {
			test_arguments(arg_types);
		}
		CallFunction(Type ret_type, const std::array<Type, ARGC> &arg_types, std::array<std::shared_ptr<Expression>, ARGC> &&args) : result_type(ret_type), arguments(args) {
			test_arguments(arg_types);
		}
		void global_source(std::ostream &s, ExpressionsSet &es) const {
			if(es.find(this)==es.end()) {
				es.insert(this);
				for(auto i=arguments.begin(); i!=arguments.end(); i++)
					(*i)->global_source(s, es);
			}
		}
		void local_source(std::ostream &s, ExpressionsSet &es) const {
			if(es.find(this)==es.end()) {
				es.insert(this);
				for(auto i=arguments.begin(); i!=arguments.end(); i++)
					(*i)->local_source(s, es);
			}
		}
		void value_source(std::ostream &s) const {
			s << NM << "(";
			bool first = true;
			for(auto i=arguments.begin(); i!=arguments.end(); i++) {
				if(first)
					first = false;
				else
					s << ", ";
				(*i)->value_source(s);
			}
			s << ")";
		}
		void push_arguments(ArgumentsStream &as) const {
			for(auto i=arguments.begin(); i!=arguments.end(); i++)
				(*i)->push_arguments(as);
		}
		void set_arguments(ValuesStream &vs) const {
			for(auto i=arguments.begin(); i!=arguments.end(); i++)
				(*i)->set_arguments(vs);
		}
		Type type() const {
			return result_type;
		}
	};
	
	class Cast : public Expression {
	private:
		const std::shared_ptr<Expression> e;
		Type cast_to;
	public:
		Cast(const std::shared_ptr<Expression> &ex, Type ct) : e(ex), cast_to(ct) {}
		void global_source(std::ostream &s, ExpressionsSet &es) const;
		void local_source(std::ostream &s, ExpressionsSet &es) const;
		void value_source(std::ostream &s) const;
		void push_arguments(ArgumentsStream &as) const;
		void set_arguments(ValuesStream &vs) const;
		Type type() const;
	};
	
	
	template<typename T>
	inline std::shared_ptr<Const<T>> cnst(const T &v) {
		return std::shared_ptr<Const<T>>(new Const<T>(v));
	}
	template<typename T>
	inline std::shared_ptr<Argument<T>> arg() {
		return std::shared_ptr<Argument<T>>(new Argument<T>());
	}
	template<typename T>
	inline std::shared_ptr<Argument<T>> arg(const T &v) {
		return std::shared_ptr<Argument<T>>(new Argument<T>(v)); 	
	}
	template<typename T>
	inline std::shared_ptr<BuffArgument<T>> argv() {
		return std::shared_ptr<BuffArgument<T>>(new BuffArgument<T>());
	}
	template<typename T>
	inline std::shared_ptr<BuffArgument<T>> argv(const mcl::Buffer &b) {
		return std::shared_ptr<BuffArgument<T>>(new BuffArgument<T>(b));
	}
	template<char mode='r'>
	inline std::shared_ptr<ImageArgument<mode>> argi() {
		return std::shared_ptr<ImageArgument<mode>>(new ImageArgument<mode>());
	}
	template<char mode='r'>
	inline std::shared_ptr<ImageArgument<mode>> argi(const mcl::Image &b) {
		return std::shared_ptr<ImageArgument<mode>>(new ImageArgument<mode>(b));
	}
	inline std::shared_ptr<ImageArgument<'r'>> argi_r() { return argi<'r'>(); }
	inline std::shared_ptr<ImageArgument<'w'>> argi_w() { return argi<'w'>(); }
	inline std::shared_ptr<ImageArgument<'r'>> argi_r(const mcl::Image &i) { return argi<'r'>(i); }
	inline std::shared_ptr<ImageArgument<'w'>> argi_w(const mcl::Image &i) { return argi<'w'>(i); }
	template<typename T>
	inline std::shared_ptr<ArrayConst<T>> cnstv(std::initializer_list<size_t> dims) {
		std::vector<size_t> v;
		v.reserve(dims.size());
		std::copy(dims.begin(), dims.end(), std::back_inserter(v));
		return std::shared_ptr<ArrayConst<T>>(new ArrayConst<T>(v));
	}
	template<typename IT>
	inline auto cnstv(IT b, IT e, std::initializer_list<size_t> dims) -> std::shared_ptr<ArrayConst<decltype(*b)>> {
		std::vector<size_t> v;
		v.reserve(dims.size());
		std::copy(dims.begin(), dims.end(), std::back_inserter(v));
		return std::shared_ptr<ArrayConst<decltype(*b)>>(new ArrayConst<decltype(*b)>(b, e, v));
	}
	template<typename C>
	inline auto cnstv(const C &c, std::initializer_list<size_t> dims) -> std::shared_ptr<ArrayConst<decltype(*c.begin())>> {
		std::vector<size_t> v;
		v.reserve(dims.size());
		std::copy(dims.begin(), dims.end(), std::back_inserter(v));
		return std::shared_ptr<ArrayConst<decltype(*c.begin())>>(new ArrayConst<decltype(*c.begin())>(c, v));
	}
	template<class T>
	inline std::shared_ptr<SelectBuff<T>> select(const std::shared_ptr<BuffArgument<T>> &e, const std::shared_ptr<Expression> &i) {
		return std::shared_ptr<SelectBuff<T>>(new SelectBuff<T>(e, i));
	}
	inline std::shared_ptr<SelectVector> select(const std::shared_ptr<Expression> &e, unsigned i) {
		return 	std::shared_ptr<SelectVector>(new SelectVector(e, i));
	}
	template<bool INP=true, bool NORM=false>
	inline std::shared_ptr<SelectImage<INP, NORM>> select(const std::shared_ptr<ImageArgument<'r'>> &i, const std::shared_ptr<Expression> &p) {
		return std::shared_ptr<SelectImage<INP, NORM>>(new SelectImage<INP, NORM>(i, p));
	}
	template<class T>
	inline std::shared_ptr<SelectArray<T>> select(const std::shared_ptr<ArrayConst<T>> &a, std::initializer_list<std::shared_ptr<Expression>> i) {
		std::vector<std::shared_ptr<Expression>> idx;
		idx.reserve(i.size());
		std::copy(i.begin(), i.end(), std::back_inserter(idx));
		return std::shared_ptr<SelectArray<T>>(new SelectArray<T>(a, idx));
	}
	inline std::shared_ptr<Expression> operator+ (const std::shared_ptr<Expression> &e1, const std::shared_ptr<Expression> &e2) {
		static char op[] = "+\0";
		return std::shared_ptr<Expression>(new BinaryOp<op>(e1, e2));
	}
	inline std::shared_ptr<Expression> operator- (const std::shared_ptr<Expression> &e1, const std::shared_ptr<Expression> &e2) {
		static char op[] = "-\0";
		return std::shared_ptr<Expression>(new BinaryOp<op>(e1, e2));
	}
	inline std::shared_ptr<Expression> operator* (const std::shared_ptr<Expression> &e1, const std::shared_ptr<Expression> &e2) {
		static char op[] = "*\0";
		return std::shared_ptr<Expression>(new BinaryOp<op>(e1, e2));
	}
	inline std::shared_ptr<Expression> operator/ (const std::shared_ptr<Expression> &e1, const std::shared_ptr<Expression> &e2) {
		static char op[] = "/\0";
		return std::shared_ptr<Expression>(new BinaryOp<op>(e1, e2));
	}
	inline std::shared_ptr<Expression> operator% (const std::shared_ptr<Expression> &e1, const std::shared_ptr<Expression> &e2) {
		static char op[] = "%\0";
		return std::shared_ptr<Expression>(new BinaryOp<op>(e1, e2));
	}
	inline std::shared_ptr<Expression> operator|| (const std::shared_ptr<Expression> &e1, const std::shared_ptr<Expression> &e2) {
		static char op[] = "||\0";
		return std::shared_ptr<Expression>(new BinaryOp<op>(e1, e2));
	}
	inline std::shared_ptr<Expression> operator&& (const std::shared_ptr<Expression> &e1, const std::shared_ptr<Expression> &e2) {
		static char op[] = "&&\0";
		return std::shared_ptr<Expression>(new BinaryOp<op>(e1, e2));
	}
	inline std::shared_ptr<Expression> operator| (const std::shared_ptr<Expression> &e1, const std::shared_ptr<Expression> &e2) {
		static char op[] = "|\0";
		return std::shared_ptr<Expression>(new BinaryOp<op>(e1, e2));
	}
	inline std::shared_ptr<Expression> operator& (const std::shared_ptr<Expression> &e1, const std::shared_ptr<Expression> &e2) {
		static char op[] = "&\0";
		return std::shared_ptr<Expression>(new BinaryOp<op>(e1, e2));
	}
	inline std::shared_ptr<Expression> operator^ (const std::shared_ptr<Expression> &e1, const std::shared_ptr<Expression> &e2) {
		static char op[] = "^\0";
		return std::shared_ptr<Expression>(new BinaryOp<op>(e1, e2));
	}
	inline std::shared_ptr<Expression> equal(const std::shared_ptr<Expression> &e1, const std::shared_ptr<Expression> &e2) {
		static char op[] = "==\0";
		return std::shared_ptr<Expression>(new BinaryOp<op>(e1, e2));
	}
	inline std::shared_ptr<Expression> not_equal(const std::shared_ptr<Expression> &e1, const std::shared_ptr<Expression> &e2) {
		static char op[] = "!=\0";
		return std::shared_ptr<Expression>(new BinaryOp<op>(e1, e2));
	}
	inline std::shared_ptr<Expression> operator! (const std::shared_ptr<Expression> &e) {
		static char op[] = "!\0";
		return std::shared_ptr<Expression>(new UnaryOp<op>(e));
	}
	inline std::shared_ptr<Expression> operator~ (const std::shared_ptr<Expression> &e) {
		static char op[] = "~\0";
		return std::shared_ptr<Expression>(new UnaryOp<op>(e));
	}
	inline std::shared_ptr<Expression> operator- (const std::shared_ptr<Expression> &e) {
		static char op[] = "-\0";
		return std::shared_ptr<Expression>(new UnaryOp<op>(e));
	}
	inline std::shared_ptr<Expression> ternary(const std::shared_ptr<Expression> &e1, const std::shared_ptr<Expression> &e2, const std::shared_ptr<Expression> &e3) {
		return std::shared_ptr<TernaryOp>(new TernaryOp(e1, e2, e3));
	}
	inline std::shared_ptr<Set> set(const std::shared_ptr<Expression> &e1, const std::shared_ptr<Expression> &e2) {
		return std::shared_ptr<Set>(new Set(e1, e2));
	}
	inline std::shared_ptr<SetImage> set(const std::shared_ptr<ImageArgument<'w'>> &img, const std::shared_ptr<Expression> &pos, const std::shared_ptr<Expression> &clr) {
		return std::shared_ptr<SetImage>(new SetImage(img, pos, clr));
	}
	inline std::shared_ptr<Sequence> seq(std::initializer_list<std::shared_ptr<Expression>> ex) {
		std::vector<std::shared_ptr<Expression>> v;
		v.reserve(ex.size());
		std::copy(ex.begin(), ex.end(), std::back_inserter(v));
		return std::shared_ptr<Sequence>(new Sequence(v));
	}
	inline std::shared_ptr<Expression> cond(const std::shared_ptr<Expression> &e1, const std::shared_ptr<Expression> &e2, const std::shared_ptr<Expression> &e3 = std::shared_ptr<Expression>()) {
		return std::shared_ptr<Expression>(new ConditionalOp(e1, e2, e3));
	}
	inline std::shared_ptr<Expression> unless(const std::shared_ptr<Expression> &e1, const std::shared_ptr<Expression> &e2, const std::shared_ptr<Expression> &e3 = std::shared_ptr<Expression>()) {
		return std::shared_ptr<Expression>(new ConditionalOp(e1, e3, e2));
	}
	template<typename T>
	inline std::shared_ptr<Expression> var(const std::shared_ptr<Expression> &e = std::shared_ptr<Expression>()) {
		return std::shared_ptr<Expression>(new Variable<T>(e));
	}
	inline std::shared_ptr<Expression> for_range(const std::shared_ptr<Expression> &i, const std::shared_ptr<Expression> &b, const std::shared_ptr<Expression> &e, const std::shared_ptr<Expression> &ex) {
		return std::shared_ptr<Expression>(new ForRange(i, b, e, ex));
	}
	inline std::shared_ptr<Expression> get_work_dim() {
		static const char nm[] = "get_work_dim";
		return std::shared_ptr<Expression>(new CallFunction<nm, 0>(Type::tp_uint));
	}
	inline std::shared_ptr<Expression> cast(const std::shared_ptr<Expression> &e, Type t) {
		return std::shared_ptr<Expression>(new Cast(e, t));
	}
	template<typename T>
	inline std::shared_ptr<Expression> cast(const std::shared_ptr<Expression> &e) {
		return std::shared_ptr<Expression>(new Cast(e, Type::type<T>()));
	}
	
#define MCLANG_SIZE_FN(FN_NAME) \
	inline std::shared_ptr<Expression> FN_NAME(cl_uint d) { \
		static const char nm[] = #FN_NAME;                  \
		const std::array<Type, 1> types = { Type::tp_uint }; \
		const std::array<std::shared_ptr<Expression>, 1> args = { cnst(d) }; \
		return std::shared_ptr<Expression>(new CallFunction<nm, 1>(Type::tp_size_t, types, args)); \
	}
	MCLANG_SIZE_FN(get_global_size);
	MCLANG_SIZE_FN(get_global_id);
	MCLANG_SIZE_FN(get_local_size);
	MCLANG_SIZE_FN(get_local_id);
	MCLANG_SIZE_FN(get_group_id);
	MCLANG_SIZE_FN(get_global_offset);
#undef MCLANG_SIZE_FN
	inline std::shared_ptr<Expression> abs(const std::shared_ptr<Expression> &e) {
		Type argt = e->type();
		assert((argt.is_vector() && argt.vector_of().is_integer()) || argt.is_integer());
		static const char nm[] = "abs";
		static const std::array<Type, 1> types = { Type::tp_void };
		const std::array<std::shared_ptr<Expression>, 1> args = { e };
		Type rtype = Type::to_unsigned(argt);
		return std::shared_ptr<Expression>(new CallFunction<nm, 1>(rtype, types, args));
	}
	inline std::shared_ptr<Expression> abs_diff(const std::shared_ptr<Expression> &e1, const std::shared_ptr<Expression> &e2) {
		Type argt = Type::max(e1->type(), e2->type());
		assert((argt.is_vector() && argt.vector_of().is_integer()) || argt.is_integer());
		static const char nm[] = "abs_diff";
		static const std::array<Type, 2> types = { Type::tp_void, Type::tp_void };
		Type rtype = Type::to_unsigned(argt);
		const std::array<std::shared_ptr<Expression>, 2> args = { cast(e1, argt), cast(e2, argt) };
		return std::shared_ptr<Expression>(new CallFunction<nm, 2>(rtype, types, args));
	}
#define MCLANG_T_FN(NM, SZ, TP) \
	inline std::shared_ptr<Expression> NM ## _internal_do(std::array<std::shared_ptr<Expression>, SZ> &args) { \
		assert(args.size() == SZ);                     \
		for(auto i=args.begin(); i!=args.end(); i++) { \
			Type t = (*i)->type();                     \
			assert((t.is_vector() ? t.vector_of().is_ ## TP() : false) || t.is_ ## TP()); \
		}; \
		Type rtype = std::accumulate(args.begin(), args.end(), Type(Type::tp_void), [](Type r, const std::shared_ptr<Expression> &e) -> Type { \
				return r==Type::tp_void ? e->type() : Type::max(r, e->type()); \
			}); \
		std::array<Type, SZ> types; \
		std::fill(types.begin(), types.end(), Type(Type::tp_void));  \
		for(auto i=args.begin(); i!=args.end(); i++)            \
			*i = cast(*i, rtype); \
		static char nm[] = #NM;               \
		return std::shared_ptr<Expression>(new CallFunction<nm, SZ>(rtype, types, args)); \
	}
#define MCLANG_T_FN_1(NM, TP) MCLANG_T_FN(NM, 1, TP); \
	inline std::shared_ptr<Expression> NM(const std::shared_ptr<Expression> &e) { \
		std::array<std::shared_ptr<Expression>, 1> args = { e }; \
		return NM ## _internal_do(args); \
	}
#define MCLANG_T_FN_2(NM, TP) MCLANG_T_FN(NM, 2, TP); \
	inline std::shared_ptr<Expression> NM(const std::shared_ptr<Expression> &e1, const std::shared_ptr<Expression> &e2) { \
		std::array<std::shared_ptr<Expression>, 2> args = { e1, e2 }; \
		return NM ## _internal_do(args); \
	}
#define MCLANG_T_FN_3(NM, TP) MCLANG_T_FN(NM, 3, TP); \
	inline std::shared_ptr<Expression> NM(const std::shared_ptr<Expression> &e1, const std::shared_ptr<Expression> &e2, const std::shared_ptr<Expression> &e3) { \
		std::array<std::shared_ptr<Expression>, 3> args = { e1, e2, e3 }; \
		return NM ## _internal_do(args); \
	}

	MCLANG_T_FN_2(add_sat, integer);
	MCLANG_T_FN_2(hadd, integer);
	MCLANG_T_FN_2(rhadd, integer);
	MCLANG_T_FN_1(clz, integer);
	MCLANG_T_FN_3(mad_hi, integer);
	MCLANG_T_FN_3(mad_sat, integer);
	MCLANG_T_FN_2(rotate, integer);
	MCLANG_T_FN_2(sub_sat, integer);
	MCLANG_T_FN_3(mad24, integer);
	MCLANG_T_FN_2(mul24, integer);
	MCLANG_T_FN_3(clamp, numeric);
	MCLANG_T_FN_2(min, numeric);
	MCLANG_T_FN_2(max, numeric);
	MCLANG_T_FN_3(mix, numeric);
	MCLANG_T_FN_1(radians, numeric);
	MCLANG_T_FN_2(step, numeric);
	MCLANG_T_FN_3(smoothstep, numeric);
	MCLANG_T_FN_1(sign, numeric);
	MCLANG_T_FN_1(acos, float);
	MCLANG_T_FN_1(acosh, float);
	MCLANG_T_FN_1(acospi, float);
	MCLANG_T_FN_1(asin, float);
	MCLANG_T_FN_1(asinh, float);
	MCLANG_T_FN_1(asinpi, float);
	MCLANG_T_FN_1(atan, float);
	MCLANG_T_FN_2(atan2, float);
	MCLANG_T_FN_1(atanh, float);
	MCLANG_T_FN_1(atanpi, float);
	MCLANG_T_FN_2(atan2pi, float);
	MCLANG_T_FN_1(cbrt, float);
	MCLANG_T_FN_1(ceil, float);
	MCLANG_T_FN_2(copysign, float);
	MCLANG_T_FN_1(cos, float);
	MCLANG_T_FN_1(cosh, float);
	MCLANG_T_FN_1(cospi, float);
	MCLANG_T_FN_1(erfc, float);
	MCLANG_T_FN_1(erf, float);
	MCLANG_T_FN_1(exp, float);
	MCLANG_T_FN_1(exp2, float);
	MCLANG_T_FN_1(exp10, float);
	MCLANG_T_FN_1(expm1, float);
	MCLANG_T_FN_1(fabs, float);
	MCLANG_T_FN_2(fdim, float);
	MCLANG_T_FN_1(floor, float);
	MCLANG_T_FN_3(fma, float);
	MCLANG_T_FN_2(fmax, float);
	MCLANG_T_FN_2(fmin, float);
	MCLANG_T_FN_2(fmod, float);
	MCLANG_T_FN_2(hypot, float);
	MCLANG_T_FN_1(lgamma, float);
	MCLANG_T_FN_1(log, float);
	MCLANG_T_FN_1(log2, float);
	MCLANG_T_FN_1(log10, float);
	MCLANG_T_FN_1(log1p, float);
	MCLANG_T_FN_1(logb, float);
	MCLANG_T_FN_3(mad, float);
	MCLANG_T_FN_2(maxmag, float);
	MCLANG_T_FN_2(minmag, float);
	MCLANG_T_FN_2(nextafter, float);
	MCLANG_T_FN_2(pow, float);
	MCLANG_T_FN_2(remainder, float);
	MCLANG_T_FN_2(remquo, float);
	MCLANG_T_FN_1(rint, float);
	MCLANG_T_FN_1(round, float);
	MCLANG_T_FN_1(rsqrt, float);
	MCLANG_T_FN_1(sin, float);
	MCLANG_T_FN_1(sinh, float);
	MCLANG_T_FN_1(sinpi, float);
	MCLANG_T_FN_1(sqrt, float);
	MCLANG_T_FN_1(tan, float);
	MCLANG_T_FN_1(tanh, float);
	MCLANG_T_FN_1(tanpi, float);
	MCLANG_T_FN_1(tgamma, float);
	MCLANG_T_FN_1(trunc, float);
	MCLANG_T_FN_2(cross, float);
	MCLANG_T_FN_1(normalize, float);
	MCLANG_T_FN_1(fast_normalize, float);
#undef MCLANG_T_FN_3
#undef MCLANG_T_FN_2
#undef MCLANG_T_FN_1
#undef MCLANG_T_FN

	inline std::shared_ptr<Expression> length(const std::shared_ptr<Expression> &e) {
		Type t = e->type();
		assert(t.is_vector() ? t.vector_of().is_float() && t.vector_size()<=4 : t.is_float());
		static const char nm[] = "length";
		std::array<Type, 1> tps = { Type::tp_void };
		std::array<std::shared_ptr<Expression>, 1> args = { e };
		return std::shared_ptr<Expression>(new CallFunction<nm, 1>(Type::tp_float, tps, args)); 
	}
	inline std::shared_ptr<Expression> distance(const std::shared_ptr<Expression> &e1, const std::shared_ptr<Expression> &e2) {
		Type t = e1->type();
		assert(t==e2->type());
		assert(t.is_vector() ? t.vector_of().is_float() && t.vector_size()<=4: t.is_float());
		static const char nm[] = "distance";
		std::array<Type, 2> tps = { Type::tp_void, Type::tp_void };
		std::array<std::shared_ptr<Expression>, 2> args = { e1, e2 };
		return std::shared_ptr<Expression>(new CallFunction<nm, 2>(Type::tp_float, tps, args));
	}
	inline std::shared_ptr<Expression> dot(const std::shared_ptr<Expression> &e1, const std::shared_ptr<Expression> &e2) {
		Type t = e1->type();
		assert(t==e2->type());
		assert(t.is_vector() ? t.vector_of().is_float() && t.vector_size()<=4: t.is_float());
		static const char nm[] = "dot";
		std::array<Type, 2> tps = { Type::tp_void, Type::tp_void };
		std::array<std::shared_ptr<Expression>, 2> args = { e1, e2 };
		return std::shared_ptr<Expression>(new CallFunction<nm, 2>(Type::tp_float, tps, args));
	}
	
}

#endif // MAY_MCLANG_HPP

