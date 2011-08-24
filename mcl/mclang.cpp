
#include "mclang.hpp"
#include <map>
#include <sstream>

namespace mclang {
	const int Type::UNSIGNED_FLAG;
	const int Type::POINTER_FLAG;
	const int Type::VECTOR_MASK;
	const int Type::tp_void;
	const int Type::tp_bool;
	const int Type::tp_char;
	const int Type::tp_uchar;
	const int Type::tp_short;
	const int Type::tp_ushort;
	const int Type::tp_int;
	const int Type::tp_uint;
	const int Type::tp_long;
	const int Type::tp_ulong;
	const int Type::tp_ptrdiff_t;
	const int Type::tp_size_t;
	const int Type::tp_float;
	const int Type::tp_image;
	const int Type::tp_image_r;
	const int Type::tp_image_w;
	const int Type::tp_sampler;
	
	static std::map<int, std::string> names = {{Type::tp_void, "void"},
		{Type::tp_bool, "bool"},
		{Type::tp_char, "char"},
		{Type::tp_uchar, "uchar"},
		{Type::tp_short, "short"},
		{Type::tp_ushort, "ushort"},
		{Type::tp_int, "int"},
		{Type::tp_uint, "uint"},
		{Type::tp_long, "long"},
		{Type::tp_ulong, "ulong"},
		{Type::tp_ptrdiff_t, "ptrdiff_t"},
		{Type::tp_size_t, "size_t"},
		{Type::tp_float, "float"},
		{Type::tp_image, "read_only image2d_t"},
		{Type::tp_image_w, "write_only image2d_t"},
		{Type::tp_sampler, "sampler_t"}};
	
	const std::string &Type::name() const {
		auto i = names.find(type_id);
		if(i!=names.end())
			return i->second;
		else if(is_vector()) {
			std::stringstream s;
			s << vector_of().name() << vector_size();
			names.insert(std::pair<int, std::string>(type_id, s.str()));
			return names[type_id];
		} else if(is_pointer()) {
			std::stringstream s;
			s << "__global " << pointer_to().name() << " *";
			names.insert(std::pair<int, std::string>(type_id, s.str()));
			return names[type_id];
		} else {
			assert(!"Invalid type.");
		}
	}
	
	std::string Expression::id() const {
		std::stringstream ss;
		const Expression *t = this;
		ss << "e" << std::hex << *(reinterpret_cast<size_t *>(&t));
		return ss.str();
	}
	
	bool Expression::is_lvalue() const {
		return false;
	}
	Type Expression::type() const {
		return Type::tp_void;
	}
	
	void Expression::global_source(std::ostream &, ExpressionsSet &) const {};
	void Expression::local_source(std::ostream &, ExpressionsSet &) const {};
	void Expression::value_source(std::ostream &) const {};
	void Expression::push_arguments(ArgumentsStream &) const {};
	void Expression::set_arguments(ValuesStream &vs) const {};
	void Expression::~Expression() {};
	
	Type SelectVector::type() const {
		return expr->type().vector_of();	
	}
	void SelectVector::global_source(std::ostream &s, ExpressionsSet &es) const {
		expr->global_source(s, es);
	}
	void SelectVector::local_source(std::ostream &s, ExpressionsSet &es) const {
		expr->local_source(s, es);
	}
	void SelectVector::value_source(std::ostream &s) const {
		static auto idx = "0123456789abcdef";
		char i[] = "x\0";
		i[0] = idx[index];
		expr->value_source(s);
		s << ".s" << i;
	}
	void SelectVector::push_arguments(ArgumentsStream &as) const {
		expr->push_arguments(as);
	}
	void SelectVector::set_arguments(ValuesStream &vs) const {
		expr->set_arguments(vs);
	}
	bool SelectVector::is_lvalue() const {
		return expr->is_lvalue();
	}
	
