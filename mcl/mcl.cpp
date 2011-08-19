
#include "mcl.hpp"
#include <vector>

struct err_message_t {
	cl_int code;
	const char * message;
};

static const std::vector<err_message_t> err_messages = {
	{ CL_INVALID_VALUE, "CL_INVALID_VALUE: invalid function parametrs." },
	{ CL_INVALID_PLATFORM, "CL_INVALID_PLATFORM: platform is not a valid platform." },
	{ CL_INVALID_DEVICE_TYPE, "CL_INVALID_DEVICE_TYPE: device_type is not a valid value." },
	{ CL_DEVICE_NOT_FOUND, "CL_DEVICE_NOT_FOUND: no OpenCL devices that matched device_type were found." },
	{ CL_INVALID_VALUE, "CL_INVALID_VALUE: some arguments are invalid." },
	{ CL_INVALID_DEVICE, "CL_INVALID_DEVICE: devices contains an invalid device or are not associated with the specified platform."},
	{ CL_DEVICE_NOT_AVAILABLE, "CL_DEVICE_NOT_AVAILABLE: device in devices is currently not available even though the device was returned by clGetDeviceIDs." },
	{ CL_OUT_OF_HOST_MEMORY, "CL_OUT_OF_HOST_MEMORY: there is a failure to allocate resources required by the OpenCL implementation on the host." },
	{ CL_INVALID_CONTEXT, "CL_INVALID_CONTEXT: context is not a valid OpenCL context." },
	{ CL_OUT_OF_RESOURCES, "CL_OUT_OF_RESOURCES: there is a failure to allocate resources required by the OpenCL implementation on the device." },
	{ CL_INVALID_QUEUE_PROPERTIES, "CL_INVALID_QUEUE_PROPERTIES: values specified in properties are valid but are not supported by the device." },
	{ CL_INVALID_COMMAND_QUEUE, "CL_INVALID_COMMAND_QUEUE: command_queue is not a valid command-queue." },
	{ CL_INVALID_BUFFER_SIZE, "CL_INVALID_BUFFER_SIZE: size is 0 or is greater than CL_DEVICE_MAX_MEM_ALLOC_SIZE value specified in table of OpenCL Device Queries for clGetDeviceInfo for all devices in context." },
	{ CL_INVALID_HOST_PTR, "CL_INVALID_HOST_PTR: host_ptr is NULL and CL_MEM_USE_HOST_PTR or CL_MEM_COPY_HOST_PTR are set in flags or if host_ptr is not NULL but CL_MEM_COPY_HOST_PTR or CL_MEM_USE_HOST_PTR are not set in flags." },
	{ CL_MEM_OBJECT_ALLOCATION_FAILURE, "CL_MEM_OBJECT_ALLOCATION_FAILURE: there is a failure to allocate memory for buffer object." },
	{ CL_INVALID_PROGRAM, "CL_INVALID_PROGRAM: program is not a valid program object." },
	{ CL_INVALID_BINARY, "CL_INVALID_BINARY: program is created with clCreateWithProgramWithBinary and devices listed in device_list do not have a valid program binary loaded." },
	{ CL_INVALID_BUILD_OPTIONS, "CL_INVALID_BUILD_OPTIONS: the build options specified by options are invalid." },
	{ CL_INVALID_OPERATION, "CL_INVALID_OPERATION: the build of a program executable for any of the devices listed in device_list by a previous call to clBuildProgram for program has not completed." },
	{ CL_COMPILER_NOT_AVAILABLE, "CL_COMPILER_NOT_AVAILABLE: program is created with clCreateProgramWithSource and a compiler is not available i.e. CL_DEVICE_COMPILER_AVAILABLE specified in the table of OpenCL Device Queries for clGetDeviceInfo is set to CL_FALSE." },
	{ CL_BUILD_PROGRAM_FAILURE, "CL_BUILD_PROGRAM_FAILURE: there is a failure to build the program executable. This error will be returned if clBuildProgram does not return until the build has completed." },
	{ CL_INVALID_KERNEL_ARGS, "CL_INVALID_KERNEL_ARGS: the kernel argument values have not been specified."},
	{ CL_SUCCESS, "CL_SUCCESS: all right." }
};

static const char *unknown_error = "OpenCL unknown error.";

