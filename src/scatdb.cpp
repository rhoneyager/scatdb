#include "../scatdb/defs.hpp"
#include <memory>
#include <string>
#include <iostream>
#include <tuple>
#include "../scatdb/scatdb.hpp"
#include "../scatdb/lowess.hpp"
#include "../scatdb/error.hpp"
//#include "../../spline/spline.hpp"

namespace scatdb {
	db::db() {}
	db::~db() {}
	scatdb_base::scatdb_base() {}
	scatdb_base::~scatdb_base() {}
	std::shared_ptr<const db::data_stats> db::getStats() const {
		if (!this->pStats)
			this->pStats = db::data_stats::generate(this);
		return this->pStats;
	}
	db::data_stats::data_stats() : count(0) {}
	db::data_stats::~data_stats() {}

	std::shared_ptr<const db> db::sort(
			db::data_entries::data_entries_floats xaxis) const {
		std::shared_ptr<db> res(new db);
			SDBR_throw(::scatdb::error::error_types::xUnimplementedFunction);

		/*
		// Convert from eigen arrays into vectors
		std::vector<double> ar, freq, temp, aeff, md, cabs, cbk, cext, csca, g;
		std::vector<int> ft;

		auto convertCol = [&](int col, std::vector<double> &out, bool dolog) {
			auto blk = floatMat.cast<float>().block(0,col,floatMat.rows(),1);
			out.insert(out.begin(), blk.data(), blk.data() + floatMat.rows());
			if (dolog) {
				for (size_t i=0; i< out.size(); ++i)
					out[i] = log10(out[i]);
			}
		};
		auto convertColInt = [&](int col, std::vector<int> &out) {
			auto blk = intMat.cast<int>().block(0,col,intMat.rows(),1);
			out.insert(out.begin(), blk.data(), blk.data() + intMat.rows());
		};
		convertColInt(data_entries::SDBR_FLAKETYPE, ft);
		convertCol(data_entries::SDBR_FREQUENCY_GHZ, freq, false);
		convertCol(data_entries::SDBR_TEMPERATURE_K, temp, false);
		convertCol(data_entries::SDBR_AEFF_UM, aeff, false);
		convertCol(data_entries::SDBR_MAX_DIMENSION_MM, md, false);
		convertCol(data_entries::SDBR_CABS_M, cabs, false);
		convertCol(data_entries::SDBR_CBK_M, cbk, false);
		convertCol(data_entries::SDBR_CEXT_M, cext, false);
		convertCol(data_entries::SDBR_CSCA_M, csca, false);
		convertCol(data_entries::SDBR_G, g, false);
		convertCol(data_entries::SDBR_AS_XY, ar, false);

		// Another slow operation to convert everything to tuples, sort based on x axis,
		// and then convert back to the vector forms.
		typedef std::tuple<int,double,double,double,double,double,double,double,double,double,double> sInner;
		std::vector< sInner > vsort;
		for (size_t i=0; i < aeff.size(); ++i)
			vsort.push_back( sInner (
				ft[i], freq[i], temp[i], aeff[i], md[i], cabs[i], cbk[i], cext[i], csca[i], g[i], ar[i]));
		if (xaxis == db::data_entries::SDBR_AEFF_UM)
			std::sort(vsort.begin(), vsort.end(), [](
			sInner lhs, sInner rhs) {
					if (std::get<3>(lhs) != std::get<3>(rhs))
						return std::get<3>(lhs) < std::get<3>(rhs);
					return std::get<4>(lhs) < std::get<4>(rhs);
				});
		else
			std::sort(vsort.begin(), vsort.end(), [](
			sInner lhs, sInner rhs) {
					if (std::get<4>(lhs) != std::get<4>(rhs))
						return std::get<4>(lhs) < std::get<4>(rhs);
					return std::get<3>(lhs) < std::get<3>(rhs);
				});
		for (size_t i=0; i < vsort.size(); ++i) {
			ft[i] = std::get<0>(vsort[i]);
			freq[i] = std::get<1>(vsort[i]);
			temp[i] = std::get<2>(vsort[i]);
			aeff[i] = std::get<3>(vsort[i]);
			md[i] = std::get<4>(vsort[i]);
			cabs[i] = std::get<5>(vsort[i]);
			cbk[i] = std::get<6>(vsort[i]);
			cext[i] = std::get<7>(vsort[i]);
			csca[i] = std::get<8>(vsort[i]);
			g[i] = std::get<9>(vsort[i]);
			ar[i] = std::get<10>(vsort[i]);

		}

		FloatMatType nfm;
		IntMatType nfi;
		nfm.resize(vsort.size(), floatMat.cols());
		nfi.resize(vsort.size(), intMat.cols());
		//nfm = floatMat;
		//nfi = intMat;

		nfm.fill(-999);
		nfi.fill(-999);

		//res->intMat.resize(intMat.rows(), intMat.cols());

		auto revertCol = [&](int col, const std::vector<double> &in, bool islog) {
			auto blk = nfm.block(0,col,nfm.rows(),1);
			for (size_t i=0; i<(size_t) nfm.rows(); ++i) {
				if (!islog)
					blk(i,0) = (float) in[i];
				else
					blk(i,0) = pow(10.f,(float) in[i]);
			}
		};
		auto revertColInt = [&](int col, const std::vector<int> &in) {
			auto blk = nfi.block(0,col,nfi.rows(),1);
			for (size_t i=0; i<(size_t)nfi.rows(); ++i) {
				blk(i,0) = (int) in[i];
			}
		};
		revertCol(data_entries::SDBR_AS_XY, ar, false);
		revertCol(data_entries::SDBR_FREQUENCY_GHZ, freq, false);
		revertCol(data_entries::SDBR_TEMPERATURE_K, temp, false);
		revertCol(data_entries::SDBR_AEFF_UM, aeff, false);
		revertCol(data_entries::SDBR_MAX_DIMENSION_MM, md, false);
		revertCol(data_entries::SDBR_CABS_M, cabs, false);
		revertCol(data_entries::SDBR_CBK_M, cbk, false);
		revertCol(data_entries::SDBR_CEXT_M, cext, false);
		revertCol(data_entries::SDBR_CSCA_M, csca, false);
		revertCol(data_entries::SDBR_G, g, false);
		revertColInt(data_entries::SDBR_FLAKETYPE, ft);

		res->floatMat = nfm;
		res->intMat = nfi;
		*/
		return res;
	}

