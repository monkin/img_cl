
#ifndef MAY_MCL_HPP
#define MAY_MCL_HPP

#include "CL/cl.h"
#include <string>
#include <vector>
#include <exception>
#include <memory>
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <iomanip>

namespace mcl {
	class Error: public std::exception {
	private:
		cl_int m_code;
	public:
		Error(const cl_int c) : m_code(c) {};
		cl_int code() const {
			return m_code;	
		}
		virtual const char *what() const throw();
		virtual ~Error() throw();
	};
	
	class Device;
	class Context;
	
	class Platform {
	private:
		static std::vector<Platform> m_platforms;
		cl_platform_id m_id;
		std::string info(cl_platform_info param_name) const {
			size_t p_size; 
			cl_int err_code = clGetPlatformInfo(m_id, param_name, 0, NULL, &p_size);
			if(err_code==CL_SUCCESS) {
				char * v = new char[p_size];
				err_code = clGetPlatformInfo(m_id, param_name, p_size, v, NULL);
				if(err_code!=CL_SUCCESS) {
					delete v;
					throw Error(err_code);
				} else {
					try {
						std::string r(v);
						delete v;
						return r;
					} catch(...) {
						delete v;
						throw;
					}
				}
			} else
				throw Error(err_code);
		};
	protected:
		Platform(const cl_platform_id p_id) : m_id(p_id) {};
		friend class Device;
	public:
		Platform(const Platform &p) : m_id(p.m_id) {};
		cl_platform_id id() const {
			return m_id;
		};
		std::string profile() const {
			return info(CL_PLATFORM_PROFILE);	
		};
		std::string version() const {
			return info(CL_PLATFORM_VERSION);	
		};
		std::string name() const {
			return info(CL_PLATFORM_NAME);	
		};
		const std::string vendor() const {
			return info(CL_PLATFORM_VENDOR);	
		};
		std::string extensions() const {
			return info(CL_PLATFORM_EXTENSIONS);	
		};
		std::vector<Device> devices() const;
		static std::vector<Platform> &platforms() {
			if(m_platforms.empty()) {
				cl_uint num_platforms;
				clGetPlatformIDs(0, NULL, &num_platforms);
				cl_platform_id* platform_ids;
				platform_ids = new cl_platform_id[num_platforms];
				try {
					clGetPlatformIDs(num_platforms, platform_ids, NULL);
					m_platforms.reserve(num_platforms);
					for(int i=0; i<num_platforms; i++)
						m_platforms.push_back(Platform(platform_ids[i]));
				} catch(...) {
					delete [] platform_ids;
					throw;
				}
				delete [] platform_ids;
			}
			return m_platforms;
		}
		bool operator==(const Platform &x) const {
			return id()==x.id();	
		}
		bool operator!=(const Platform &x) const {
			return id()!=x.id();	
		}
	};
	
	class Program;
	