namespace mcl {
	std::vector<Platform> Platform::m_platforms;
	const char *Error::what() const throw() {
		for(auto i=err_messages.begin(); i!=err_messages.end(); i++) {
			if(i->code==m_code)
				return i->message;
		}
		return unknown_error;
	};
	Error::~Error() throw() {};
	std::vector<Device> Platform::devices() const {
		cl_uint num_devices;
		cl_int err_code = clGetDeviceIDs(m_id, CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices);
		if(err_code==CL_SUCCESS) {
			std::vector<Device> r;
			r.reserve(num_devices);
			cl_device_id *ds = new cl_device_id[num_devices];
			//cl_int clGetDeviceIDs (cl_platform_id platform,
			//	cl_device_type device_type, cl_uint num_entries, cl_device_id *devices, cl_uint *num_devices)
			err_code = clGetDeviceIDs(m_id, CL_DEVICE_TYPE_ALL, num_devices, ds, NULL);
			if(err_code==CL_SUCCESS) {
				for(int i=0; i<num_devices; i++)
					r.push_back(Device(ds[i]));
				return r;
			} else
				delete [] ds;
		}
		throw Error(err_code);
	}
	Buffer Context::buffer(const size_t sz) const {
		return Buffer(*this, sz, true, true);
	}
	Buffer Context::buffer_r(const size_t sz) const {
		return Buffer(*this, sz, true, false);
	}
	Buffer Context::buffer_w(const size_t sz) const {
		return Buffer(*this, sz, false, true);
	}
	Buffer Context::buffer_rw(const size_t sz) const {
		return buffer(sz);
	}
	Image Context::image(const cl_image_format &f, const size_t w, const size_t h) const {
		return Image(*this, f, w, h, CL_MEM_READ_WRITE);	
	}
	Image Context::image_r(const cl_image_format &f, const size_t w, const size_t h) const {
		return Image(*this, f, w, h, CL_MEM_READ_ONLY);	
	}
	Image Context::image_w(const cl_image_format &f, const size_t w, const size_t h) const {
		return Image(*this, f, w, h, CL_MEM_WRITE_ONLY);	
	}
	Image Context::image_rw(const cl_image_format &f, const size_t w, const size_t h) const {
		return Image(*this, f, w, h, CL_MEM_READ_WRITE);	
	}
	Kernel::Kernel(const Program &p, const std::string &nm) {
		cl_int err_code;
		kernel = clCreateKernel(p.id(), nm.c_str(), &err_code);
		if(err_code!=CL_SUCCESS)
			throw Error(err_code);
	}
	
	template<>
	Kernel &Kernel::set_arg<Buffer>(cl_uint arg_index, const Buffer &arg) {
		cl_mem buff_id = arg.id();
		cl_int err_code = clSetKernelArg(kernel, arg_index, sizeof(cl_mem), &buff_id);
		if(err_code!=CL_SUCCESS)
			throw Error(err_code);
		return *this;
	}
	template<>
	Kernel &Kernel::set_arg<Image>(cl_uint arg_index, const Image &arg) {
		cl_mem buff_id = arg.id();
		cl_int err_code = clSetKernelArg(kernel, arg_index, sizeof(cl_mem), &buff_id);
		if(err_code!=CL_SUCCESS)
			throw Error(err_code);
		return *this;
	}
	template<>
	Kernel &Kernel::set_arg<Sampler>(cl_uint arg_index, const Sampler &arg) {
		cl_sampler sampler_id = arg.id();
		cl_int err_code = clSetKernelArg(kernel, arg_index, sizeof(cl_sampler), &sampler_id);
		if(err_code!=CL_SUCCESS)
			throw Error(err_code);
		return *this;
	}
	
	namespace PixelFormat {
		const cl_image_format grayscale_8bit = { CL_LUMINANCE, CL_UNORM_INT8 };
		const cl_image_format grayscale_16bit = { CL_LUMINANCE, CL_UNORM_INT16 };
		const cl_image_format grayscale_float = { CL_LUMINANCE, CL_FLOAT };
		const cl_image_format rgba_8bit = { CL_RGBA, CL_UNORM_INT8 };
		const cl_image_format rgba_16bit = { CL_RGBA, CL_UNORM_INT16 };
		const cl_image_format rgba_float = { CL_RGBA, CL_FLOAT };	
	}
}