	/*
	std::shared_ptr<const db> db::interpolate(
			db::data_entries::SDBR_data_entries_floats xaxis) const {
		std::shared_ptr<db> res(new db);
		// First, get the stats. Want min and max values for effective radius.
		auto stats = this->getStats();
		double minRad = stats->floatStats(data_entries::SDBR_S_MIN,xaxis),
			   maxRad = stats->floatStats(data_entries::SDBR_S_MAX,xaxis);

		double low = 0, high = 0;
		if (xaxis == db::data_entries::SDBR_AEFF_UM) {
			low = (double) ((int) (minRad/10.)) * 10.;
			high = (double) ((int) (maxRad/10.)+1) * 10.;
		} else if (xaxis == db::data_entries::SDBR_MAX_DIMENSION_MM) {
			low = (double) (((int) (minRad*10.)) / 10);
			high = (double) (((int) (maxRad*10.)+1) / 10);
		}

		// Convert from eigen arrays into vectors
		std::vector<double> aeff, cabs, cbk, cext, csca, g,
			rcabs, rcbk, rcext, rcsca, rg,
			icabs, icbk, icext, icsca, ig,
			rw, residuals, lx, lcabs, lcbk, lcext, lcsca, lg;


		auto convertCol = [&](int col, std::vector<double> &out, bool dolog) {
			auto blk = floatMat.cast<float>().block(0,col,floatMat.rows(),1);
			out.insert(out.begin(), blk.data(), blk.data() + floatMat.rows());
			if (dolog) {
				for (size_t i=0; i< out.size(); ++i)
					out[i] = log10(out[i]);
			}
		};
		if (xaxis == data_entries::SDBR_AEFF_UM)
			convertCol(data_entries::SDBR_AEFF_UM, aeff, false);
		else
			convertCol(data_entries::SDBR_MAX_DIMENSION_MM, aeff, false);
		convertCol(data_entries::SDBR_CABS_M, cabs, false);
		convertCol(data_entries::SDBR_CBK_M, cbk, false);
		convertCol(data_entries::SDBR_CEXT_M, cext, false);
		convertCol(data_entries::SDBR_CSCA_M, csca, false);
		convertCol(data_entries::SDBR_G, g, false);

		// Another slow operation to convert everything to tuples, sort based on x axis,
		// and then convert back to the vector forms.
		typedef std::tuple<double,double,double,double,double,double> sInner;
		std::vector< sInner > vsort;
		for (size_t i=0; i < aeff.size(); ++i)
			vsort.push_back( sInner (
				aeff[i], cabs[i], cbk[i], cext[i], csca[i], g[i]));
		std::sort(vsort.begin(), vsort.end(), [](
			sInner lhs, sInner rhs) {
					return std::get<0>(lhs) < std::get<0>(rhs);
				});
		for (size_t i=0; i < vsort.size(); ++i) {
			aeff[i] = std::get<0>(vsort[i]);
			cabs[i] = std::get<1>(vsort[i]);
			cbk[i] = std::get<2>(vsort[i]);
			cext[i] = std::get<3>(vsort[i]);
			csca[i] = std::get<4>(vsort[i]);
			g[i] = std::get<5>(vsort[i]);
		}

		const int sz = (int)(high-low+1)/10;
		lx.reserve(sz);
		lcabs.reserve(sz);
		lcbk.reserve(sz);
		lcext.reserve(sz);
		lcsca.reserve(sz);
		lg.reserve(sz);
		double span = 10;
		if (xaxis == data_entries::SDBR_MAX_DIMENSION_MM)
			span = 0.1;

		for (double x = low; x <= high; x += span) {
			lx.push_back(x);
		}

		// Call interpolation engine here
		//double *spline_cubic_set ( int n, double t[], double y[], int ibcbeg, 
		//	double ybcbeg, int ibcend, double ybcend );
		//double spline_cubic_val ( int n, double t[], double y[], double ypp[], 
		//	double tval, double *ypval, double *yppval );
		auto splineit = [&](std::vector<double> &ys, std::vector<double> &iys) {
			// Really inefficient filtering... Spline code cannot have duplicate x values
			std::vector<double> fxs, fys;
			fxs.reserve(aeff.size()); fys.reserve(aeff.size());
			double last = -9999;
			for (size_t i=0; i<aeff.size(); ++i) {
				double x = aeff[i];
				double y = ys[i];
				if (abs((last/x)-1) < 0.000001 || last >= x) {
					// Already tested.
					//std::cerr << "Ignoring x " << x << " with last " << last << std::endl;
					continue;
				}
				fxs.push_back(x);
				fys.push_back(y);
				last = x;
				//std::cerr << x << std::endl;
			}
			//
			double *aeffpp = spline_cubic_set(
				(int) fxs.size(), fxs.data(), fys.data(),
				0, 0, 0, 0);


			for (double x = low; x <= high; x += span) {
				double ypval = 0, yppval = 0;
				double val = spline_cubic_val((int) fxs.size(), fxs.data(), fys.data(), aeffpp,
					x, &ypval, &yppval);
				iys.push_back(val);
			}
			delete[] aeffpp;
		};

		splineit(cabs, lcabs);
		splineit(cbk, lcbk);
		splineit(cext, lcext);
		splineit(csca, lcsca);
		splineit(g, lg);

		FloatMatType nfm;
		IntMatType nfi;
		nfm.resize(lx.size(), floatMat.cols());
		nfi.resize(lx.size(), intMat.cols());
		//nfm = floatMat;
		//nfi = intMat;

		nfm.fill(-999);
		nfi.fill(-999);

		//res->intMat.resize(intMat.rows(), intMat.cols());

		auto revertCol = [&](int col, const std::vector<double> &in, bool islog) {
			auto blk = nfm.block(0,col,nfm.rows(),1);
			for (size_t i=0; i<(size_t)nfm.rows(); ++i) {
				if (!islog)
					blk(i,0) = (float) in[i];
				else
					blk(i,0) = pow(10.f,(float) in[i]);
			}
		};
		if (xaxis == db::data_entries::SDBR_AEFF_UM)
			revertCol(data_entries::SDBR_AEFF_UM, lx, false);
		else
			revertCol(data_entries::SDBR_MAX_DIMENSION_MM, lx, false);
		revertCol(data_entries::SDBR_CABS_M, lcabs, false);
		revertCol(data_entries::SDBR_CBK_M, lcbk, false);
		revertCol(data_entries::SDBR_CEXT_M, lcext, false);
		revertCol(data_entries::SDBR_CSCA_M, lcsca, false);
		revertCol(data_entries::SDBR_G, lg, false);

		res->floatMat = nfm;
		res->intMat = nfi;
		return res;
	}
	*/