	Sampler Sampler::self;
	void Sampler::global_source(std::ostream &s, ExpressionsSet &es) const {
		if(es.find(&self)==es.end()) {
			es.insert(&self);
			s << "\nconst sampler_t smp_f_n CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;\n"
					"const sampler_t smp_f_l CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_LENEAR;\n"
					"const sampler_t smp_t_n CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;\n"
					"const sampler_t smp_t_l CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP | CLK_FILTER_LENEAR;\n";
		}
	}
	
	void TernaryOp::global_source(std::ostream &s, ExpressionsSet &es) const {
		op1->global_source(s, es);
		op2->global_source(s, es);
		op3->global_source(s, es);
	}
	void TernaryOp::local_source(std::ostream &s, ExpressionsSet &es) const {
		op1->local_source(s, es);
		op2->local_source(s, es);
		op3->local_source(s, es);
	}
	void TernaryOp::value_source(std::ostream &s) const {
		s << "(";
		op1->value_source(s);
		s << " ? ";
		op2->value_source(s);
		s << " : ";
		op3->value_source(s);
		s << ")";
	}
	void TernaryOp::push_arguments(ArgumentsStream &as) const {
		op1->push_arguments(as);
		op2->push_arguments(as);
		op3->push_arguments(as);
	}
	void TernaryOp::set_arguments(ValuesStream &vs) const {
		op1->set_arguments(vs);
		op2->set_arguments(vs);
		op3->set_arguments(vs);
	}
	Type TernaryOp::type() const {
		return Type::max(op2->type(), op3->type());	
	}
	
	
	void ConditionalOp::global_source(std::ostream &s, ExpressionsSet &es) const {
		op1->global_source(s, es);
		if((bool)op2)
			op2->global_source(s, es);
		if((bool)op3)
			op3->global_source(s, es);
	}
	void ConditionalOp::local_source(std::ostream &s, ExpressionsSet &es) const {
		if(es.find(this)==es.end()) {
			es.insert(this);
			op1->local_source(s, es);
			if((bool)op2)
				op2->local_source(s, es);
			if((bool)op3)
				op3->local_source(s, es);
		}
	}
	void ConditionalOp::value_source(std::ostream &s) const {
		if((bool)op2) {
			s << "if(";
			op1->value_source(s);
			s << ") {\n";
			op2->value_source(s);
			if((bool)op3) {
				s << ";\n} else {\n";
				op3->value_source(s);
				s << ";\n};";
			} else
				s << ";\n};";
		} else if((bool)op3) {
			s << "if(!";
			op1->value_source(s);
			s << ") {\n";
			op2->value_source(s);
			s << ";\n}\n";
		}
	}
	void ConditionalOp::push_arguments(ArgumentsStream &as) const {
		op1->push_arguments(as);
		if((bool)op2)
			op2->push_arguments(as);
		if((bool)op3)
			op3->push_arguments(as);
	}
	void ConditionalOp::set_arguments(ValuesStream &vs) const {
		op1->set_arguments(vs);
		if((bool)op2)
			op2->set_arguments(vs);
		if((bool)op3)
			op3->set_arguments(vs);
	}
	
	
	void Set::global_source(std::ostream &s, ExpressionsSet &es) const {
		e1->global_source(s, es);
		e2->global_source(s, es);
	}
	void Set::local_source(std::ostream &s, ExpressionsSet &es) const {
		e1->local_source(s, es);
		e2->local_source(s, es);
	}
	void Set::value_source(std::ostream &s) const {
		e1->value_source(s);
		s << " = ";
		e2->value_source(s);
	}
	void Set::push_arguments(ArgumentsStream &as) const {
		e1->push_arguments(as);
		e2->push_arguments(as);
	}
	void Set::set_arguments(ValuesStream &vs) const {
		e1->set_arguments(vs);
		e2->set_arguments(vs);
	}
	Type Set::type() const {
		return e1->type();	
	}
	
