#pragma warning( disable : 4996 ) // -D_SCL_SECURE_NO_WARNINGS
#pragma warning( disable : 4244 ) // 'argument': conversion from 'std::streamsize' to 'int', possible loss of data - boost::copy
#include "../scatdb/defs.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <fstream>
//#include "../scatdb/macros.hpp"
#include "../scatdb/shape/shape.hpp"
#include "../scatdb/shape/shapeForwards.hpp"
#include "../private/shapeIOtext.hpp"
#include "../private/shapeBackend.hpp"
#include "../scatdb/error.hpp"
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

/// Internal namespace for the reader parsers
namespace {
	namespace qi = boost::spirit::qi;
	namespace ascii = boost::spirit::ascii;
	namespace phoenix = boost::phoenix;

	/** \brief Parses space-separated shapefile entries.
	**/
	template <typename Iterator>
	bool parse_shapefile_entries(Iterator first, Iterator last, std::vector<long>& v)
	{
		using qi::double_;
		using qi::long_;
		using qi::phrase_parse;
		using qi::_1;
		using ascii::space;
		using phoenix::push_back;

		bool r = phrase_parse(first, last,

			//  Begin grammar
			(
				// *long_[push_back(phoenix::ref(v), _1)]
				*long_
				)
			,
			//  End grammar

			space, v);

		if (first != last) // fail if we did not get a full match
			return false;
		return r;
	}

	/// Used in quickly printing shapefile
	template <typename OutputIterator, typename Container>
	bool print_shapefile_entries(OutputIterator& sink, Container const& v)
	{
		using boost::spirit::karma::long_;
		using boost::spirit::karma::repeat;
		using boost::spirit::karma::generate;
		//using boost::spirit::karma::generate_delimited;
		using boost::spirit::ascii::space;

		bool r = generate(
			sink,                           // destination: output iterator
			*(
				//repeat(7)()
				'\t' << long_ << '\t' << // point id
				long_ << '\t' << long_ << '\t' << long_ << '\t' << // point coordinates
				long_ << '\t' << long_ << '\t' << long_ << '\n' // dielectric
				),
			//space,                          // the delimiter-generator
			v                               // the data to output 
		);
		return r;
	}

	template <typename OutputIterator, typename Container>
	bool print_shapefile_pts(OutputIterator& sink, Container const& v)
	{
		using boost::spirit::karma::long_;
		using boost::spirit::karma::repeat;
		using boost::spirit::karma::generate;
		//using boost::spirit::karma::generate_delimited;
		using boost::spirit::ascii::space;

		bool r = generate(
			sink,                           // destination: output iterator
			*(
				//repeat(7)()
				long_ << '\t' << long_ << '\t' << long_ << '\n'
				),
			//space,                          // the delimiter-generator
			v                               // the data to output 
		);
		return r;
	}
}

