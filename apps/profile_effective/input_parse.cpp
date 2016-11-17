#define _SCL_SECURE_NO_WARNINGS
#include "parser.hpp"
#include "../../scatdb/error.hpp"
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>


/// Internal namespace for the reader parsers
namespace {
	namespace qi = boost::spirit::qi;
	namespace ascii = boost::spirit::ascii;
	namespace phoenix = boost::phoenix;

	/** \brief Parses space-separated shapefile entries.
	**/
	template <typename Iterator>
	bool parse_float_entries(Iterator first, Iterator last, std::vector<float>& v)
	{
		using qi::double_;
		using qi::float_;
		using qi::long_;
		using qi::phrase_parse;
		using qi::_1;
		using ascii::space;
		using phoenix::push_back;

		bool r = phrase_parse(first, last,

			//  Begin grammar
			(
				// *long_[push_back(phoenix::ref(v), _1)]
				*float_
				)
			,
			//  End grammar

			space, v);

		if (first != last) // fail if we did not get a full match
			return false;
		return r;
	}

}

namespace scatdb {
	namespace profiles {
		forward_set_p forward_conc_table::import(const char* filename) {
			using namespace std;
			std::shared_ptr<std::vector<forward_p> > res(new std::vector<forward_p>);

			// Input structure goes:
			// Description line
			// Bin midpoints, Bin endpoints, Bin width
			// Temp, Concentrations in each bin
			ifstream in(filename);
			string lin;
			// Start with six lines of junk
			for (size_t i = 0; i < 6; ++i) std::getline(in, lin);
			std::vector<float> vmids, vends, vwidths;
			std::vector<float> vline;
			// Midpoints
			std::getline(in, lin);
			parse_float_entries(lin.cbegin(), lin.cend(), vmids);
			// Endpoints
			std::getline(in, lin); std::getline(in, lin);
			parse_float_entries(lin.cbegin(), lin.cend(), vends);
			// Bin width
			std::getline(in, lin); std::getline(in, lin);
			parse_float_entries(lin.cbegin(), lin.cend(), vwidths);
			std::shared_ptr<tbl_t> dat(new tbl_t);
			dat->resize((int)vmids.size(), dat->cols());
			auto dmids = dat->block(0, defs::BIN_MID, dat->rows(), 1);
			std::copy_n(vmids.data(), vmids.size(), dmids.data());
			// vmids is one element larger than vmids...
			auto dmins = dat->block(0, defs::BIN_LOWER, dat->rows(), 1);
			std::copy_n(vends.data(), vmids.size(), dmins.data());
			auto dmaxs = dat->block(0, defs::BIN_UPPER, dat->rows(), 1);
			std::copy_n(vends.data() + 1, vmids.size(), dmaxs.data());

			auto dwid = dat->block(0, defs::BID_WIDTH, dat->rows(), 1);
			std::copy_n(vwidths.data(), vmids.size(), dwid.data());

			auto dconc_base = dat->block(0, defs::CONCENTRATION, dat->rows(), 1);
			dconc_base.setZero();
			// Five more lines of junk
			for (size_t i = 0; i < 5; ++i) std::getline(in, lin);
			// Temp and concentrations
			for (int i = 0; i < 5; ++i) {
				std::getline(in, lin);
				vline.clear();
				parse_float_entries(lin.cbegin(), lin.cend(), vline);
				std::shared_ptr<forward_conc_table> leg(new forward_conc_table);
				leg->tempC = vline.at(0);
				std::shared_ptr<tbl_t> dnew(new tbl_t);
				*dnew = *dat;
				auto dconc = dnew->block(0, defs::CONCENTRATION, dat->rows(), 1);
				std::copy_n(vline.data() + 1, vmids.size(), dconc.data());
				leg->data = dnew;
				switch (i) {
				case 0:
				case 1:
					leg->pt = defs::particle_types::AGG_IRREG;
					break;
				case 2:
				case 3:
					leg->pt = defs::particle_types::AGG_NEEDLE;
					break;
				case 4:
					leg->pt = defs::particle_types::AGG_COMPACT;
					break;
				default:
					SDBR_throw(scatdb::error::error_types::xArrayOutOfBounds);
				}
				leg->pt;

				res->push_back(leg);
			}
			return res;
		}
		forward_p forward_conc_table::readText(const char* filename) {
			std::shared_ptr<forward_conc_table> res(new forward_conc_table);
			SDBR_throw(scatdb::error::error_types::xUnimplementedFunction);
			return res;

		}
	}
}