	std::shared_ptr<const db> db::regress(
		db::data_entries::data_entries_floats xaxis,
		double f, uint64_t nsteps, double delta) const {
		std::shared_ptr<db> res(new db);

		// First, get the stats. Want min and max values for effective radius.
		auto stats = this->getStats();
		double minRad = stats->floatStats(data_entries::SDBR_S_MIN,xaxis),
			   maxRad = stats->floatStats(data_entries::SDBR_S_MAX,xaxis);

		double low = 0, high = 0;
		if (xaxis == db::data_entries::SDBR_AEFF_UM) {
			low = (double) ((int) (minRad/10.)) * 10.;
			high = (double) ((int) (maxRad/10.)+1) * 10.;
		} else if (xaxis == db::data_entries::SDBR_MAX_DIMENSION_MM) {
			low = (double) (((int) (minRad*10.)) / 10);
			high = (double) (((int) (maxRad*10.)+1) / 10);
		}

		// Convert from eigen arrays into vectors
		std::vector<double> aeff, cabs, cbk, cext, csca, g,
			rcabs, rcbk, rcext, rcsca, rg,
			icabs, icbk, icext, icsca, ig,
			rw, residuals, lx, lcabs, lcbk, lcext, lcsca, lg;
		auto convertCol = [&](int col, std::vector<double> &out, bool dolog) {
			auto blk = floatMat.cast<float>().block(0,col,floatMat.rows(),1);
			out.insert(out.begin(), blk.data(), blk.data() + floatMat.rows());
			if (dolog) {
				for (size_t i=0; i< out.size(); ++i)
					out[i] = log10(out[i]);
			}
		};
		if (xaxis == db::data_entries::SDBR_AEFF_UM)
			convertCol(data_entries::SDBR_AEFF_UM, aeff, false);
		else
			convertCol(data_entries::SDBR_MAX_DIMENSION_MM, aeff, false);
		convertCol(data_entries::SDBR_CABS_M, cabs, true);
		convertCol(data_entries::SDBR_CBK_M, cbk, true);
		convertCol(data_entries::SDBR_CEXT_M, cext, true);
		convertCol(data_entries::SDBR_CSCA_M, csca, true);
		convertCol(data_entries::SDBR_G, g, false);

		lowess(aeff, cabs, f, (long) nsteps, delta, rcabs, rw, residuals);
		lowess(aeff, cbk, f, (long) nsteps, delta, rcbk, rw, residuals);
		lowess(aeff, cext, f, (long) nsteps, delta, rcext, rw, residuals);
		lowess(aeff, csca, f, (long) nsteps, delta, rcsca, rw, residuals);
		lowess(aeff, g, f, (long) nsteps, delta, rg, rw, residuals);

		FloatMatType nfm;
		IntMatType nfi;
		//nfm.resize(lx.size(), floatMat.cols());
		//nfi.resize(lx.size(), intMat.cols());
		nfm = floatMat;
		nfi = intMat;

		nfm.block(0,0,nfm.rows(),nfm.cols()).fill(-999);
		nfi.block(0,0,nfi.rows(),nfi.cols()).fill(-999);

		//res->intMat.resize(intMat.rows(), intMat.cols());

		auto revertCol = [&](int col, const std::vector<double> &in, bool islog) {
			auto blk = nfm.block(0,col,nfm.rows(),1);
			for (size_t i=0; i<(size_t)nfm.rows(); ++i) {
				if (!islog)
					blk(i,0) = (float) in[i];
				else
					blk(i,0) = pow(10.f,(float) in[i]);
			}
		};
		if (xaxis == db::data_entries::SDBR_AEFF_UM)
			revertCol(data_entries::SDBR_AEFF_UM, aeff, false);
		else
			revertCol(data_entries::SDBR_MAX_DIMENSION_MM, aeff, false);
		revertCol(data_entries::SDBR_CABS_M, rcabs, true);
		revertCol(data_entries::SDBR_CBK_M, rcbk, true);
		revertCol(data_entries::SDBR_CEXT_M, rcext, true);
		revertCol(data_entries::SDBR_CSCA_M, rcsca, true);
		revertCol(data_entries::SDBR_G, rg, false);

		res->floatMat = nfm;
		res->intMat = nfi;
		return res;
	}