	class Device {
	private:
		cl_device_id m_id;
	protected:
		friend class Platform;
		friend class Context;
		friend class Program;
		Device(cl_device_id d_id) : m_id(d_id) {};
		std::string info_str(const cl_device_info param_name) const {
			//cl_int clGetDeviceInfo (cl_device_id device,
			//		cl_device_info param_name, size_t param_value_size,
			//		void *param_value, size_t *param_value_size_ret)
			size_t l;
			cl_int err_code = clGetDeviceInfo(m_id, param_name, 0, NULL, &l);
			if(err_code==CL_SUCCESS) {
				char *s = new char[l];
				err_code = clGetDeviceInfo(m_id, param_name, l, s, NULL);
				if(err_code==CL_SUCCESS) {
					try {
						std::string r(s);
						delete [] s; s = NULL;						
						return r;
					} catch(...) {
						delete s;
						throw;
					}
				} else
					delete [] s;
			}
			throw Error(err_code);
		}
		template<typename T, const cl_device_info param_name>
		T info_t() const {
			T res;
			cl_int err_code = clGetDeviceInfo(m_id, param_name, sizeof(T), &res, NULL);
			if(err_code==CL_SUCCESS)
				return res;
			else
				throw Error(err_code);
		}
	public:
		Device(const Device &d) : m_id(d.m_id) {};
		cl_device_id id() const {
			return m_id;	
		}
		cl_uint address_bits() const {
			return info_t<cl_uint, CL_DEVICE_ADDRESS_BITS>();	
		}
		cl_bool available() const {
			return info_t<cl_bool, CL_DEVICE_AVAILABLE>();
		}
		cl_bool compiler_available() const {
			return info_t<cl_bool, CL_DEVICE_COMPILER_AVAILABLE>();
		}
		/*cl_device_fp_config double_fp_config() const {
			return info_t<cl_device_fp_config, CL_DEVICE_DOUBLE_FP_CONFIG>();
		}*/
		cl_bool endian_little() const {
			return info_t<cl_bool, CL_DEVICE_ENDIAN_LITTLE>();
		}
		cl_bool error_correction_support() const {
			return info_t<cl_bool, CL_DEVICE_ERROR_CORRECTION_SUPPORT>();
		}
		cl_device_exec_capabilities execution_capabilities() const {
			return info_t<cl_device_exec_capabilities, CL_DEVICE_EXECUTION_CAPABILITIES>();
		}
		std::string device_extensions() const {
			return info_str(CL_DEVICE_EXTENSIONS);	
		}
		cl_ulong global_mem_cache_size() const {
			return info_t<cl_ulong, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE>();
		}
		cl_device_mem_cache_type global_mem_cache_type() const {
			return info_t<cl_device_mem_cache_type, CL_DEVICE_GLOBAL_MEM_CACHE_TYPE>();
		}
		cl_uint global_mem_cacheline_size() const {
			return info_t<cl_uint, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE>();
		}
		cl_ulong global_mem_size() const {
			return info_t<cl_ulong, CL_DEVICE_GLOBAL_MEM_SIZE>();
		}
		/*const cl_device_fp_config half_fp_config() const {
			return info_t<cl_device_fp_config, CL_DEVICE_HALF_FP_CONFIG>();
		}*/
		cl_bool image_support() const {
			return info_t<cl_bool, CL_DEVICE_IMAGE_SUPPORT>();	
		}
		size_t image2d_max_height() const {
			return info_t<size_t, CL_DEVICE_IMAGE2D_MAX_HEIGHT>();
		}
		size_t image2d_max_width() const {
			return info_t<size_t, CL_DEVICE_IMAGE2D_MAX_WIDTH>();
		}
		size_t image3d_max_depth() const {
			return info_t<size_t, CL_DEVICE_IMAGE3D_MAX_DEPTH>();
		}
		size_t image3d_max_height() const {
			return info_t<size_t, CL_DEVICE_IMAGE3D_MAX_HEIGHT>();
		}
		size_t image3d_max_width() const {
			return info_t<size_t, CL_DEVICE_IMAGE3D_MAX_WIDTH>();
		}
		cl_ulong local_mem_size() const {
			return info_t<cl_ulong, CL_DEVICE_LOCAL_MEM_SIZE>();
		}
		cl_device_local_mem_type local_mem_type() const {
			return info_t<cl_device_local_mem_type, CL_DEVICE_LOCAL_MEM_TYPE>();
		}
		cl_uint max_clock_frequency() const {
			return info_t<cl_uint, CL_DEVICE_MAX_CLOCK_FREQUENCY>();
		}
		cl_uint max_compute_units() const {
			return info_t<cl_uint, CL_DEVICE_MAX_COMPUTE_UNITS>();
		}
		cl_uint max_constant_args() const {
			return info_t<cl_uint, CL_DEVICE_MAX_CONSTANT_ARGS>();
		}
		cl_ulong max_constant_buffer_size() const {
			return info_t<cl_ulong, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE>();
		}
		cl_ulong max_mem_alloc_size() const {
			return info_t<cl_ulong, CL_DEVICE_MAX_MEM_ALLOC_SIZE>();
		}
		size_t max_parameter_size() const {
			return info_t<size_t, CL_DEVICE_MAX_PARAMETER_SIZE>();
		}
		cl_uint max_read_image_args() const {
			return info_t<cl_uint, CL_DEVICE_MAX_READ_IMAGE_ARGS>();
		}
		cl_uint max_samplers() const {
			return info_t<cl_uint, CL_DEVICE_MAX_SAMPLERS>();
		}
		size_t max_work_group_size() const {
			return info_t<size_t, CL_DEVICE_MAX_WORK_GROUP_SIZE>();
		}
		cl_uint max_work_item_dimensions() const {
			return info_t<cl_uint, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS>();
		}
		std::vector<size_t> max_work_item_sizes() const {
			cl_uint sz = max_work_item_dimensions();
			std::vector<size_t> res;
			res.reserve(sz);
			size_t *r =	new size_t[sz];
			cl_int err_code = clGetDeviceInfo(m_id, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t[sz]), &r, NULL);
			if(err_code==CL_SUCCESS) {
				for(cl_uint i=0; i<sz; i++)
					res.push_back(r[i]);
				delete [] r;
				return res;
			} else
				delete [] r;
			throw Error(err_code);
		}
		cl_uint max_write_image_args() const {
			return info_t<cl_uint, CL_DEVICE_MAX_WRITE_IMAGE_ARGS>();
		}
		cl_uint mem_base_addr_align() const {
			return info_t<cl_uint, CL_DEVICE_MEM_BASE_ADDR_ALIGN>();
		}
		cl_uint min_data_type_align_size() const {
			return info_t<cl_uint, CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE>();
		}
		std::string name() const {
			return info_str(CL_DEVICE_NAME);	
		}
		Platform platform() const {
			return Platform(info_t<cl_platform_id, CL_DEVICE_PLATFORM>());
		}
		cl_uint prefered_vector_width_char() const {
			return info_t<cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR>();
		}
		cl_uint prefered_vector_width_short() const {
			return info_t<cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT>();
		}
		cl_uint prefered_vector_width_int() const {
			return info_t<cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT>();
		}
		cl_uint prefered_vector_width_long() const {
			return info_t<cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG>();
		}
		cl_uint prefered_vector_width_float() const {
			return info_t<cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT>();
		}
		cl_uint prefered_vector_width_double() const {
			return info_t<cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE>();
		}
		std::string profile() const {
			return info_str(CL_DEVICE_PROFILE);	
		}
		size_t profiling_timer_resolution() const {
			return info_t<size_t, CL_DEVICE_PROFILING_TIMER_RESOLUTION>();
		}
		cl_command_queue_properties queue_properties() const { 
			return info_t<cl_command_queue_properties, CL_DEVICE_QUEUE_PROPERTIES>();
		}
		cl_device_fp_config single_fp_config() const {
			return info_t<cl_device_fp_config, CL_DEVICE_SINGLE_FP_CONFIG>();	
		}
		cl_device_type type() const {
			return info_t<cl_device_type, CL_DEVICE_TYPE>();
		}
		bool is_cpu() const {
			return type()==CL_DEVICE_TYPE_CPU;	
		}
		bool is_gpu() const {
			return type()==CL_DEVICE_TYPE_GPU;
		}
		bool is_accelerator() const {
			return type()==CL_DEVICE_TYPE_ACCELERATOR;
		}
		std::string vendor() const {
			return info_str(CL_DEVICE_VENDOR);	
		}
		cl_uint vendor_id() const {
			return info_t<cl_uint, CL_DEVICE_VENDOR_ID>();	
		}
		std::string version() const {
			return info_str(CL_DEVICE_VERSION);	
		}
		std::string driver_version() const {
			return info_str(CL_DRIVER_VERSION);	
		}
		bool operator==(const Device &x) const {
			return id()==x.id();	
		}
		bool operator!=(const Device &x) const {
			return id()!=x.id();	
		}
	};
	
	class Buffer;
	class Image;
	
	class Context {
	private:
		cl_context context;
	protected:
		friend class Buffer;
		friend class Program;
		Context(const cl_context ctx) : context(ctx) {
			cl_int err_code = clRetainContext(context);
			if(err_code)
				throw Error(err_code);
		}
	public:
		Context(const std::vector<Device> &ds) {
			cl_device_id *devs = new cl_device_id[ds.size()];
			cl_int err_code;
			for(size_t i=0; i<ds.size(); i++)
				devs[i] = ds[i].id();
			context = clCreateContext (NULL, ds.size(), devs, NULL, NULL, &err_code);
			delete [] devs;
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
		}
		Context(const Device &d) {
			cl_device_id dev = d.id();
			cl_int err_code;
			context = clCreateContext (NULL, 1, &dev, NULL, NULL, &err_code);
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
		}
		Context(const Context &c) {
			context = c.context;
			cl_int err_code = clRetainContext(context);
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
		}
		Context(Context &&c) {
			context = c.context;
			c.context = NULL;
		}
		Context &operator=(const Context &c) {
			if(this!=&c) {
				context = c.context;
				cl_int err_code = clRetainContext(context);
				if(err_code!=CL_SUCCESS)
					throw Error(err_code);
			}
		}
		~Context() {
			if(context) {
				cl_int err_code = clReleaseContext(context);
				if(err_code!=CL_SUCCESS)
					throw Error(err_code);
			}
		}
		cl_context id() const {
			return context;	
		}
		cl_int reference_count() const {
			cl_int r;
			cl_int err_code = clGetContextInfo(context, CL_CONTEXT_REFERENCE_COUNT, sizeof(r), &r, NULL);
			if(err_code==CL_SUCCESS)
				return r;
			else
				throw Error(err_code);
		}
		std::vector<Device> devices() const {
			std::vector<Device> r;
			size_t sz;
			cl_int err_code = clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &sz);
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
			sz /= sizeof(cl_device_id);
			r.reserve(sz);
			cl_device_id *ds = new cl_device_id[sz];
			err_code = clGetContextInfo(context, CL_CONTEXT_DEVICES, sizeof(cl_device_id[sz]), ds, NULL);
			if(err_code==CL_SUCCESS) {
				try {
					for(size_t i=0; i<sz; i++)
						r.push_back(Device(ds[i]));
					delete [] ds;
					ds = 0;
					return r;
				} catch(...) {
					delete [] ds;
					throw;					
				}
			}
			delete [] ds; 
			throw Error(err_code);
		}
		bool operator==(const Context &x) const {
			return id()==x.id();	
		}
		bool operator!=(const Context &x) const {
			return id()!=x.id();	
		}
		Buffer buffer(size_t sz) const;
		Buffer buffer_r(size_t sz) const;
		Buffer buffer_w(size_t sz) const;
		Buffer buffer_rw(size_t sz) const;
		Image image(const cl_image_format &f, size_t w, size_t h) const;
		Image image_r(const cl_image_format &f, size_t w, size_t h) const;
		Image image_w(const cl_image_format &f, size_t w, size_t h) const;
		Image image_rw(const cl_image_format &f, size_t w, size_t h) const;
	};
	
	class Buffer {
	private:
		cl_mem buffer;
	protected:
		friend class Context;
		Buffer(const Context &ctx, size_t sz, bool readable, bool writable) {
			cl_int err_code;
			if(readable || writable) {
				buffer = clCreateBuffer(ctx.id(), (readable && writable) ? CL_MEM_READ_WRITE : (readable ? CL_MEM_READ_ONLY : CL_MEM_WRITE_ONLY), sz, NULL, &err_code);
				if(err_code!=CL_SUCCESS)
					throw new Error(err_code);
			} else
				throw Error(CL_INVALID_VALUE);
		}
		template<typename T>
		T info_t(const cl_mem_info mi) const {
			T r;
			cl_int err_code = clGetMemObjectInfo(buffer, mi, sizeof(r), &r, NULL);
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
			else
				return r;
		}
	public:
		cl_mem id() const {
			return buffer;	
		}
		Buffer(const Buffer &b) {
			buffer = b.buffer;
			cl_int  err_code = clRetainMemObject(buffer);
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
		}
		Buffer(Buffer &&b) {
			buffer = b.buffer;
			b.buffer = NULL;
		}
		Buffer &operator= (const Buffer &b) {
			if(this!=&b) {
				cl_int err_code = clReleaseMemObject(buffer);
				if(err_code!=CL_SUCCESS)
					throw Error(err_code);
				buffer = b.buffer;
				err_code = clRetainMemObject(buffer);
				if(err_code!=CL_SUCCESS)
					throw Error(err_code);
			}
			return *this;
		}
		~Buffer() {
			if(buffer)
				clRetainMemObject(buffer);
		}
		bool is_read_only() const {
			return (info_t<cl_mem_flags>(CL_MEM_FLAGS) & CL_MEM_READ_ONLY)!=0;
		}
		bool is_write_only() const {
			return (info_t<cl_mem_flags>(CL_MEM_FLAGS) & CL_MEM_WRITE_ONLY)!=0;
		}
		bool is_read_write() const {
			return (info_t<cl_mem_flags>(CL_MEM_FLAGS) & CL_MEM_READ_WRITE)!=0;
		}
		size_t size() const {
			return info_t<size_t>(CL_MEM_SIZE);
		}
		cl_uint reference_count() const {
			return info_t<cl_uint>(CL_MEM_REFERENCE_COUNT);	
		}
		Context context() const {
			return Context(info_t<cl_context>(CL_MEM_CONTEXT));
		}
		bool operator==(const Buffer &x) const {
			return id()==x.id();	
		}
		bool operator!=(const Buffer &x) const {
			return id()!=x.id();	
		}
	};
	
	namespace PixelFormat {
		extern const cl_image_format grayscale_8bit;// = { CL_LUMINANCE, CL_UNORM_INT8 };
		extern const cl_image_format grayscale_16bit;// = { CL_LUMINANCE, CL_UNORM_INT16 };
		extern const cl_image_format grayscale_float;// = { CL_LUMINANCE, CL_FLOAT };
		extern const cl_image_format rgba_8bit;// = { CL_RGBA, CL_UNORM_INT8 };
		extern const cl_image_format rgba_16bit;// = { CL_RGBA, CL_UNORM_INT16 };
		extern const cl_image_format rgba_float;// = { CL_RGBA, CL_FLOAT };
	}
	
	class Image {
	private:
		cl_mem image;
		template<typename T>
		T info_t(const cl_image_info param_name) const {
			T r;
			cl_int err_code = clGetImageInfo (image, param_name, sizeof(T), &r, NULL);
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
			return r;
		}
	protected:
		friend class Context;
		Image(const Context &c, const cl_image_format format, size_t w, size_t h, const cl_mem_flags flags=CL_MEM_READ_WRITE) {
			cl_int err_code;
			image = clCreateImage2D(c.id(), flags, &format, w, h, 0, 0, &err_code);
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
		}
	public:
		Image(const Image &img) : image(img.image) {
			cl_int err_code = clRetainMemObject(image);
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
		}
		Image(Image &&img) : image(img.image) {
			img.image = NULL;
		}
		cl_mem id() const {
			return image;
		}
		Image &operator=(const Image &img) {
			if(this!=&img) {
				cl_int err_code = clReleaseMemObject(image);
				if(err_code!=CL_SUCCESS)
					throw Error(err_code);
				image = img.image;
				err_code = clRetainMemObject(image);
				if(err_code!=CL_SUCCESS)
					throw Error(err_code);
			}
			return *this;
		}
		size_t width() const {
			return info_t<size_t>(CL_IMAGE_WIDTH);
		}
		size_t height() const {
			return info_t<size_t>(CL_IMAGE_HEIGHT);
		}
		size_t element_size() const {
			return info_t<size_t>(CL_IMAGE_ELEMENT_SIZE);
		}
		const cl_image_format format() const {
			return info_t<cl_image_format>(CL_IMAGE_FORMAT);
		}
		~Image() {
			if(image)
				clReleaseMemObject(image);
		}
		bool operator==(const Image &x) const {
			return id()==x.id();	
		}
		bool operator!=(const Image &x) const {
			return id()!=x.id();	
		}
	};
	
	class Sampler {
	private:
		cl_sampler sampler;
		Sampler(const Context &c, cl_addressing_mode addressing_mode, cl_filter_mode filter_mode) {
			cl_int err_code;
			sampler = clCreateSampler(c.id(), true, addressing_mode, filter_mode, &err_code);
			if(err_code)
				throw Error(err_code);
		}
	public:
		Sampler(const Sampler &s) : sampler(s.sampler) {
			cl_int err_code = clRetainSampler(sampler);
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
		}
		Sampler(Sampler &&s) : sampler(s.sampler) {
			s.sampler = NULL;
		}
		cl_sampler id() const {
			return sampler;	
		}
		Sampler &operator= (const Sampler &s) {
			cl_int err_code = clReleaseSampler(sampler);
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
			sampler = s.sampler;
			err_code = clRetainSampler(sampler);
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
			return *this;
		}
		~Sampler() {
			if(sampler)
				clReleaseSampler(sampler);	
		}
		static Sampler linear_mirrored_repeat(const Context &c) {
			return Sampler(c, CL_ADDRESS_MIRRORED_REPEAT, CL_FILTER_LINEAR);
		}
		static Sampler linear_repeat(const Context &c) {
			return Sampler(c, CL_ADDRESS_REPEAT, CL_FILTER_LINEAR);
		}
		static Sampler linear_clamp_to_edge(const Context &c) {
			return Sampler(c, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_LINEAR);
		}
		static Sampler linear_none(const Context &c) {
			return Sampler(c, CL_ADDRESS_CLAMP, CL_FILTER_LINEAR);
		}
		static Sampler nearest_mirrored_repeat(const Context &c) {
			return Sampler(c, CL_ADDRESS_MIRRORED_REPEAT, CL_FILTER_NEAREST);
		}
		static Sampler nearest_repeat(const Context &c) {
			return Sampler(c, CL_ADDRESS_REPEAT, CL_FILTER_NEAREST);
		}
		static Sampler nearest_clamp_to_edge(const Context &c) {
			return Sampler(c, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_NEAREST);
		}
		static Sampler nearest_none(const Context &c) {
			return Sampler(c, CL_ADDRESS_CLAMP, CL_FILTER_NEAREST);
		}
	};
	
	
	template<class T>
	void program_cb(cl_program p, void *data) {
		try {
			reinterpret_cast<T *>(data)();
			delete reinterpret_cast<T *>(data);
		} catch(...) {
			delete reinterpret_cast<T *>(data);
			throw;
		}
	}
	
	class Kernel {
	private:
		cl_kernel kernel;
	protected:
		friend class Program;
		Kernel(const Program &p, const std::string &nm);
	public:
		const cl_kernel id() const {
			return kernel;
		}
		Kernel(const Kernel &k) : kernel(k.kernel) {
			cl_int err_code = clRetainKernel(kernel);
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
		}
		Kernel(Kernel &&k) : kernel(k.kernel) {
			k.kernel = NULL;
		}
		Kernel &operator=(const Kernel &c) {
			if(this!=&c) {
				cl_int err_code = clReleaseKernel(kernel);
				if(err_code!=CL_SUCCESS)
					throw Error(err_code);
				kernel = c.kernel;
				err_code = clRetainKernel(kernel);
				if(err_code!=CL_SUCCESS)
					throw Error(err_code);
			}
			return *this;
		}
		template<typename T>
		Kernel &set_arg(cl_uint arg_index, const T &arg);
		~Kernel() {
			if(kernel)
				clReleaseKernel(kernel);
		}
	};
	
	template<typename T>
	Kernel &Kernel::set_arg(cl_uint arg_index, const T &arg) {
		cl_int err_code = clSetKernelArg(kernel, arg_index, sizeof(T), &arg);
		if(err_code!=CL_SUCCESS)
			throw Error(err_code);
		return *this;
	}
	
	template<>
	Kernel &Kernel::set_arg<Buffer>(cl_uint arg_index, const Buffer &arg);
	template<>
	Kernel &Kernel::set_arg<Image>(cl_uint arg_index, const Image &arg);
	template<>
	Kernel &Kernel::set_arg<Sampler>(cl_uint arg_index, const Sampler &arg);
	
	class Program {
		private:
			cl_program program;
			void init(const Context &c, const std::vector<Device> &dv, const std::vector<std::string> &sv) {
				cl_int err_code;
				const char **strings = 0;
				size_t *lengths = 0;
				try {
					strings = new const char *[sv.size()];
					lengths = new size_t[sv.size()];
					for(size_t i=0; i<sv.size(); i++) {
						strings[i] = sv[i].c_str();
						lengths[i] = sv[i].length();
					}
					program = clCreateProgramWithSource(c.id(), sv.size(), strings, lengths, &err_code);
					delete [] strings;
					delete [] lengths;
					if(err_code!=CL_SUCCESS)
						throw Error(err_code);
				} catch(...) {
					delete [] strings;
					delete [] lengths;
					throw;	
				}
			}
			template<typename T>
			T info_t(const cl_program_info param_name) const {
				T r;
				cl_int err_code = clGetProgramInfo(program, param_name, sizeof(T), &r, NULL);
				if(err_code==CL_SUCCESS)
					return r;
				else
					throw Error(err_code);
			}
		public:
			Program(const Context &c, const std::vector<std::string> &sv) {
				init(c, c.devices(), sv);
			}
			Program(const Context &c, const std::string &s) {
				init(c, c.devices(), {s});
			}
			Program(const Context &c, const std::vector<Device> &dv, const std::vector<std::string> &sv) {
				init(c, dv, sv);
			}
			Program(const Context &c, const std::vector<Device> &dv, const std::string &s) {
				init(c, dv, {s});
			}
			Program(const Program &p) : program(p.program) {
				cl_int err_code = clRetainProgram(program);
				if(err_code!=CL_SUCCESS)
					throw Error(err_code);
			}
			Program(Program &&p) : program(p.program) {
				p.program = NULL;
			}
			~Program() {
				if(program)
					clReleaseProgram(program);
			}
			Program &operator=(const Program &p) {
				if(this!=&p) {
					cl_int err_code = clReleaseProgram(program);
					if(err_code!=CL_SUCCESS)
						throw Error(err_code);
					program = p.program;
					err_code = clRetainProgram(program);
					if(err_code!=CL_SUCCESS)
						throw Error(err_code);
				}
				return *this;
			}
			cl_program id() const {
				return program;	
			}
			void build(const std::string &s = "") {
				cl_int err_code = clBuildProgram(program, 0, NULL, s.c_str(), NULL, NULL);
				if(err_code!=CL_SUCCESS)
					throw Error(err_code);
			}
			template<class T>
			void build(const std::string &s, T &&cb) {
				cl_int err_code = clBuildProgram(program, 0, NULL, s.c_str(), program_cb<T>, new T(cb));
				if(err_code!=CL_SUCCESS)
					throw Error(err_code);
			}
			Context context() const {
				return Context(info_t<cl_context>(CL_PROGRAM_CONTEXT));
			}
			cl_uint reference_count() const {
				return info_t<cl_uint>(CL_PROGRAM_REFERENCE_COUNT);
			}
			std::vector<Device> devices() const {
				size_t arr_size;
				cl_int err_code = clGetProgramInfo(program, CL_PROGRAM_DEVICES, 0, NULL, &arr_size);
				if(err_code!=CL_SUCCESS)
					throw Error(err_code);
				std::vector<Device> r;
				arr_size /= sizeof(cl_device_id);
				r.reserve(arr_size);
				cl_device_id *arr = new cl_device_id[arr_size];
				try {
					err_code = clGetProgramInfo(program, CL_PROGRAM_DEVICES, sizeof(cl_device_id[arr_size]), arr, NULL);
					if(err_code!=CL_SUCCESS)
						throw Error(err_code);
					std::transform(arr, arr+arr_size, std::back_inserter(r), [](const cl_device_id &d_id) {
						return Device(d_id);
					});
					delete [] arr;
					return r;
				} catch(...) {
					delete [] arr;
					throw;	
				}
			}
			cl_build_status build_status(const Device &d) const {
				cl_build_status r;
				cl_int err_code = clGetProgramBuildInfo(program, d.id(), CL_PROGRAM_BUILD_STATUS, sizeof(cl_build_status), &r, NULL);
				if(err_code!=CL_SUCCESS)
					throw Error(err_code);
				return r;
			}
			std::string build_log(const Device &d) const {
				size_t rl;
				cl_int err_code = clGetProgramBuildInfo(program, d.id(), CL_PROGRAM_BUILD_LOG, 0, NULL, &rl);
				if(err_code!=CL_SUCCESS)
					throw Error(err_code);
				char *s = new char[rl];
				try {
					err_code = clGetProgramBuildInfo(program, d.id(), CL_PROGRAM_BUILD_LOG, rl, s, NULL);
					if(err_code!=CL_SUCCESS)
						throw Error(err_code);
					std::string r = s;
					delete [] s;
					s = 0;
					return r;
				} catch(...) {
					delete [] s;
					throw;
				}
			}
			Kernel kernel(const std::string &nm) const {
				return Kernel(*this, nm);
			}
	};
	
	class Queue {
	private:
		cl_command_queue queue;
		template<typename T, cl_command_queue_info QI>
		const T info_t() const {
			T r;
			cl_int err_code = clGetCommandQueueInfo(queue, QI, sizeof(r), &r, 0);
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
			else
				return r;	
		}
	public:
		Queue(const Context &ctx, const Device &dv, bool out_of_order=true, bool enable_profiling=false) {
			cl_int err_code;
			queue = clCreateCommandQueue(ctx.id(), dv.id(), (out_of_order ? CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE : 0) | (enable_profiling ? CL_QUEUE_PROFILING_ENABLE : 0), &err_code);
		}
		Queue(const Queue &q) : queue(q.queue) {
			cl_int err_code = clRetainCommandQueue(queue);
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
		}
		Queue(Queue &&q) : queue(q.queue) {
			q.queue = NULL;
		}
		Queue &operator=(const Queue &q) {
			if(this!=&q) {
				cl_int err_code = clReleaseCommandQueue(queue);
				if(err_code!=CL_SUCCESS)
					throw Error(err_code);
				queue = q.queue;
				err_code = clRetainCommandQueue(queue);
				if(err_code!=CL_SUCCESS)
					throw Error(err_code);
			}
			return *this;
		}
		cl_command_queue id() const  {
			return queue;	
		}
		cl_context context() const {
			return info_t<cl_context, CL_QUEUE_CONTEXT>();
		}
		cl_device_id device() const {
			return info_t<cl_device_id, CL_QUEUE_DEVICE>();
		}
		cl_int reference_count() const {
			return info_t<cl_int, CL_QUEUE_REFERENCE_COUNT>();
		}
		bool is_out_of_order() const {
			return (info_t<cl_command_queue_properties, CL_QUEUE_PROPERTIES>() & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE)!=0;
		}
		bool is_profiling_enabled() const {
			return (info_t<cl_command_queue_properties, CL_QUEUE_PROPERTIES>() & CL_QUEUE_PROFILING_ENABLE)!=0;
		}
		template<typename T>
		Queue &mov(const Buffer &b, T *v) {
			cl_int err_code = clEnqueueReadBuffer(queue, b.id(), false, 0, b.size(), v, 0, NULL, NULL);
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
			return *this;
		}
		template<typename T>
		Queue &mov(const T *v, const Buffer &b) {
			cl_int err_code = clEnqueueWriteBuffer(queue, b.id(), false, 0, b.size(), v, 0, NULL, NULL);
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
			return *this;
		}
		template<typename T>
		Queue &mov(const Buffer &b, std::vector<T> &v) {
			assert(b.size()%sizeof(T)==0);
			v.resize(b.size()/sizeof(T));
			mov(b, v.data());
			return *this;
		}
		template<typename T>
		Queue &mov(const std::vector<T> &v, const Buffer &b) {
			assert(b.size()==v.size()*sizeof(T));
			mov(v.data(), b);
			return *this;
		}
		template<typename T>
		Queue &mov(const Image &img, T *v) {
			size_t origin[] = { 0, 0, 0 };
			size_t region[] = { img.width(), img.height(), 1 };
			cl_int err_code = clEnqueueReadImage(queue, img.id(), false, origin, region, 0, 0, v, 0, NULL, NULL);
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
			return *this;
		}
		template<typename T>
		Queue &mov(const T *v, const Image &img) {
			size_t origin[] = { 0, 0, 0 };
			size_t region[] = { img.width(), img.height(), 1 };
			cl_int err_code = clEnqueueWriteImage(queue, img.id(), false, origin, region, 0, 0, v, 0, NULL, NULL);
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
			return *this;
		}
		template<typename T>
		Queue &mov(const Image &img, std::vector<T> *v) {
			v.resize(img.element_size()*img.width()*img.height()/sizeof(T));
			mov(img, v.data());
			return *this;
		}
		template<typename T>
		Queue &mov(const std::vector<T> *v, const Image &img) {
			assert(v.size()*sizeof(T)==img.element_size()*img.width()*img.height());
			mov(v.data(), img);
			return *this;
		}
		Queue &flush() {
			cl_int err_code = clFlush(queue);
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
			return *this;
		}
		Queue &finish() {
			cl_int err_code = clFinish(queue);
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
			return *this;
		}
		Queue &barrier() {
			cl_int err_code = clEnqueueBarrier(queue);
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
			return *this;
		}
		Queue &task(const Kernel &k) {
			cl_int err_code = clEnqueueTask(queue, k.id(), 0, NULL, NULL);
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
			return *this;
		}
		Queue &task(const Kernel &k, size_t items) {
			cl_int err_code = clEnqueueNDRangeKernel(queue, k.id(), 1, NULL, &items, NULL, 0, NULL, NULL);
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
			return *this;
		}
		Queue &task(const Kernel &k, size_t items_x, size_t items_y) {
			size_t sz[] = { items_x, items_y };
			cl_int err_code = clEnqueueNDRangeKernel(queue, k.id(), 2, NULL, sz, NULL, 0, NULL, NULL);
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
			return *this;
		}
		Queue &task(const Kernel &k, size_t items_x, size_t items_y, size_t items_z) {
			size_t sz[] = { items_x, items_y, items_z };
			cl_int err_code = clEnqueueNDRangeKernel(queue, k.id(), 3, NULL, sz, NULL, 0, NULL, NULL);
			if(err_code!=CL_SUCCESS)
				throw Error(err_code);
			return *this;	
		}
		~Queue() {
			if(queue)
				clReleaseCommandQueue(queue);
		}
	};
	
}


#endif //MAY_MCL__HPP

