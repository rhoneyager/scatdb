// Code segment directly based on Liu's code. Rewritten in C++ so that it may be 
// compiled in an MSVC environment
#define _SCL_SECURE_NO_WARNINGS // Issue with linterp
#pragma warning( disable : 4244 ) // warnings C4244 and C4267: size_t to int and int <-> _int64
#pragma warning( disable : 4267 )
#include <cmath>
#include <complex>
#include <fstream>
#include <valarray>
//#include <thread>
#include <mutex>
#include "../scatdb/refract/refract.hpp"
#include "../scatdb/refract/refractBase.hpp"
#include "../scatdb/zeros.hpp"
#include "../scatdb/units/units.hpp"
#include "../private/linterp.h"
#include "../scatdb/error.hpp"
#include "../scatdb/logging.hpp"

namespace {
	std::mutex m_setup;

	enum class hanelAmedium { WATER_RE, WATER_IM, ICE_RE, ICE_IM, NACL_RE, NACL_IM, SEASALT_RE, SEASALT_IM };
	std::shared_ptr<InterpMultilinear<1, double > > setupHanelA(hanelAmedium m)
	{
		std::lock_guard<std::mutex> lock(m_setup);
		static std::shared_ptr<InterpMultilinear<1, double > > res_water_re, res_water_im,
			res_ice_re, res_ice_im, res_nacl_re, res_nacl_im, res_seasalt_re, res_seasalt_im;
		if (!res_water_re) {

			const size_t nWvlengths = 90;
			const double Dwavelengths[nWvlengths] = { 0.2, 0.25, 0.3, 0.337, 0.4, 0.488, 0.515,
				0.55, 0.633, 0.694, 0.86, 1.06, 1.3, 1.536, 1.8, 2, 2.25, 2.5,
				2.7, 3, 3.2, 3.392, 3.5, 3.75, 4, 4.5, 5, 5.5, 6, 6.2, 6.5,
				7.2, 7.9, 8.2, 8.5, 8.7, 9, 9.2, 9.5, 9.8, 10, 10.591, 11,
				11.5, 12.5, 13, 14, 14.8, 15, 16.4, 17.2, 18, 18.5, 20, 21.3,
				22.5, 25, 27.9, 30, 35, 40, 45, 50, 55, 60, 65, 70, 80, 90,
				100, 110, 120, 135, 150, 165, 180, 200, 250, 300, 400, 500,
				750, 1000, 1500, 2000, 3000, 5000, 10000, 20000, 30000 };
			static std::vector<double > wavelengths(nWvlengths);
			for (size_t i = 0; i < nWvlengths; ++i) wavelengths[i] = Dwavelengths[i];
			const double water_re[nWvlengths] = { 1.396, 1.362, 1.349, 1.345, 1.339, 1.335,
				1.334, 1.333, 1.332, 1.331, 1.329, 1.326, 1.323, 1.318, 1.312, 1.306,
				1.292, 1.261, 1.188, 1.371, 1.478, 1.422, 1.4, 1.369, 1.351, 1.332,
				1.325, 1.298, 1.265, 1.363, 1.339, 1.312, 1.294, 1.286, 1.278, 1.272,
				1.262, 1.255, 1.243, 1.229, 1.218, 1.179, 1.153, 1.126, 1.123, 1.146,
				1.21, 1.258, 1.27, 1.346, 1.386, 1.423, 1.443, 1.48, 1.491, 1.506, 1.531,
				1.549, 1.551, 1.532, 1.519, 1.536, 1.587, 1.645, 1.703, 1.762, 1.821,
				1.92, 1.979, 2.037, 2.06, 2.082, 2.094, 2.106, 2.109, 2.113, 2.117,
				2.12, 2.121, 2.142, 2.177, 2.291, 2.437, 2.562, 2.705, 3.013, 3.627,
				4.954, 6.728, 7.682 };
			const double water_im[nWvlengths] = { 1.10E-07, 3.35E-08, 1.60E-08, 8.45E-09,
				1.86E-09, 9.69E-10, 1.18E-09, 1.96E-09, 1.46E-08, 3.05E-08, 3.29E-07,
				4.18E-06, 3.69E-05, 9.97E-05, 1.15E-04, 1.10E-03, 3.90E-04, 0.00174,
				0.019, 0.272, 0.0924, 0.0204, 0.0094, 0.0035, 0.0046, 0.0134, 0.0124,
				0.0116, 0.107, 0.088, 0.0392, 0.0321, 0.0339, 0.0351, 0.0367, 0.0379,
				0.0399, 0.0415, 0.0444, 0.0479, 0.0508, 0.0674, 0.0968, 0.142, 0.259,
				0.305, 0.37, 0.396, 0.402, 0.427, 0.429, 0.426, 0.421, 0.393, 0.379,
				0.37, 0.356, 0.339, 0.328, 0.336, 0.385, 0.449, 0.514, 0.551, 0.587,
				0.582, 0.576, 0.52, 0.49, 0.46, 0.44, 0.42, 0.41, 0.4, 0.41, 0.42,
				0.43, 0.46, 0.49, 0.53, 0.58, 0.65, 0.73, 0.96, 1.19, 1.59, 2.14, 2.79,
				2.87, 2.51 };
			//static std::vector<double > water_re(nWvlengths);
			//for (size_t i = 0; i < nWvlengths; ++i) water_re[i] = water_re[i], water_im[i]);
			const double ice_re[nWvlengths] = { 1.394, 1.351, 1.334, 1.326, 1.32, 1.313, 1.312,
				1.311, 1.308, 1.306, 1.303, 1.3, 1.295, 1.29, 1.282, 1.273, 1.256, 1.225,
				1.163, 1.045, 1.652, 1.51, 1.453, 1.391, 1.361, 1.34, 1.327, 1.299, 1.296,
				1.313, 1.32, 1.318, 1.313, 1.306, 1.291, 1.282, 1.269, 1.261, 1.245, 1.219,
				1.197, 1.098, 1.093, 1.176, 1.387, 1.472, 1.569, 1.579, 1.572, 1.531, 1.534,
				1.522, 1.51, 1.504, 1.481, 1.455, 1.414, 1.358, 1.325, 1.226, 1.202, 1.299,
				1.629, 1.767, 1.585, 1.748, 1.869, 1.903, 1.856, 1.832, 1.821, 1.819, 1.819,
				1.823, 1.829, 1.832, 1.827, 1.807, 1.795, 1.786, 1.783, 1.782, 1.781, 1.782,
				1.782, 1.782, 1.783, 1.784, 1.784, 1.784 };
			const double ice_im[nWvlengths] = { 1.50E-08, 8.60E-09, 5.50E-09, 4.50E-09, 2.71E-09,
				1.75E-09, 2.19E-09, 3.11E-09, 1.09E-08, 2.62E-08, 2.15E-07, 1.96E-06,
				1.32E-05, 6.10E-04, 1.13E-04, 1.61E-03, 2.13E-04, 7.95E-04, 0.00293, 0.429,
				0.283, 0.0401, 0.0161, 0.007, 0.01, 0.0287, 0.012, 0.0217, 0.0647, 0.0683,
				0.0559, 0.0544, 0.0479, 0.039, 0.0391, 0.04, 0.0429, 0.0446, 0.0459, 0.047,
				0.051, 0.131, 0.239, 0.36, 0.422, 0.389, 0.283, 0.191, 0.177, 0.125, 0.107,
				0.0839, 0.076, 0.067, 0.0385, 0.0291, 0.0299, 0.049, 0.065, 0.155, 0.344,
				0.601, 0.543, 0.42, 0.39, 0.49, 0.399, 0.235, 0.165, 0.139, 0.126, 0.12,
				0.108, 0.0962, 0.0846, 0.065, 0.0452, 0.0222, 0.0153, 0.0125, 0.0106, 0.008,
				0.0065, 0.0049, 0.004, 0.003, 0.0021, 0.0013, 7.90E-04, 5.90E-04 };
			//static std::vector<std::complex<double> > ice(nWvlengths);
			//for (size_t i = 0; i < nWvlengths; ++i) ice[i] = std::complex<double>(ice_re[i], ice_im[i]);
			const double nacl_re[nWvlengths] = { 1.79, 1.655, 1.607, 1.587, 1.567, 1.553, 1.55,
				1.547, 1.542, 1.539, 1.534, 1.531, 1.529, 1.528, 1.527, 1.527, 1.526, 1.525,
				1.525, 1.524, 1.524, 1.523, 1.523, 1.522, 1.522, 1.52, 1.519, 1.517, 1.515,
				1.515, 1.513, 1.51, 1.507, 1.505, 1.504, 1.503, 1.501, 1.5, 1.498, 1.496,
				1.495, 1.491, 1.488, 1.484, 1.476, 1.471, 1.462, 1.454, 1.451, 1.435, 1.425,
				1.414, 1.406, 1.382, 1.36, 1.33, 1.27, 1.17, 1.08, 0.78, 0.58, 0.27, 0.14,
				0.31, 4.52, 5.28, 3.92, 3.17, 2.87, 2.74, 2.64, 2.59, 2.54, 2.5, 2.48, 2.47,
				2.45, 2.44, 2.43, 2.43, 2.43, 2.43, 2.43, 2.43, 2.43, 2.43, 2.43, 2.43, 2.43, 2.43 };
			const double nacl_im[nWvlengths] = { 3.10E-09, 2.30E-09, 1.50E-09, 8.70E-10, 3.80E-10,
				1.10E-10, 4.90E-11, 6.80E-11, 1.10E-10, 1.50E-10, 2.40E-10, 3.50E-10,
				4.80E-10, 6.10E-10, 7.50E-10, 8.60E-10, 1.00E-09, 1.10E-09, 1.20E-09,
				1.40E-09, 1.50E-09, 1.60E-09, 1.65E-09, 1.80E-09, 1.80E-09, 1.80E-09,
				1.70E-09, 2.60E-09, 4.90E-09, 5.80E-09, 7.20E-09, 1.00E-08, 1.40E-08,
				1.50E-08, 1.60E-08, 1.70E-08, 1.90E-08, 2.00E-08, 3.00E-08, 4.40E-08,
				5.30E-08, 8.00E-08, 1.30E-07, 3.30E-07, 1.40E-06, 2.80E-06, 8.80E-06,
				2.30E-05, 2.70E-05, 7.60E-05, 1.30E-04, 2.00E-04, 2.90E-04, 6.20E-04,
				9.90E-04, 0.0014, 0.0035, 0.01, 0.026, 0.14, 0.66, 1.08, 1.99, 3.46, 6.94,
				0.761, 0.271, 0.123, 0.0968, 0.087, 0.079, 0.077, 0.072, 0.064, 0.056, 0.052,
				0.047, 0.041, 0.03, 0.027, 0.024, 0.012, 0.008, 0.0061, 0.0047, 0.0029,
				0.0024, 5.60E-04, 4.10E-04, 2.60E-04 };
			//static std::vector<std::complex<double> > nacl(nWvlengths);
			//for (size_t i = 0; i < nWvlengths; ++i) nacl[i] = std::complex<double>(nacl_re[i], nacl_im[i]);
			const double seasalt_re[nWvlengths] = { 1.51, 1.51, 1.51, 1.51, 1.5, 1.5, 1.5,
				1.5, 1.49, 1.49, 1.48, 1.47, 1.47, 1.46, 1.45, 1.45, 1.44, 1.43, 1.4,
				1.61, 1.49, 1.48, 1.48, 1.47, 1.48, 1.49, 1.47, 1.42, 1.41, 1.6, 1.46,
				1.42, 1.4, 1.42, 1.48, 1.6, 1.65, 1.61, 1.58, 1.56, 1.54, 1.5, 1.48,
				1.48, 1.42, 1.41, 1.41, 1.43, 1.45, 1.56, 1.74, 1.78, 1.77, 1.76, 1.76,
				1.76, 1.76, 1.77, 1.77, 1.76, 1.74, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
			const double seasalt_im[nWvlengths] = { 1.00E-04, 5.00E-06, 2.00E-06, 4.00E-07,
				3.00E-08, 2.00E-08, 1.00E-08, 1.00E-08, 2.00E-08, 1.00E-07, 3.00E-06,
				2.00E-04, 4.00E-04, 6.00E-04, 8.00E-04, 0.001, 0.002, 0.004, 0.007,
				0.01, 0.003, 0.002, 0.0016, 0.0014, 0.0014, 0.0014, 0.0025, 0.0036,
				0.011, 0.022, 0.005, 0.007, 0.013, 0.02, 0.026, 0.03, 0.028, 0.026,
				0.018, 0.016, 0.015, 0.014, 0.014, 0.014, 0.016, 0.018, 0.023, 0.03,
				0.035, 0.09, 0.12, 0.13, 0.135, 0.152, 0.165, 0.18, 0.205, 0.275, 0.3,
				0.5, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0 };
			//static std::vector<std::complex<double> > seasalt(nWvlengths);
			//for (size_t i = 0; i < nWvlengths; ++i) seasalt[i] = std::complex<double>(seasalt_re[i], seasalt_im[i]);

			static std::vector< std::vector<double>::iterator > grid_iter_list;
			grid_iter_list.push_back(wavelengths.begin());

			// the size of the grid in each dimension
			static std::array<size_t, 1> grid_sizes;
			grid_sizes[0] = wavelengths.size();
			// total number of elements
			static size_t num_elements = grid_sizes[0];// *grid_sizes[1];


			res_water_re = std::shared_ptr<InterpMultilinear<1, double > >
				(new InterpMultilinear<1, double >
				(grid_iter_list.begin(), grid_sizes.begin(), water_re, water_re + num_elements));
			res_water_im = std::shared_ptr<InterpMultilinear<1, double > >
				(new InterpMultilinear<1, double >
				(grid_iter_list.begin(), grid_sizes.begin(), water_im, water_im + num_elements));
			res_ice_re = std::shared_ptr<InterpMultilinear<1, double > >
				(new InterpMultilinear<1, double >
				(grid_iter_list.begin(), grid_sizes.begin(), ice_re, ice_re + num_elements));
			res_ice_im = std::shared_ptr<InterpMultilinear<1, double > >
				(new InterpMultilinear<1, double >
				(grid_iter_list.begin(), grid_sizes.begin(), ice_im, ice_im + num_elements));
			res_nacl_re = std::shared_ptr<InterpMultilinear<1, double > >
				(new InterpMultilinear<1, double >
				(grid_iter_list.begin(), grid_sizes.begin(), nacl_re, nacl_re + num_elements));
			res_nacl_im = std::shared_ptr<InterpMultilinear<1, double > >
				(new InterpMultilinear<1, double >
				(grid_iter_list.begin(), grid_sizes.begin(), nacl_im, nacl_im + num_elements));
			res_seasalt_re = std::shared_ptr<InterpMultilinear<1, double > >
				(new InterpMultilinear<1, double >
				(grid_iter_list.begin(), grid_sizes.begin(), seasalt_re, seasalt_re + num_elements));
			res_seasalt_im = std::shared_ptr<InterpMultilinear<1, double > >
				(new InterpMultilinear<1, double >
				(grid_iter_list.begin(), grid_sizes.begin(), seasalt_im, seasalt_im + num_elements));
		}
		if (m == hanelAmedium::WATER_RE) return res_water_re;
		else if (m == hanelAmedium::WATER_IM) return res_water_im;
		else if (m == hanelAmedium::ICE_RE) return res_ice_re;
		else if (m == hanelAmedium::ICE_IM) return res_ice_im;
		else if (m == hanelAmedium::NACL_RE) return res_nacl_re;
		else if (m == hanelAmedium::NACL_IM) return res_nacl_im;
		else if (m == hanelAmedium::SEASALT_RE) return res_seasalt_re;
		else if (m == hanelAmedium::SEASALT_IM) return res_seasalt_im;
		else SDBR_throw(scatdb::error::error_types::xUnimplementedFunction)
			.add<std::string>("Reason", "Bad enum input for hanelAmedium. "
				"This is not a "
				"case that the Hanel dielectric code can handle.");
		return nullptr;
	}
	enum class hanelBmedium { SAND_O_RE, SAND_O_IM, SAND_E_RE, SAND_E_IM, DUST_LIKE_RE, DUST_LIKE_IM };
	std::shared_ptr<InterpMultilinear<1, double > > setupHanelB(hanelBmedium m)
	{
		std::lock_guard<std::mutex> lock(m_setup);
		static std::shared_ptr<InterpMultilinear<1, double> > res_sand_o_re, res_sand_o_im,
			res_sand_e_re, res_sand_e_im, res_dust_re, res_dust_im;
		if (!res_sand_o_re) {

			const size_t nWvlengths = 68, nWvlengthsDust = 61;
			const double D_wavelengths[nWvlengths] = { 0.2, 0.25, 0.3, 0.337, 0.4, 0.488,
				0.515, 0.55, 0.633, 0.694, 0.86, 1.06, 1.3, 1.536, 1.8, 2, 2.25, 2.5,
				2.7, 3, 3.2, 3.392, 3.5, 3.75, 4, 4.5, 5, 5.5, 6, 6.2, 6.5, 7.2, 7.9,
				8.2, 8.5, 8.7, 9, 9.2, 9.5, 9.8, 10, 10.591, 11, 11.5, 12.5, 13, 14,
				14.8, 15, 16.4, 17.2, 18, 18.5, 20, 21.3, 22.5, 25, 27.9, 30, 35, 40,
				50, 60, 80, 100, 150, 200, 300 };
			static std::vector<double > wavelengths(nWvlengths);
			for (size_t i = 0; i < nWvlengths; ++i) wavelengths[i] = D_wavelengths[i];
			const double sand_o_re[nWvlengths] = { 1.665, 1.686, 1.677, 1.665, 1.656,
				1.667, 1.666, 1.664, 1.655, 1.65, 1.635, 1.627, 1.623, 1.62, 1.615,
				1.611, 1.607, 1.602, 1.597, 1.591, 1.585, 1.579, 1.576, 1.566, 1.555,
				1.535, 1.511, 1.466, 1.42, 1.389, 1.342, 1.161, 0.662, 0.175, 0.356,
				0.854, 0.221, 0.612, 4.274, 2.933, 2.615, 2.18, 2.015, 1.844, 1.564,
				2.119, 1.715, 1.727, 1.669, 1.163, 2.011, 1.584, 1.346, 0.145, 0.346,
				5.37, 0.946, 2.162, 3.446, 2.655, 2.491, 2.446, 2.388, 2.347, 2.326,
				2.31, 2.299, 2.296 };
			const double sand_o_im[nWvlengths] = { 0.147, 0.117, 0.09, 0.0778, 0.0335,
				0.0109, 0.00809, 0.00474, 5.22E-04, 5.61E-05, 2.46E-04, 1.92E-06,
				6.51E-07, 3.63E-07, 2.67E-07, 3.00E-07, 3.31E-07, 8.58E-07, 1.19E-06,
				2.60E-05, 7.13E-06, 6.78E-06, 8.66E-06, 5.75E-05, 7.10E-05, 4.86E-04,
				0.00539, 0.00533, 0.00656, 0.00764, 0.00571, 0.0124, 0.0922, 0.632,
				1.82, 2.19, 2.06, 4.07, 0.356, 0.0789, 0.0474, 0.0214, 0.0172, 0.019,
				2.02, 0.0515, 0.0394, 0.0381, 0.0348, 0.233, 0.655, 0.0912, 0.0603,
				0.886, 2.59, 1.35, 1.84, 0.382, 0.353, 0.0243, 0.0107, 0.00613, 0.00428,
				0.00494, 0.00187, 0.00143, 0.00121, 0.00106 };
			//static std::vector<std::complex<double> > sand_o(nWvlengths);
			//for (size_t i = 0; i < nWvlengths; ++i) sand_o[i] = std::complex<double>(sand_o_re[i], sand_o_im[i]);
			const double sand_e_re[nWvlengths] = { 1.665, 1.686, 1.677, 1.665, 1.656,
				1.667, 1.666, 1.664, 1.655, 1.65, 1.635, 1.627, 1.623, 1.62, 1.615, 1.611,
				1.607, 1.602, 1.597, 1.591, 1.585, 1.579, 1.576, 1.566, 1.555, 1.535,
				1.511, 1.466, 1.42, 1.389, 1.342, 1.16, 0.639, 0.155, 0.239, 1.557, 0.251,
				1.59, 3.733, 2.794, 2.533, 2.151, 2.003, 1.859, 1.305, 2.524, 1.762, 1.58,
				1.533, 0.955, 1.404, 0.489, 0.229, 1.389, 2.853, 2.698, 1.559, 4.4, 4.196,
				2.836, 2.606, 2.495, 2.436, 2.389, 2.37, 2.352, 2.347, 2.343 };
			const double sand_e_im[nWvlengths] = { 0.147, 0.117, 0.09, 0.0778, 0.0335,
				0.0109, 0.00809, 0.00474, 5.22E-04, 5.61E-05, 2.46E-04, 1.92E-06, 6.51E-07,
				3.63E-07, 2.67E-07, 3.00E-07, 3.31E-07, 1.09E-06, 1.59E-06, 8.69E-06,
				4.53E-06, 6.12E-06, 8.66E-06, 3.58E-05, 7.10E-05, 3.45E-04, 0.00427, 0.00503,
				0.00468, 0.00764, 0.00717, 0.0146, 0.0938, 0.693, 1.74, 0.709, 2.56, 5.85,
				0.207, 0.0629, 0.0405, 0.0198, 0.016, 0.0159, 0.123, 0.291, 0.0229, 0.0293,
				0.0333, 0.481, 0.153, 0.249, 1.1, 4.57, 0.448, 0.295, 0.154, 0.915, 1.41,
				0.0369, 0.0151, 0.00509, 0.00319, 0.00192, 0.0014, 8.24E-04, 5.88E-04, 3.43E-04 };
			//static std::vector<std::complex<double> > sand_e(nWvlengths);
			//for (size_t i = 0; i < nWvlengths; ++i) sand_e[i] = std::complex<double>(sand_e_re[i], sand_e_im[i]);
			const double dust_re[nWvlengthsDust] = { 1.53, 1.53, 1.53, 1.53, 1.53, 1.53,
				1.53, 1.53, 1.53, 1.53, 1.52, 1.52, 1.46, 1.4, 1.33, 1.26, 1.22, 1.18, 1.18,
				1.16, 1.22, 1.26, 1.28, 1.27, 1.26, 1.26, 1.25, 1.22, 1.15, 1.14, 1.13, 1.4,
				1.15, 1.13, 1.3, 1.4, 1.7, 1.72, 1.73, 1.74, 1.75, 1.62, 1.62, 1.59, 1.51,
				1.47, 1.52, 1.57, 1.57, 1.6, 1.63, 1.64, 1.64, 1.68, 1.77, 1.9, 1.97, 1.89,
				1.8, 1.9, 2.1 };
			const double dust_im[nWvlengthsDust] = { 0.07, 0.03, 0.008, 0.008, 0.008, 0.008,
				0.008, 0.008, 0.008, 0.008, 0.008, 0.008, 0.008, 0.008, 0.008, 0.008, 0.009,
				0.009, 0.013, 0.012, 0.01, 0.013, 0.011, 0.011, 0.012, 0.014, 0.016, 0.021,
				0.037, 0.039, 0.042, 0.055, 0.04, 0.074, 0.09, 0.1, 0.14, 0.15, 0.162, 0.162,
				0.162, 0.12, 0.105, 0.1, 0.09, 0.1, 0.085, 0.1, 0.1, 0.1, 0.1, 0.115, 0.12,
				0.22, 0.28, 0.28, 0.24, 0.32, 0.42, 0.5, 0.6 };
			//static std::vector<std::complex<double> > dust(nWvlengthsDust);
			//for (size_t i = 0; i < nWvlengthsDust; ++i) dust[i] = std::complex<double>(dust_re[i], dust_im[i]);

			static std::vector< std::vector<double>::iterator > grid_iter_list;
			grid_iter_list.push_back(wavelengths.begin());

			// the size of the grid in each dimension
			static std::array<size_t, 1> grid_sizes;
			grid_sizes[0] = wavelengths.size();
			// total number of elements
			static size_t num_elements = grid_sizes[0];// *grid_sizes[1];

			static std::array<size_t, 1> grid_sizes_small;
			grid_sizes_small[0] = nWvlengthsDust;
			// total number of elements
			static size_t num_elements_small = grid_sizes_small[0];// *grid_sizes[1];

			res_sand_o_re = std::shared_ptr<InterpMultilinear<1, double > >
				(new InterpMultilinear<1, double >
				(grid_iter_list.begin(), grid_sizes.begin(), sand_o_re, sand_o_re + num_elements));
			res_sand_e_re = std::shared_ptr<InterpMultilinear<1, double > >
				(new InterpMultilinear<1, double >
				(grid_iter_list.begin(), grid_sizes.begin(), sand_e_re, sand_e_re + num_elements));
			res_dust_re = std::shared_ptr<InterpMultilinear<1, double > >
				(new InterpMultilinear<1, double >
				(grid_iter_list.begin(), grid_sizes_small.begin(), dust_re, dust_re + num_elements_small));
			res_sand_o_im = std::shared_ptr<InterpMultilinear<1, double > >
				(new InterpMultilinear<1, double >
				(grid_iter_list.begin(), grid_sizes.begin(), sand_o_im, sand_o_im + num_elements));
			res_sand_e_im = std::shared_ptr<InterpMultilinear<1, double > >
				(new InterpMultilinear<1, double >
				(grid_iter_list.begin(), grid_sizes.begin(), sand_e_im, sand_e_im + num_elements));
			res_dust_im = std::shared_ptr<InterpMultilinear<1, double > >
				(new InterpMultilinear<1, double >
				(grid_iter_list.begin(), grid_sizes_small.begin(), dust_im, dust_im + num_elements_small));
		}
		if (m == hanelBmedium::SAND_O_RE) return res_sand_o_re;
		else if (m == hanelBmedium::SAND_O_IM) return res_sand_o_im;
		else if (m == hanelBmedium::SAND_E_RE) return res_sand_e_re;
		else if (m == hanelBmedium::SAND_E_IM) return res_sand_e_im;
		else if (m == hanelBmedium::DUST_LIKE_RE) return res_dust_re;
		else if (m == hanelBmedium::DUST_LIKE_IM) return res_dust_im;
		else SDBR_throw(scatdb::error::error_types::xUnimplementedFunction)
			.add<std::string>("Reason", "Bad enum input for hanelBmedium. "
				"This is not a "
				"case that the Hanel dielectric code can handle.");
		return nullptr;
	}
}

// Water complex refractive index
// from Liu's mcx.f
// LIEBE, HUFFORD AND MANABE, INT. J. IR & MM WAVES V.12, pp.659-675
//  (1991);  Liebe et al, AGARD Conf. Proc. 542, May 1993.
// Valid from 0 to 1000 GHz. freq in GHz, temp in K
void scatdb::refract::implementations::mWaterLiebe(double f, double t, std::complex<double> &m)
{
	if (f < 0 || f > 1000 || t > 273.15)
		SDBR_throw(scatdb::error::error_types::xModelOutOfRange)
		.add<double>("Frequency (GHz)", f)
		.add<double>("Temperature (K)", t)
		.add<std::string>("Reason", "Allowed freq. range (GHz) is (0,1000), and allowed temp. range is <= 273.15 K.");

	double theta1 = 1.0 - (300.0 / t);
	double eps0 = 77.66 - (103.3*theta1);
	double eps1 = .0671*eps0;
	double eps2 = 3.52;
	double fp = (316.*theta1 + 146.4)*theta1 + 20.20;
	double fs = 39.8*fp;
	using namespace std;
	complex<double> eps;
	eps = complex<double>(eps0 - eps1, 0) / complex<double>(1.0, f / fp)
		+ complex<double>(eps1 - eps2, 0) / complex<double>(1.0, f / fs)
		+ complex<double>(eps2, 0);
	m = sqrt(eps);
	SDBR_log("refract", scatdb::logging::DEBUG_2, 
		"mWaterLiebe result for freq: " << f << " temp " << t << " is " << m);
}

void scatdb::refract::implementations::mWaterFreshMeissnerWentz(double f, double tK, std::complex<double> &m)
{
	if (f < 0 || f > 500 || tK < 273.15)
		SDBR_throw(scatdb::error::error_types::xModelOutOfRange)
		.add<double>("Frequency (GHz)", f)
		.add<double>("Temperature (K)", tK)
		.add<std::string>("Reason", "Allowed freq. range (GHz) is (0,500), and temp. must be >= 273.15 K.");

	const double as[11] = {
		5.7230, 0.022379, -0.00071237, 5.0478,
		-0.070315, 0.00060059, 3.6143, 0.028841,
		0.13652, 0.0014825, 0.00024166
	};

	double tC = tK - 273.15;
	if (tC < -20 || tC > 40)
		SDBR_throw(scatdb::error::error_types::xModelOutOfRange)
		.add<double>("Frequency (GHz)", f)
		.add<double>("Temperature (K)", tK)
		.add<std::string>("Reason", "Allowed temp. range (C) is (-20,40)");


	// static dielectric constant for pure water (Stogryn)
	double es = (37088.6 - (82.168*tC)) / (tC + 421.854);

	double e1 = as[0] + (as[1] * tC) + (as[2] * tC*tC);
	double nu1 = (45 + tC) / (as[3] + (tC*as[4]) + (tC*tC*as[5]));
	double einf = as[6] + (tC*as[7]);
	double nu2 = (45 + tC) / (as[8] + (tC*as[9]) + (tC*tC*as[10]));
	double sigma = 0;
	// vacuum electric permittivity
	const double oneover2pie0 = 17.97510; // GHz m / S

	using namespace std;
	complex<double> eps;
	eps = complex<double>(es - e1, 0) / (complex<double>(1, f / nu1));
	eps += complex<double>(e1 - einf, 0) / (complex<double>(1, f / nu2));
	eps += complex<double>(einf, -sigma * oneover2pie0 / f); // -sigma / (f*2.*pi*e0));
	m = sqrt(eps);
	SDBR_log("refract", scatdb::logging::DEBUG_2, 
		"mWaterFreshMeissnerWentz result for freq: " << f << " GHz temp " << tK << " K is " << m);
}


void scatdb::refract::implementations::mIceMatzler(double f, double t, std::complex<double> &m)
{
	if (f < 0 || f > 1000 || t > 273.15)
		SDBR_throw(scatdb::error::error_types::xModelOutOfRange)
		.add<double>("Frequency (GHz)", f)
		.add<double>("Temperature (K)", t)
		.add<std::string>("Reason", "Allowed freq. range (GHz) is (0,1000), and allowed temp. range is <= 273.15 K.");

	double er = 0;
	if (t>243.0)
		er = 3.1884 + 9.1e-4*(t - 273.0);
	else
		er = 3.1611 + 4.3e-4*(t - 243.0);
	// Imaginary part
	double theta = 300.0 / (t)-1.0;
	double alpha = (0.00504 + 0.0062*theta)*exp(-22.1*theta);
	double dbeta = exp(-9.963 + 0.0372*(t - 273.16));
	const double B1 = 0.0207;
	const double B2 = 1.16e-11;
	const double b = 335;
	double betam = B1 / t*exp(b / t) / pow((exp(b / t) - 1.0), 2) + B2*pow(f, 2.0);
	double beta = betam + dbeta;
	double ei = alpha / f + beta*f;
	std::complex<double> e(er, -ei);
	m = sqrt(e);
	SDBR_log("refract", scatdb::logging::DEBUG_2,
		"mIceMatzler result for freq: " << f << " GHz and temp " << t << " K is " << m);
}

void scatdb::refract::implementations::mIceWarren(double f, double t, std::complex<double> &m)
{
	if (f < 0.167 || f > 8600 || t > 272.15 || t < 213.15)
		SDBR_throw(scatdb::error::error_types::xModelOutOfRange)
		.add<double>("Frequency (GHz)", f)
		.add<double>("Temperature (K)", t)
		.add<std::string>("Reason", "Allowed freq. range (GHz) is (0.167,8600), and allowed temp. range is 213 - 272 K.");

	// Warren table 2 is used for interpolation
	static bool setup = false;
	static std::vector<double> tempCs, wavelengths;
	static std::vector<double > vals_re, vals_im;
	static std::shared_ptr<InterpMultilinear<2, double > > interp_ML_warren_re, interp_ML_warren_im;
	auto setupWarren = [&]()
	{
		std::lock_guard<std::mutex> lock(m_setup);
		if (setup) return;

		tempCs.push_back(-1); tempCs.push_back(-5); tempCs.push_back(-20); tempCs.push_back(-60);
		wavelengths.reserve(62);
		vals_re.reserve(600); vals_im.reserve(600);

		// First wavelength is at tbl[0], last is at tbl[549]
		static const double tbl[] = {
			.1670, 1.8296, 8.30e-2, 1.8296, 8.30e-2, 1.8296, 8.30e-2, 1.8296, 8.30e-2,
			.1778, 1.8236, 6.90e-2, 1.8236, 6.90e-2, 1.8236, 6.90e-2, 1.8236, 6.90e-2,
			.1884, 1.8315, 5.70e-2, 1.8315, 5.70e-2, 1.8315, 5.70e-2, 1.8315, 5.70e-2,
			.1995, 1.8275, 4.56e-2, 1.8275, 4.56e-2, 1.8275, 4.56e-2, 1.8275, 4.45e-2,
			.2113, 1.8222, 3.79e-2, 1.8222, 3.79e-2, 1.8222, 3.79e-2, 1.8222, 3.55e-2,
			.2239, 1.8172, 3.14e-2, 1.8172, 3.14e-2, 1.8172, 3.14e-2, 1.8172, 2.91e-2,
			.2371, 1.8120, 2.62e-2, 1.8120, 2.62e-2, 1.8120, 2.62e-2, 1.8120, 2.44e-2,
			.2512, 1.8070, 2.24e-2, 1.8070, 2.24e-2, 1.8070, 2.19e-2, 1.8070, 1.97e-2,
			.2661, 1.8025, 1.96e-2, 1.8025, 1.96e-2, 1.8025, 1.88e-2, 1.8025, 1.67e-2,
			.2818, 1.7983, 1.76e-2, 1.7983, 1.76e-2, 1.7983, 1.66e-2, 1.7983, 1.40e-2,
			.2985, 1.7948, 1.67e-2, 1.7948, 1.67e-2, 1.7948, 1.54e-2, 1.7948, 1.26e-2,
			.3162, 1.7921, 1.62e-2, 1.7921, 1.60e-2, 1.7921, 1.47e-2, 1.7921, 1.08e-2,
			.3548, 1.7884, 1.55e-2, 1.7884, 1.50e-2, 1.7884, 1.35e-2, 1.7884, 8.90e-3,
			.3981, 1.7860, 1.47e-2, 1.7860, 1.40e-2, 1.7860, 1.25e-2, 1.7860, 7.34e-3,
			.4467, 1.7843, 1.39e-2, 1.7843, 1.31e-2, 1.7843, 1.15e-2, 1.7843, 6.40e-3,
			.5012, 1.7832, 1.32e-2, 1.7832, 1.23e-2, 1.7832, 1.06e-2, 1.7832, 5.60e-3,
			.5623, 1.7825, 1.25e-2, 1.7825, 1.15e-2, 1.7825, 9.77e-3, 1.7825, 5.00e-3,
			.6310, 1.7820, 1.18e-2, 1.7820, 1.08e-2, 1.7820, 9.01e-3, 1.7820, 4.52e-3,
			.7943, 1.7817, 1.06e-2, 1.7817, 9.46e-3, 1.7816, 7.66e-3, 1.7815, 3.68e-3,
			1.000, 1.7816, 9.54e-3, 1.7816, 8.29e-3, 1.7814, 6.52e-3, 1.7807, 2.99e-3,
			1.259, 1.7819, 8.56e-3, 1.7819, 7.27e-3, 1.7816, 5.54e-3, 1.7801, 2.49e-3,
			2.500, 1.7830, 6.21e-3, 1.7830, 4.91e-3, 1.7822, 3.42e-3, 1.7789, 1.55e-3,
			5.000, 1.7843, 4.49e-3, 1.7843, 3.30e-3, 1.7831, 2.10e-3, 1.7779, 9.61e-4,
			10.00, 1.7852, 3.24e-3, 1.7852, 2.22e-3, 1.7838, 1.29e-3, 1.7773, 5.95e-4,
			20.00, 1.7862, 2.34e-3, 1.7861, 1.49e-3, 1.7839, 7.93e-4, 1.7772, 3.69e-4,
			32.00, 1.7866, 1.88e-3, 1.7863, 1.14e-3, 1.7840, 5.70e-4, 1.7772, 2.67e-4,
			35.00, 1.7868, 1.74e-3, 1.7864, 1.06e-3, 1.7840, 5.35e-4, 1.7772, 2.51e-4,
			40.00, 1.7869, 1.50e-3, 1.7865, 9.48e-4, 1.7840, 4.82e-4, 1.7772, 2.29e-4,
			45.00, 1.7870, 1.32e-3, 1.7865, 8.50e-4, 1.7840, 4.38e-4, 1.7772, 2.11e-4,
			50.00, 1.7870, 1.16e-3, 1.7865, 7.66e-4, 1.7840, 4.08e-4, 1.7772, 1.96e-4,
			60.00, 1.7871, 8.80e-4, 1.7865, 6.30e-4, 1.7839, 3.50e-4, 1.7772, 1.73e-4,
			70.00, 1.7871, 6.95e-4, 1.7865, 5.20e-4, 1.7838, 3.20e-4, 1.7772, 1.55e-4,
			90.00, 1.7872, 4.64e-4, 1.7865, 3.84e-4, 1.7837, 2.55e-4, 1.7772, 1.31e-4,
			111.0, 1.7872, 3.40e-4, 1.7865, 2.96e-4, 1.7837, 2.12e-4, 1.7772, 1.13e-4,
			120.0, 1.7872, 3.11e-4, 1.7865, 2.70e-4, 1.7837, 2.00e-4, 1.7772, 1.06e-4,
			130.0, 1.7872, 2.94e-4, 1.7865, 2.52e-4, 1.7837, 1.86e-4, 1.7772, 9.90e-5,
			140.0, 1.7872, 2.79e-4, 1.7865, 2.44e-4, 1.7837, 1.75e-4, 1.7772, 9.30e-5,
			150.0, 1.7872, 2.70e-4, 1.7865, 2.36e-4, 1.7837, 1.66e-4, 1.7772, 8.73e-5,
			160.0, 1.7872, 2.64e-4, 1.7865, 2.30e-4, 1.7837, 1.56e-4, 1.7772, 8.30e-5,
			170.0, 1.7872, 2.58e-4, 1.7865, 2.28e-4, 1.7837, 1.49e-4, 1.7772, 7.87e-5,
			180.0, 1.7872, 2.52e-4, 1.7865, 2.25e-4, 1.7837, 1.44e-4, 1.7772, 7.50e-5,
			200.0, 1.7872, 2.49e-4, 1.7865, 2.20e-4, 1.7837, 1.35e-4, 1.7772, 6.83e-5,
			250.0, 1.7872, 2.54e-4, 1.7865, 2.16e-4, 1.7837, 1.21e-4, 1.7772, 5.60e-5,
			290.0, 1.7872, 2.64e-4, 1.7865, 2.17e-4, 1.7837, 1.16e-4, 1.7772, 4.96e-5,
			320.0, 1.7872, 2.74e-4, 1.7865, 2.20e-4, 1.7837, 1.16e-4, 1.7772, 4.55e-5,
			350.0, 1.7872, 2.89e-4, 1.7665, 2.25e-4, 1.7837, 1.17e-4, 1.7772, 4.21e-5,
			380.0, 1.7872, 3.05e-4, 1.7865, 2.32e-4, 1.7837, 1.20e-4, 1.7772, 3.91e-5,
			400.0, 1.7872, 3.15e-4, 1.7865, 2.39e-4, 1.7837, 1.23e-4, 1.7772, 3.76e-5,
			450.0, 1.7872, 3.46e-4, 1.7865, 2.60e-4, 1.7837, 1.32e-4, 1.7772, 3.40e-5,
			500.0, 1.7872, 3.82e-4, 1.7865, 2.86e-4, 1.7837, 1.44e-4, 1.7772, 3.10e-5,
			600.0, 1.7872, 4.62e-4, 1.7865, 3.56e-4, 1.7837, 1.68e-4, 1.7772, 2.64e-5,
			640.0, 1.7872, 5.00e-4, 1.7865, 3.83e-4, 1.7837, 1.80e-4, 1.7772, 2.51e-5,
			680.0, 1.7872, 5.50e-4, 1.7865, 4.15e-4, 1.7837, 1.90e-4, 1.7772, 2.43e-5,
			720.0, 1.7872, 5.95e-4, 1.7865, 4.45e-4, 1.7837, 2.09e-4, 1.7772, 2.39e-5,
			760.0, 1.7872, 6.47e-4, 1.7865, 4.76e-4, 1.7837, 2.16e-4, 1.7772, 2.37e-5,
			800.0, 1.7872, 6.92e-4, 1.7865, 5.08e-4, 1.7837, 2.29e-4, 1.7772, 2.38e-5,
			840.0, 1.7872, 7.42e-4, 1.7865, 5.40e-4, 1.7837, 2.40e-4, 1.7772, 2.40e-5,
			900.0, 1.7872, 8.20e-4, 1.7865, 5.86e-4, 1.7837, 2.60e-4, 1.7772, 2.46e-5,
			1000., 1.7872, 9.70e-4, 1.7865, 6.78e-4, 1.7837, 2.92e-4, 1.7772, 2.66e-5,
			2000., 1.7872, 1.95e-3, 1.7865, 1.28e-3, 1.7837, 6.10e-4, 1.7772, 4.45e-5,
			5000., 1.7872, 5.78e-3, 1.7865, 3.55e-3, 1.7840, 1.02e-3, 1.7772, 8.70e-5,
			8600., 1.7880, 9.70e-3, 1.7872, 5.60e-3, 1.7845, 1.81e-3, 1.7780, 1.32e-4
		};

		for (size_t i = 0; i < 558; ++i)
		{
			if (i % 9 == 0) wavelengths.push_back(tbl[i]);
			else if (i % 9 == 1 || i % 9 == 3 || i % 9 == 5 || i % 9 == 7) vals_re.push_back(tbl[i]);
			else vals_im.push_back(tbl[i]);
		}

		static std::vector< std::vector<double>::iterator > grid_iter_list;
		grid_iter_list.push_back(tempCs.begin());
		grid_iter_list.push_back(wavelengths.begin());

		// the size of the grid in each dimension
		static std::array<size_t, 2> grid_sizes;
		grid_sizes[0] = tempCs.size();
		grid_sizes[1] = wavelengths.size();
		// total number of elements
		static size_t num_elements = grid_sizes[0] * grid_sizes[1];

		// construct the interpolator. the last two arguments are pointers to the underlying data
		interp_ML_warren_re = std::shared_ptr<InterpMultilinear<2, double > >(new InterpMultilinear<2, double >
			(grid_iter_list.begin(), grid_sizes.begin(), vals_re.data(), vals_re.data() + num_elements));
		interp_ML_warren_im = std::shared_ptr<InterpMultilinear<2, double > >(new InterpMultilinear<2, double >
			(grid_iter_list.begin(), grid_sizes.begin(), vals_im.data(), vals_im.data() + num_elements));

		setup = true;
	};


	setupWarren();

	// interpolate one value
	double wvlen = scatdb::units::conv_spec("GHz", "mm").convert(f);
	std::array<double, 2> args = { wvlen, t - 273.15 };
	m = std::complex<double>(interp_ML_warren_re->interp(args.begin()),
		-1.0 * interp_ML_warren_im->interp(args.begin()));
	SDBR_log("refract", scatdb::logging::DEBUG_2, "mIceWarren result for freq: " << f << " GHz, temp " << t << " K is " << m);
}


void scatdb::refract::implementations::mWaterHanel(double lambda, std::complex<double> &m)
{
	if (lambda < 0.2 || lambda > 30000)
		SDBR_throw(scatdb::error::error_types::xModelOutOfRange)
		.add<double>("Wavelength (um)", lambda)
		.add<std::string>("Reason", "Allowed wavelength range (um) is (0.2,30000).");

	array<double, 1> args = { lambda };
	m = std::complex<double>(setupHanelA(hanelAmedium::WATER_RE)->interp(args),
		-1.0 * setupHanelA(hanelAmedium::WATER_IM)->interp(args));
	SDBR_log("refract", scatdb::logging::DEBUG_2, "mWaterHanel result for lambda: " << lambda << " is " << m);
}

void scatdb::refract::implementations::mIceHanel(double lambda, std::complex<double> &m)
{
	if (lambda < 0.2 || lambda > 30000)
		SDBR_throw(scatdb::error::error_types::xModelOutOfRange)
		.add<double>("Wavelength (um)", lambda)
		.add<std::string>("Reason", "Allowed wavelength range (um) is (0.2,30000).");

	array<double, 1> args = { lambda };
	m = std::complex<double>(setupHanelA(hanelAmedium::ICE_RE)->interp(args),
		-1.0 * setupHanelA(hanelAmedium::ICE_IM)->interp(args));
	SDBR_log("refract", scatdb::logging::DEBUG_2, "mIceHanel result for lambda: " << lambda << " is " << m);
}

void scatdb::refract::implementations::mNaClHanel(double lambda, std::complex<double> &m)
{
	if (lambda < 0.2 || lambda > 30000)
		SDBR_throw(scatdb::error::error_types::xModelOutOfRange)
		.add<double>("Wavelength (um)", lambda)
		.add<std::string>("Reason", "Allowed wavelength range (um) is (0.2,30000).");

	//double wvlen = scatdb::units::conv_spec("GHz", "mm").convert(f);
	array<double, 1> args = { lambda };
	m = std::complex<double>(setupHanelA(hanelAmedium::NACL_RE)->interp(args),
		-1.0 * setupHanelA(hanelAmedium::NACL_IM)->interp(args));
	SDBR_log("refract", scatdb::logging::DEBUG_2, "mNaClHanel result for lambda: " << lambda << " is " << m);
}

void scatdb::refract::implementations::mSeaSaltHanel(double lambda, std::complex<double> &m)
{
	if (lambda < 0.2 || lambda > 30000)
		SDBR_throw(scatdb::error::error_types::xModelOutOfRange)
		.add<double>("Wavelength (um)", lambda)
		.add<std::string>("Reason", "Allowed wavelength range (um) is (0.2,30000).");
	//double wvlen = scatdb::units::conv_spec("GHz", "mm").convert(f);
	array<double, 1> args = { lambda };
	m = std::complex<double>(setupHanelA(hanelAmedium::SEASALT_RE)->interp(args),
		-1.0 * setupHanelA(hanelAmedium::SEASALT_IM)->interp(args));
	SDBR_log("refract", scatdb::logging::DEBUG_2, "mSeaSaltHanel result for lambda: " << lambda << " is " << m);
}

void scatdb::refract::implementations::mDustHanel(double lambda, std::complex<double> &m)
{
	if (lambda < 0.2 || lambda > 300)
		SDBR_throw(scatdb::error::error_types::xModelOutOfRange)
		.add<double>("Wavelength (um)", lambda)
		.add<std::string>("Reason", "Allowed wavelength range (um) is (0.2,300).");
	//double wvlen = scatdb::units::conv_spec("GHz", "mm").convert(f);
	array<double, 1> args = { lambda };
	m = std::complex<double>(setupHanelB(hanelBmedium::DUST_LIKE_RE)->interp(args),
		-1.0 * setupHanelB(hanelBmedium::DUST_LIKE_IM)->interp(args));
	SDBR_log("refract", scatdb::logging::DEBUG_2, "mDustHanel result for lambda: " << lambda << " is " << m);
}

void scatdb::refract::implementations::mSandOHanel(double lambda, std::complex<double> &m)
{
	if (lambda < 0.2 || lambda > 300)
		SDBR_throw(scatdb::error::error_types::xModelOutOfRange)
		.add<double>("Wavelength (um)", lambda)
		.add<std::string>("Reason", "Allowed wavelength range (um) is (0.2,300).");
	//double wvlen = scatdb::units::conv_spec("GHz", "mm").convert(f);
	array<double, 1> args = { lambda };
	m = std::complex<double>(setupHanelB(hanelBmedium::SAND_O_RE)->interp(args),
		-1.0 * setupHanelB(hanelBmedium::SAND_O_IM)->interp(args));
	SDBR_log("refract", scatdb::logging::DEBUG_2, "mSandOHanel result for lambda: " << lambda << " is " << m);
}

void scatdb::refract::implementations::mSandEHanel(double lambda, std::complex<double> &m)
{
	if (lambda < 0.2 || lambda > 300)
		SDBR_throw(scatdb::error::error_types::xModelOutOfRange)
		.add<double>("Wavelength (um)", lambda)
		.add<std::string>("Reason", "Allowed wavelength range (um) is (0.2,300).");
	//double wvlen = scatdb::units::conv_spec("GHz", "mm").convert(f);
	array<double, 1> args = { lambda };
	m = std::complex<double>(setupHanelB(hanelBmedium::SAND_E_RE)->interp(args),
		-1.0 * setupHanelB(hanelBmedium::SAND_E_IM)->interp(args));
	SDBR_log("refract", scatdb::logging::DEBUG_2, "mSandEHanel result for lambda: " << lambda << " is " << m);
}