	template<>
	DLEXPORT_SDBR const char* db::data_entries::stringify<float>(uint64_t ev) {
#define _tostr(a) #a
#define tostr(a) _tostr(a)
#define check(a) if (ev == a) return tostr(a);
		check(SDBR_FREQUENCY_GHZ); check(SDBR_TEMPERATURE_K); check(SDBR_AEFF_UM);
		check(SDBR_MAX_DIMENSION_MM); check(SDBR_CABS_M); check(SDBR_CBK_M); check(SDBR_CEXT_M);
		check(SDBR_CSCA_M); check(SDBR_G); check(SDBR_AS_XY);

		return "";
#undef check
#undef tostr
#undef _tostr
	}

	template<>
	DLEXPORT_SDBR const char* db::data_entries::stringify<int>(uint64_t ev) {
#define _tostr(a) #a
#define tostr(a) _tostr(a)
#define check(a) if (ev == a) return tostr(a);
		check(SDBR_FLAKETYPE);
		return "";
#undef check
#undef tostr
#undef _tostr
	}

	DLEXPORT_SDBR const char* db::data_entries::stringifyStats(uint64_t ev) {
		//S_MIN, S_MAX, MEDIAN, MEAN, SD, SKEWNESS, KURTOSIS
#define _tostr(a) #a
#define tostr(a) _tostr(a)
#define check(a) if (ev == a) return tostr(a);
		check(SDBR_S_MIN); check(SDBR_S_MAX); check(SDBR_MEDIAN);
		check(SDBR_MEAN); check(SDBR_SD); check(SDBR_SKEWNESS); check(SDBR_KURTOSIS);

		return "";
#undef check
#undef tostr
#undef _tostr
	}