namespace scatdb
{
	namespace plugins
	{
		namespace builtin
		{
			namespace shape {
				std::shared_ptr<::scatdb::shape::shape> readDDSCAT(const char* in)
				{
					std::shared_ptr<::scatdb::shape::shape> s = ::scatdb::shape::shape::generate();
					std::string str(in);
					std::string desc;
					std::shared_ptr<::scatdb::shape::shapeStorage_t> data(
						new ::scatdb::shape::shapeStorage_t);
					std::shared_ptr<::scatdb::shape::shapeHeaderStorage_t> hdr
						(new ::scatdb::shape::shapeHeaderStorage_t);
					size_t headerEnd = 0, numPoints = 0;
					readHeader(str.c_str(), desc, numPoints, hdr, headerEnd);
					data->resize((int)numPoints, ::scatdb::shape::backends::NUM_SHAPECOLS);
					readTextContents(str.c_str(), headerEnd, data);
					//auto ing = scatdb::generateIngest();
					/// \todo Change ingest to show absolute path
					//ing->sources.push_back(opts->getVal<std::string>("filename", "Unknown file"));
					//s->setIngestInformation(ing);
					s->setDescription(desc);
					s->setHeader(hdr);
					s->setPoints(data);
					return s;
				}
				void readHeader(const char* in, std::string &desc, size_t &np,
					std::shared_ptr<::scatdb::shape::shapeHeaderStorage_t> hdr,
					size_t &headerEnd)
				{
					using namespace std;

					// Do header processing using istreams.
					// The previous method used strings, but this didn't work with compressed reads.
					//size_t &pend = headerEnd;
					const char* pend = in;
					const char* pstart = in;

					// The header is seven lines long
					for (size_t i = 0; i < 7; i++)
					{
						pstart = pend;
						pend = strchr(pend, '\n');
						pend++; // Get rid of the newline
								//pend = in.find_first_of("\n", pend+1);
						string lin(pstart, pend - pstart - 1);
						if (*(lin.rbegin()) == '\r') lin.pop_back();
						//std::getline(in,lin);

						size_t posa = 0, posb = 0;
						Eigen::Array3f *v = nullptr;
						switch (i)
						{
						case 0: // Title line
							desc = lin;
							break;
						case 1: // Number of dipoles
						{
							// Seek to first nonspace character
							posa = lin.find_first_not_of(" \t\n", posb);
							// Find first space after this position
							posb = lin.find_first_of(" \t\n", posa);
							size_t len = posb - posa;
							string s = lin.substr(posa, len);
							np = boost::lexical_cast<size_t>(s);
							//np = macros::m_atoi<size_t>(&(lin.data()[posa]), len);
						}
						break;
						case 6: // Junk line
						default:
							break;
						case 2: // a1
						case 3: // a2
						case 4: // d
						case 5: // x0
								// These all have the same structure. Read in three doubles, then assign.
						{
							Eigen::Array3f v;
							for (size_t j = 0; j < 3; j++)
							{
								// Seek to first nonspace character
								posa = lin.find_first_not_of(" \t\n,", posb);
								// Find first space after this position
								posb = lin.find_first_of(" \t\n,", posa);
								size_t len = posb - posa;
								string s = lin.substr(posa, len);
								v(j) = boost::lexical_cast<float>(s);
								//v(j) = macros::m_atof<float>(&(lin.data()[posa]), len);
							}
							hdr->block<3, 1>(0, i - 2) = v;
							
						}
						break;
						}
					}

					headerEnd = (pend - in) / sizeof(char);
				}
				void readTextContents(const char *iin, size_t headerEnd,
					std::shared_ptr<::scatdb::shape::shapeStorage_t> data)
				{
					using namespace std;
					
					//Eigen::Vector3f crdsm, crdsi; // point location and diel entries
					const char* pa = &iin[headerEnd];
					const char* pb = strchr(pa + 1, '\0');

					std::vector<long> parser_vals; //(numPoints*8);
					size_t numPoints = (size_t)data->rows();
					parser_vals.reserve(numPoints * 8);
					parse_shapefile_entries(pa, pb, parser_vals);

					if (numPoints == 0) SDBR_throw(error::error_types::xBadInput)
						.add<std::string>("Reason", "Header indicates no dipoles.");
					if (parser_vals.size() == 0) SDBR_throw(error::error_types::xBadInput)
						.add<std::string>("Reason", "Unable to parse dipoles.");
					if (parser_vals.size() < (size_t) ((data->rows() - 1) * 7))
						SDBR_throw(error::error_types::xBadInput)
						.add<std::string>("Reason", "When reading shapefile, "
							"header dipoles do not match the number in the file.");

					for (size_t i = 0; i < numPoints; ++i)
					{
						// First field truly is a dummy variable. No correclation with point ordering at all.
						//size_t pIndex = parser_vals[index].at(7 * i) - 1;
						size_t pIndex = 7 * i;
						auto crdsm = data->block<1, scatdb::shape::backends::NUM_SHAPECOLS>(i, 0);
						for (size_t j = 0; j < 7; j++) // TODO: rewrite using eigen?
						{
							float val = (float)parser_vals.at(pIndex + j);
							crdsm(j) = val;
						}
					}
					
				}
				std::shared_ptr<::scatdb::shape::shape> readTextRaw(const char *iin)
				{
					using namespace std;
					auto res = scatdb::shape::shape::generate();
					//Eigen::Vector3f crdsm, crdsi; // point location and diel entries
					const char* pa = iin;
					const char* pb = strchr(pa + 1, '\0');
					const char* firstLineEnd = strchr(pa + 1, '\n');
					// Attempt to guess the number of points based on the number of lines in the file.
					int numPoints = std::count(pa, pb, '\n');
					std::vector<long> parser_vals, firstLineVals; //(numPoints*8);
					parser_vals.reserve(numPoints * 8);
					parse_shapefile_entries(pa, pb, parser_vals);
					parse_shapefile_entries(pa, firstLineEnd, firstLineVals);

					size_t numCols = firstLineVals.size();
					bool good = false;
					if ((numCols == 3)) good = true;
					if (!good) SDBR_throw(error::error_types::xBadInput)
						.add<std::string>("Reason", "Unable to interpret input. Number of columns is not supported.")
						.add<size_t>("numCols", numCols)
						.add<std::string>("first-line", std::string(pa, firstLineEnd - pa))
						.add<int>("numberOfPoints", numPoints);

					if (parser_vals.size() == 0) SDBR_throw(error::error_types::xBadInput)
						.add<std::string>("Reason", "Unable to parse dipoles.");
					
					if (numCols == 3) {
						scatdb::shape::shapePointsOnly_t pts;
						pts.resize(numPoints, 3);
						for (size_t i = 0; i < numPoints; ++i)
						{
							// First field truly is a dummy variable. No correclation with point ordering at all.
							//size_t pIndex = parser_vals[index].at(7 * i) - 1;
							size_t pIndex = 3 * i;
							auto crdsm = pts.block<1, 3>(i, 0);
							for (size_t j = 0; j < 3; j++) 
							{
								float val = (float)parser_vals.at(pIndex + j);
								crdsm(j) = val;
							}
						}

						res->setPoints(pts);
					}

					return res;
				}

