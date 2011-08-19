
#include "mcl/mcl.hpp"
#include "mcl/mclang.hpp"
#include <wx/wx.h>
#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include <iostream>
#include <algorithm>

namespace m = mclang;

class Source
{
public:
	Source(const char* filename) : m_size(0), m_source(0) {
		std::ifstream file( filename,
		std::ios::in | std::ios::binary | std::ios::ate );
		if(file.is_open()) {
			m_size = size_t(file.tellg()) + 1;
			m_source = new char[m_size];
			file.seekg(0, std::ios::beg );
			file.read(m_source, m_size-1);
			file.close();
			m_source[m_size-1] = '\0';
		}
	}
	virtual ~Source() {
		if(m_source)
			delete [] m_source;
	}
	const char* get() {
		return m_source;
	}
	std::pair<char*, size_t> getPair() {
		return std::make_pair( m_source, m_size );
	}
	size_t size() {
		return m_size;
	}
private:
	size_t m_size;
	char*  m_source; 
};

class A {};
class B : public A {};
A operator+(const std::shared_ptr<A> &, const std::shared_ptr<A> &) {
	return A();	
}

int main() {
	std::shared_ptr<B> x(new B());
	A y = x + x;
	/*wxInitAllImageHandlers();
	wxImage img(L"./img.jpg");
	
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	cl_context_properties cps[3] = { 
		CL_CONTEXT_PLATFORM, 
		(cl_context_properties)(platforms[0])(), 
		0 
	};
	
	std::cout << 1;
	std::cout.flush();
	cl::Context context(CL_DEVICE_TYPE_CPU, cps);

	// Get a list of devices on this platform
	std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();

	// Create a command queue and use the first device
	cl::CommandQueue queue(context, devices[0]);
	
	cl::Image2D climg(context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RGB, CL_UNSIGNED_INT8), img.GetWidth(), img.GetHeight());
	cl::Image2D clrimg(context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RGB, CL_UNSIGNED_INT8), img.GetWidth(), img.GetHeight());
	
	cl::size_t<3> origin;
	origin[0] = origin[1] = origin[2] = 0;                                        
	
	cl::size_t<3> sz;
	sz[0] = img.GetWidth();
	sz[1] = img.GetHeight();
	sz[2] = 1;
	
	std::cout << queue.enqueueWriteImage(climg, true, origin, sz, img.GetWidth()*3, img.GetWidth()*3, img.GetData());
	std::cout << 3;
	std::cout.flush();
	Source source("test.cl");
	cl::Program program(context, cl::Program::Sources(1, source.getPair()));
	if(program.build(devices)!=CL_SUCCESS) {
		std::cout << "Build Status: " << program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(devices[0]) << std::endl;
		std::cout << "Build Options:\t" << program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(devices[0]) << std::endl;
		std::cout << "Build Log:\t " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;
	}
	cl::Kernel kernel(program, "blur");
	kernel.setArg(0, img.GetWidth());
	kernel.setArg(1, img.GetHeight());
	kernel.setArg(2, climg);
	kernel.setArg(3, clrimg);
	queue.finish();
	queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(img.GetWidth(), img.GetHeight()), cl::NDRange(1, 1));
	queue.finish();
	unsigned char *dt = reinterpret_cast<unsigned char *>(malloc(img.GetWidth()*img.GetHeight()*3));
	queue.enqueueReadImage(clrimg, true, origin, sz, img.GetWidth()*3, img.GetWidth()*3, dt);
	queue.finish();
	img.SetData(dt);
	img.SaveFile("out.jpg", wxBITMAP_TYPE_JPEG);*/
	try {
		std::for_each(mcl::Platform::platforms().begin(), mcl::Platform::platforms().end(), [](const mcl::Platform &p) {
				std::cout << p.id() << "; " << p.profile() << "; " << p.vendor() << "; " << p.name() << "; " << p.version() << std::endl;
				std::vector<mcl::Device> devs = p.devices();
				std::for_each(devs.begin(), devs.end(), [](const mcl::Device &d) {
						std::cout << "    " << d.id() << "; " << d.name() << "; " << d.version() << std::endl;
						mcl::Context context(d);
						std::vector<float> v_buff;
						for(int i=0; i<64; i++)
							v_buff.push_back(i);
						mcl::Buffer buff = context.buffer(sizeof(cl_float[64]));
						mcl::Buffer b2(buff);
						
						auto pbuff = m::argv<cl_float>(buff);
						auto ptree = m::set(m::select(pbuff, m::get_global_id(0)), m::select(pbuff, m::get_global_id(0)) * m::cnst(2));
						
						std::cout << std::endl << ptree->build() << std::endl;
						
						mcl::Program program(context, ptree->build());
						try {
							program.build();
						} catch(const mcl::Error &e) {
							std::cerr << "log: " << program.build_log(d) << std::endl;
							throw;
						}
						mcl::Kernel kernel = program.kernel("main_kernel");
						kernel.set_arg(0, buff);
						mcl::Queue queue(context, d);
						queue.mov(v_buff, buff).barrier();
						queue.task(kernel, 64).barrier();
						queue.mov(buff, v_buff).finish();
						std::cout << "array: ";
						std::for_each(v_buff.begin(), v_buff.end(), [](const float &f) {
							std::cout << f << " ";
						});
						std::cout << std::endl;
					});
			});
	} catch(const mcl::Error &e) {
		std::cerr << "error: " << e.code() << ": " << e.what() << std::endl;	
	}
	return 0;
}