	DLEXPORT_SDBR const char* db::data_entries::getCategoryDescription(uint64_t catid) {
		const char 
			*hexl = "Liu [2004] Long hexagonal column l/d=4",
			*hexs = "Liu [2004] Short hexagonal column l/d=2",
			*hexb = "Liu [2004] Block hexagonal column l/d=1",
			*hexf = "Liu [2004] Thick hexagonal plate l/d=0.2",
			*hexp = "Liu [2004] Thin hexagonal plate l/d=0.05",
			*ros3 = "Liu [2008] 3-bullet rosette",
			*ros4 = "Liu [2008] 4-bullet rosette",
			*ros5 = "Liu [2008] 5-bullet rosette",
			*ros6 = "Liu [2008] 6-bullet rosette",
			*sstr = "Liu [2008] sector-like snowflake",
			*sden = "Liu [2008] dendrite snowflake",
			*nlh13 = "Nowell, Liu and Honeyager [2013] Rounded Aggregates",
			*hln16obl = "Honeyager, Liu and Nowell [2016] Oblate Aggregates",
			*hln16pro = "Honeyager, Liu and Nowell [2016] Prolate Aggregates",
			*ori = "Ori et al. [2014] SAM model",
			*ls1 = "Leinonen and Szyrmer [2015] - A 0.0",
			*ls2 = "Leinonen and Szyrmer [2015] - A 0.1",
			*ls3 = "Leinonen and Szyrmer [2015] - A 0.2",
			*ls4 = "Leinonen and Szyrmer [2015] - A 0.5",
			*ls5 = "Leinonen and Szyrmer [2015] - A 1.0",
			*ls6 = "Leinonen and Szyrmer [2015] - A 2.0",
			*ls7 = "Leinonen and Szyrmer [2015] - B 0.1",
			*ls8 = "Leinonen and Szyrmer [2015] - B 0.2",
			*ls9 = "Leinonen and Szyrmer [2015] - B 0.5",
			*ls10 = "Leinonen and Szyrmer [2015] - B 1.0",
			*ls11 = "Leinonen and Szyrmer [2015] - B 2.0",
			*ls12 = "Leinonen and Szyrmer [2015] - C",
			*tcfda = "Tyynela and Chandrasekhar [2014] - Fernlike dendrite aggregates",
			*tcna = "Tyynela and Chandrasekhar [2014] - Needle aggregates",
			*tcb6 = "Tyynela and Chandrasekhar [2014] - 6-bullet rosette aggregates",
			*tcsa = "Tyynela and Chandrasekhar [2014] - Stellar dendrite aggregates",
			*kuo = "Kuo et al. [2016+] - Kuo's model",
			*hw14 = "Hogan and Westbrook [2014] - Westbrook aggregates, random orientation SSRG",
			*unknown = "Unknown";
		if (catid == 0) return hexl;
		else if (catid == 1) return hexs;
		else if (catid == 2) return hexb;
		else if (catid == 3) return hexf;
		else if (catid == 4) return hexp;
		else if (catid == 5) return ros3;
		else if (catid == 6) return ros4;
		else if (catid == 7) return ros5;
		else if (catid == 8) return ros6;
		else if (catid == 9) return sstr;
		else if (catid == 10) return sden;
		else if (catid == 20) return nlh13;
		else if (catid == 21) return hln16obl;
		else if (catid == 22) return hln16pro;
		else if (catid == 30) return ori;
		else if (catid == 40) return ls1;
		else if (catid == 41) return ls2;
		else if (catid == 42) return ls3;
		else if (catid == 43) return ls4;
		else if (catid == 44) return ls5;
		else if (catid == 45) return ls6;
		else if (catid == 46) return ls7;
		else if (catid == 47) return ls8;
		else if (catid == 48) return ls9;
		else if (catid == 49) return ls10;
		else if (catid == 50) return ls11;
		else if (catid == 51) return ls12;
		else if (catid == 60) return tcfda;
		else if (catid == 61) return tcna;
		else if (catid == 62) return tcb6;
		else if (catid == 63) return tcsa;
		else if (catid == 70) return kuo;
		else if (catid == 80) return hw14;
		else return unknown;
	}

	void db::data_stats::print(std::ostream &out) const {
		out << "Stats for " << count << " entries." << std::endl;
		out << "\tMIN\tMAX\tMEDIAN\tMEAN\tSD\tSKEWNESS\tKURTOSIS\n";
		for (int i=0; i<data_entries::SDBR_NUM_DATA_ENTRIES_FLOATS; ++i) {
			out << data_entries::stringify<float>(i) << std::endl;
			out << "\t" << floatStats(data_entries::SDBR_S_MIN,i) << "\t"
				<< floatStats(data_entries::SDBR_S_MAX,i) << "\t"
				<< floatStats(data_entries::SDBR_MEDIAN,i) << "\t"
				<< floatStats(data_entries::SDBR_MEAN,i) << "\t"
				<< floatStats(data_entries::SDBR_SD,i) << "\t"
				<< floatStats(data_entries::SDBR_SKEWNESS,i) << "\t"
				<< floatStats(data_entries::SDBR_KURTOSIS,i) << std::endl;
		}
	}
}

