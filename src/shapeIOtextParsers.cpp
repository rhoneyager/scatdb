#pragma warning( disable : 4996 ) // -D_SCL_SECURE_NO_WARNINGS
#pragma warning( disable : 4244 ) // 'argument': conversion from 'std::streamsize' to 'int', possible loss of data - boost::copy
#include "../Ryan_Scat/defs.hpp"
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
#include "../Ryan_Scat/macros.hpp"
#include "../Ryan_Scat/shape.hpp"
#include "../Ryan_Scat/shapeForwards.hpp"
#include "../Ryan_Scat/shapeIO.hpp"
#include "../private/shapeIOtext.hpp"
#include "../private/shapeBackend.hpp"
#include "../Ryan_Scat/ingest.hpp"
#include "../Ryan_Scat/error.hpp"
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
}

namespace Ryan_Scat
{
	namespace plugins
	{
		namespace builtin
		{
			namespace shape {
				void readDDSCAT(std::shared_ptr<::Ryan_Scat::shape::shape> s,
					std::istream &in, std::shared_ptr<registry::options> opts)
				{
					std::ostringstream so;
					boost::iostreams::copy(in, so);
					std::string str = so.str();
					std::string desc;
					std::shared_ptr<::Ryan_Scat::shape::shapeStorage_t> data(
						new ::Ryan_Scat::shape::shapeStorage_t);
					std::shared_ptr<::Ryan_Scat::shape::shapeHeaderStorage_t> hdr
						(new ::Ryan_Scat::shape::shapeHeaderStorage_t);
					size_t headerEnd = 0, numPoints = 0;
					readHeader(str.c_str(), desc, numPoints, hdr, headerEnd);
					data->resize((int)numPoints, ::Ryan_Scat::shape::backends::NUM_SHAPECOLS);
					readTextContents(str.c_str(), headerEnd, data);
					auto ing = Ryan_Scat::generateIngest();
					/// \todo Change ingest to show absolute path
					ing->sources.push_back(opts->getVal<std::string>("filename", "Unknown file"));
					s->setIngestInformation(ing);
					s->setDescription(desc);
					s->setHeader(hdr);
					s->setPoints(data);
				}
				void readHeader(const char* in, std::string &desc, size_t &np,
					std::shared_ptr<::Ryan_Scat::shape::shapeHeaderStorage_t> hdr,
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
							np = macros::m_atoi<size_t>(&(lin.data()[posa]), len);
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
								v(j) = macros::m_atof<float>(&(lin.data()[posa]), len);
							}
							hdr->block<3, 1>(0, i - 2) = v;
							
						}
						break;
						}
					}

					headerEnd = (pend - in) / sizeof(char);
				}
				void readTextContents(const char *iin, size_t headerEnd,
					std::shared_ptr<::Ryan_Scat::shape::shapeStorage_t> data)
				{
					using namespace std;
					
					//Eigen::Vector3f crdsm, crdsi; // point location and diel entries
					const char* pa = &iin[headerEnd];
					const char* pb = strchr(pa + 1, '\0');

					std::vector<long> parser_vals; //(numPoints*8);
					size_t numPoints = (size_t)data->rows();
					parser_vals.reserve(numPoints * 8);
					parse_shapefile_entries(pa, pb, parser_vals);

					if (numPoints == 0) RSthrow(error::error_types::xBadInput)
						.add<std::string>("Reason", "Header indicates no dipoles.");
					if (parser_vals.size() == 0) RSthrow(error::error_types::xBadInput)
						.add<std::string>("Reason", "Unable to parse dipoles.");
					if (parser_vals.size() < (size_t) ((data->rows() - 1) * 7))
						RSthrow(error::error_types::xBadInput)
						.add<std::string>("Reason", "When reading shapefile, "
							"header dipoles do not match the number in the file.");

					for (size_t i = 0; i < numPoints; ++i)
					{
						// First field truly is a dummy variable. No correclation with point ordering at all.
						//size_t pIndex = parser_vals[index].at(7 * i) - 1;
						size_t pIndex = 7 * i;
						auto crdsm = data->block<1, Ryan_Scat::shape::backends::NUM_SHAPECOLS>(i, 0);
						for (size_t j = 0; j < 7; j++) // TODO: rewrite using eigen?
						{
							float val = (float)parser_vals.at(pIndex + j);
							crdsm(j) = val;
						}
					}
					
				}

				/*
				void shapefile::print(std::ostream &out) const
				{
					using namespace std;
					out << desc << endl;
					out << numPoints << "\t= Number of lattice points" << endl;
					out << a1(0) << "\t" << a1(1) << "\t" << a1(2);
					out << "\t= target vector a1 (in TF)" << endl;
					out << a2(0) << "\t" << a2(1) << "\t" << a2(2);
					out << "\t= target vector a2 (in TF)" << endl;
					out << d(0) << "\t" << d(1) << "\t" << d(2);
					out << "\t= d_x/d  d_y/d  d_x/d  (normally 1 1 1)" << endl;
					out << x0(0) << "\t" << x0(1) << "\t" << x0(2);
					out << "\t= X0(1-3) = location in lattice of target origin" << endl;
					out << "\tNo.\tix\tiy\tiz\tICOMP(x, y, z)" << endl;
					//size_t i = 1;

					std::vector<long> oi(numPoints * 7);

					for (size_t j = 0; j < numPoints; j++)
					{
						long point = latticeIndex(j);
						auto it = latticePts.block<1, 3>(j, 0);
						auto ot = latticePtsRi.block<1, 3>(j, 0);
						oi[j * 7 + 0] = point;
						oi[j * 7 + 1] = (long)(it)(0);
						oi[j * 7 + 2] = (long)(it)(1);
						oi[j * 7 + 3] = (long)(it)(2);
						oi[j * 7 + 4] = (long)(ot)(0);
						oi[j * 7 + 5] = (long)(ot)(1);
						oi[j * 7 + 6] = (long)(ot)(2);

						//out << "\t" << i << "\t";
						//out << (it)(0) << "\t" << (it)(1) << "\t" << (it)(2) << "\t";
						//out << (ot)(0) << "\t" << (ot)(1) << "\t" << (ot)(2);
						//out << endl;
					}

					std::string generated;
					std::back_insert_iterator<std::string> sink(generated);
					if (!print_shapefile_entries(sink, oi))
					{
						// generating failed
						cerr << "Generating failed\n";
						throw;
					}
					out << generated;
				}

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
