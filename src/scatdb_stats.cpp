#include "../scatdb/defs.hpp"
#include <memory>
#include <tuple>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/accumulators/statistics/covariance.hpp>
#include <boost/accumulators/statistics/density.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/kurtosis.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/median.hpp>
#include <boost/accumulators/statistics/moment.hpp>
#include <boost/accumulators/statistics/skewness.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <boost/accumulators/statistics/variates/covariate.hpp>
#include "../scatdb/scatdb.hpp"

namespace scatdb {
	std::shared_ptr<const db::data_stats> db::data_stats::generate(const db* src) {
		std::shared_ptr<db::data_stats> res(new db::data_stats);
		if (!src) return res;
		using namespace boost::accumulators;
		// NOTE: Using doubles instead of floats to avoid annoying boost internal library warnings
		typedef accumulator_set<double, boost::accumulators::stats <
			tag::min,
			tag::max,
			tag::mean,
			tag::median,
			tag::skewness,
			tag::kurtosis,
			tag::variance
		> > acc_type;

		// Push the data to the accumulator functions.
		std::vector<acc_type> accs(data_entries::SDBR_NUM_DATA_ENTRIES_FLOATS);
		for (int i = 0; i<src->floatMat.rows(); ++i) {
			for (int j = 0; j<src->floatMat.cols(); ++j) {
				if (src->floatMat(i, j) < -900) continue;
				accs[j]((double)src->floatMat(i, j));
			}
		}
		res->count = (int)src->floatMat.rows();

		// Extract the parameters
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
		//typedef Eigen::Matrix<float, data_entries::SDBR_NUM_DATA_ENTRIES_STATS,
		//	data_entries::SDBR_NUM_DATA_ENTRIES_FLOATS> StatsFloatType;
		for (int j = 0; j<data_entries::SDBR_NUM_DATA_ENTRIES_FLOATS; ++j) {
			res->floatStats(data_entries::SDBR_S_MIN, j) = (float)boost::accumulators::min(accs[j]);
			res->floatStats(data_entries::SDBR_S_MAX, j) = (float)boost::accumulators::max(accs[j]);
			res->floatStats(data_entries::SDBR_MEAN, j) = (float)boost::accumulators::mean(accs[j]);
			res->floatStats(data_entries::SDBR_MEDIAN, j) = (float)boost::accumulators::median(accs[j]);
			res->floatStats(data_entries::SDBR_SKEWNESS, j) = (float)boost::accumulators::skewness(accs[j]);
			res->floatStats(data_entries::SDBR_KURTOSIS, j) = (float)boost::accumulators::kurtosis(accs[j]);
			res->floatStats(data_entries::SDBR_SD, j) = (float)std::pow(boost::accumulators::variance(accs[j]), 0.5);
		}
		return res;
	}

}