	void SetImage::global_source(std::ostream &s, ExpressionsSet &es) const {
		image->global_source(s, es);
		position->global_source(s, es);
		color->global_source(s, es);
	}
	void SetImage::local_source(std::ostream &s, ExpressionsSet &es) const {
		image->local_source(s, es);
		position->local_source(s, es);
		color->local_source(s, es);
	}
	void SetImage::value_source(std::ostream &s) const {
		s << "write_imagef(";
		image->value_source(s);
		s << ", ";
		position->value_source(s);
		s << ", ";
		color->value_source(s);
		s << ";";
	}
	void SetImage::push_arguments(ArgumentsStream &as) const {
		image->push_arguments(as);
		position->push_arguments(as);
		color->push_arguments(as);
	}
	void SetImage::set_arguments(ValuesStream &vs) const {
		image->set_arguments(vs);
		position->set_arguments(vs);
		color->set_arguments(vs);
	}
	
	void Sequence::global_source(std::ostream &s, ExpressionsSet &es) const {
		std::for_each(children.begin(), children.end(), [&](const std::shared_ptr<Expression> &i) {
			i->global_source(s, es);
		});
	}
	void Sequence::local_source(std::ostream &s, ExpressionsSet &es) const {
		std::for_each(children.begin(), children.end(), [&](const std::shared_ptr<Expression> &i) {
			i->local_source(s, es);
		});
	}
	void Sequence::value_source(std::ostream &s) const {
		std::for_each(children.begin(), children.end(), [&](const std::shared_ptr<Expression> &i) {
			i->value_source(s);
			s << ";\n";
		});
	}
	void Sequence::push_arguments(ArgumentsStream &as) const {
		std::for_each(children.begin(), children.end(), [&](const std::shared_ptr<Expression> &i) {
			i->push_arguments(as);
		});
	}
	void Sequence::set_arguments(ValuesStream &vs) const {
		std::for_each(children.begin(), children.end(), [&](const std::shared_ptr<Expression> &i) {
			i->set_arguments(vs);
		});
	}
	
	void ForRange::global_source(std::ostream &s, ExpressionsSet &es) const {
		index->global_source(s, es);
		begin->global_source(s, es);
		end->global_source(s, es);
		expression->global_source(s, es);
	}
	void ForRange::local_source(std::ostream &s, ExpressionsSet &es) const {
		index->local_source(s, es);
		begin->local_source(s, es);
		end->local_source(s, es);
		expression->local_source(s, es);
	}
	void ForRange::value_source(std::ostream &s) const {
		s << "for(";
		index->value_source(s);
		s << " = ";
		begin->value_source(s);
		s << "; ";
		index->value_source(s);
		s << " < ";
		end->value_source(s);
		s << "; ";
		index->value_source(s);
		s << "++) {\n";
		expression->value_source(s);
		s << ";\n};\n";
	}
	void ForRange::push_arguments(ArgumentsStream &as) const {
		index->push_arguments(as);
		begin->push_arguments(as);
		end->push_arguments(as);
		expression->push_arguments(as);
	}
	void ForRange::set_arguments(ValuesStream &vs) const {
		index->set_arguments(vs);
		begin->set_arguments(vs);
		end->set_arguments(vs);
		expression->set_arguments(vs);
	}
	
	void Cast::global_source(std::ostream &s, ExpressionsSet &es) const {
		e->global_source(s, es);
	}
	void Cast::local_source(std::ostream &s, ExpressionsSet &es) const {
		e->local_source(s, es);
	}
	void Cast::value_source(std::ostream &s) const {
		Type et = e->type();
		if(cast_to == et)
			e->value_source(s);
		else if(cast_to.is_vector()) {
			s << "convert_" << cast_to.name() << "(";
			e->value_source(s);
			s << ")";
		} else {
			s << "((" << cast_to.name() << ")";
			e->value_source(s);
			s << ")";
		}
	}
	void Cast::push_arguments(ArgumentsStream &as) const {
		e->push_arguments(as);
	}
	void Cast::set_arguments(ValuesStream &vs) const {
		e->set_arguments(vs);
	}
	Type Cast::type() const {
		return cast_to;
	}
	
}