				void writeDDSCAT(const std::string &filename, ::scatdb::shape::shape_ptr p)
				{
					using namespace std;
					std::ofstream out(filename.c_str());

					out << p->getDescription() << endl;
					out << p->numPoints() << "\t= Number of lattice points" << endl;
					auto hdr = p->getHeader();
					using namespace scatdb::shape::backends;
					out << (*hdr)(A1, 0) << "\t" << (*hdr)(A1, 1) << "\t" << (*hdr)(A1, 2);
					out << "\t= target vector a1 (in TF)" << endl;
					out << (*hdr)(A2, 0) << "\t" << (*hdr)(A2, 1) << "\t" << (*hdr)(A2, 2);
					out << "\t= target vector a2 (in TF)" << endl;
					out << (*hdr)(D, 0) << "\t" << (*hdr)(D, 1) << "\t" << (*hdr)(D, 2);
					out << "\t= d_x/d  d_y/d  d_x/d  (normally 1 1 1)" << endl;
					out << (*hdr)(X0, 0) << "\t" << (*hdr)(X0, 1) << "\t" << (*hdr)(X0, 2);
					out << "\t= X0(1-3) = location in lattice of target origin" << endl;
					out << "\tNo.\tix\tiy\tiz\tICOMP(x, y, z)" << endl;
					//size_t i = 1;

					std::vector<long> oi(p->numPoints() * 7);
					auto pts = p->getPoints();
					for (size_t j = 0; j < p->numPoints(); j++)
					{
						auto it = pts->block<1, 7>(j, 0);
						oi[j * 7 + 0] = (long)(it)(0);
						oi[j * 7 + 1] = (long)(it)(1);
						oi[j * 7 + 2] = (long)(it)(2);
						oi[j * 7 + 3] = (long)(it)(3);
						oi[j * 7 + 4] = (long)(it)(4);
						oi[j * 7 + 5] = (long)(it)(5);
						oi[j * 7 + 6] = (long)(it)(6);
					}

					std::string generated;
					std::back_insert_iterator<std::string> sink(generated);
					if (!print_shapefile_entries(sink, oi))
					{
						SDBR_throw(error::error_types::xOtherError)
							.add<std::string>("Reason", "Somehow unable to print the shape points properly.");
					}
					out << generated;
				}
				void writeTextRaw(const std::string &filename, ::scatdb::shape::shape_ptr p)
				{
					using namespace std;
					std::ofstream out(filename.c_str());
					std::vector<long> oi(p->numPoints() * 3);
					scatdb::shape::shapePointsOnly_t pts;
					p->getPoints(pts);
					for (size_t j = 0; j < p->numPoints(); j++)
					{
						auto it = pts.block<1, 3>(j, 1);
						oi[j * 7 + 0] = (long)(it)(0);
						oi[j * 7 + 1] = (long)(it)(1);
						oi[j * 7 + 2] = (long)(it)(2);
					}

					std::string generated;
					std::back_insert_iterator<std::string> sink(generated);
					if (!print_shapefile_pts(sink, oi))
					{
						SDBR_throw(error::error_types::xOtherError)
							.add<std::string>("Reason", "Somehow unable to print the shape points properly.");
					}
					out << generated;
				}

				std::shared_ptr<::scatdb::shape::shape> readTextFile(
					const std::string &filename) {
					// Open the file and copy to a string. Check the first few lines to see if any
					// alphanumeric characters are present. If there are, treat it as a DDSCAT file.
					// Otherwise, treat as a raw text file.
					std::ifstream in(filename.c_str());
					std::ostringstream so;
					boost::iostreams::copy(in, so);
					std::string s = so.str();

					auto end = s.find_first_of("\n\0");
					if (std::string::npos == s.find_first_not_of("0123456789. \t\n", 0, end)) {
						return readDDSCAT(s.c_str());
					}
					else {
						return readTextRaw(s.c_str());
					}
				}

				/*
				void shapefile::recalcStats()
				{
					using namespace std;
					using namespace boost::accumulators;
					accumulator_set<float, boost::accumulators::stats<tag::mean, tag::min, tag::max> > m_x, m_y, m_z;

					for (size_t i = 0; i < numPoints; i++)
					{
						auto pt = latticePts.block<1, 3>(i, 0);
						//auto Npt = latticePtsNorm.block<1, 3>(i, 0);
						//Npt = pt.array().transpose() - means;
						m_x(pt(0));
						m_y(pt(1));
						m_z(pt(2));
					}
					mins(0) = boost::accumulators::min(m_x);
					mins(1) = boost::accumulators::min(m_y);
					mins(2) = boost::accumulators::min(m_z);

					maxs(0) = boost::accumulators::max(m_x);
					maxs(1) = boost::accumulators::max(m_y);
					maxs(2) = boost::accumulators::max(m_z);

					means(0) = boost::accumulators::mean(m_x);
					means(1) = boost::accumulators::mean(m_y);
					means(2) = boost::accumulators::mean(m_z);

					rehash();
				}
				*/
			}
		}
	}
}
