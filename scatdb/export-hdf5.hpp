#pragma once
#pragma warning( disable : 4251 ) // warning C4251: dll-interface

#include "defs.hpp"
#include <memory>
#include <string>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Dense>

#include <hdf5.h>
#include <H5Cpp.h>

namespace scatdb {
	namespace plugins
	{
		namespace hdf5
		{
			/// Master switch for zlib compression
			DLEXPORT_SDBR void useZLIB(bool);
			DLEXPORT_SDBR bool useZLIB();

			/// Provides a method for calculating the offsets from std::arrays of data
#define ARRAYOFFSET(TYPE, INDEX) [](){TYPE a; return (size_t) &a[INDEX] - (size_t) &a; }()

			DLEXPORT_SDBR std::shared_ptr<H5::Group> openOrCreateGroup(std::shared_ptr<H5::CommonFG> base, const char* name);
			DLEXPORT_SDBR std::shared_ptr<H5::Group> openGroup(std::shared_ptr<H5::CommonFG> base, const char* name);
			DLEXPORT_SDBR bool groupExists(std::shared_ptr<H5::CommonFG> base, const char* name);

			/// \param std::shared_ptr<H5::AtomType> is a pointer to a newly-constructed matching type
			/// \returns A pair of (the matching type, a flag indicating passing by pointer or reference)
			typedef std::shared_ptr<H5::AtomType> MatchAttributeTypeType;
			template <class DataType>
			MatchAttributeTypeType MatchAttributeType();

			/// Check to see if output type is for a string
			template <class DataType> bool isStrType() { return false; }
			template<> DLEXPORT_SDBR bool isStrType<std::string>();
			template<> DLEXPORT_SDBR bool isStrType<const char*>();

			/// Handles proper insertion of strings versus other data types
			template <class DataType>
			void insertAttr(H5::Attribute &attr, std::shared_ptr<H5::AtomType> vls_type, const DataType& value)
			{
				attr.write(*vls_type, &value);
			}
			template <> DLEXPORT_SDBR void insertAttr<std::string>(H5::Attribute &attr, std::shared_ptr<H5::AtomType> vls_type, const std::string& value);

			/// Convenient template to add an attribute of a variable type to a group or dataset
			template <class DataType, class Container>
			void addAttr(std::shared_ptr<Container> obj, const char* attname, const DataType &value)
			{
				std::shared_ptr<H5::AtomType> vls_type = MatchAttributeType<DataType>();
				H5::DataSpace att_space(H5S_SCALAR);
				H5::Attribute attr = obj->createAttribute(attname, *vls_type, att_space);
				insertAttr<DataType>(attr, vls_type, value);
			}

			/// Writes an array (or vector) of objects
			template <class DataType, class Container>
			void addAttrArray(std::shared_ptr<Container> obj, const char* attname, 
					const DataType *value, size_t rows, size_t cols)
			{
				hsize_t sz[] = { (hsize_t) rows, (hsize_t) cols };
				if (sz[0] == 1)
				{
					sz[0] = sz[1];
					sz[1] = 1;
				}
				int dimensionality = (sz[1] == 1) ? 1 : 2;

				std::shared_ptr<H5::AtomType> ftype = MatchAttributeType<DataType>();
				//H5::IntType ftype(H5::PredType::NATIVE_FLOAT);
				H5::ArrayType vls_type(*ftype, dimensionality, sz);

				H5::DataSpace att_space(H5S_SCALAR);
				H5::Attribute attr = obj->createAttribute(attname, vls_type, att_space);
				attr.write(vls_type, value);
			}

			/// Eigen objects have a special writing function, as MSVC 2012 disallows partial template specialization.
			template <class DataType, class Container>
			void addAttrEigen(std::shared_ptr<Container> obj, const char* attname, 
					const DataType &value)
			{
				addAttrArray<typename DataType::Scalar, Container>(
						obj, attname, value.data(), (size_t) value.rows(), (size_t) value.cols() );
			}

			/// Attribute writing for complex objects
			template <class DataType, class Container>
			void addAttrComplex(std::shared_ptr<Container> obj, const char* attname, 
					const DataType *value, size_t rows, size_t cols)
			{
				Eigen::Matrix<typename DataType::value_type, Eigen::Dynamic, Eigen::Dynamic> mr, mi;
				mr.resize(rows, cols); mi.resize(rows, cols);
				for (size_t i=0; i< rows; ++i)
					for (size_t j=0; j < cols; ++j)
					{
						mr(i,j) = value[i*cols+j].real();
						mi(i,j) = value[i*cols+j].imag();
					}
				std::string attR(attname); attR.append("_r");
				std::string attI(attname); attI.append("_i");
				addAttrEigen<Eigen::Matrix<typename DataType::value_type, 
					Eigen::Dynamic, Eigen::Dynamic>, Container>
						(obj, attR.c_str(), mr);
				addAttrEigen<Eigen::Matrix<typename DataType::value_type, 
					Eigen::Dynamic, Eigen::Dynamic>, Container>
						(obj, attI.c_str(), mi);
			}


			/// Handles proper insertion of strings versus other data types
			template <class DataType>
			void loadAttr(H5::Attribute &attr, std::shared_ptr<H5::AtomType> vls_type, DataType& value)
			{
				attr.read(*vls_type, &value);
			}
			template <> DLEXPORT_SDBR void loadAttr<std::string>(H5::Attribute &attr, std::shared_ptr<H5::AtomType> vls_type, std::string& value);


			/// Convenient template to read an attribute of a variable
			template <class DataType, class Container>
			void readAttr(std::shared_ptr<Container> obj, const char* attname, DataType &value)
			{
				std::shared_ptr<H5::AtomType> vls_type = MatchAttributeType<DataType>();
				H5::DataSpace att_space(H5S_SCALAR);
				H5::Attribute attr = obj->openAttribute(attname); //, *vls_type, att_space);
				loadAttr<DataType>(attr, vls_type, value);
			}

			/// Reads an array (or vector) of objects
			template <class DataType, class Container>
			void readAttrArray(std::shared_ptr<Container> obj, const char* attname,
				DataType *value, size_t rows, size_t cols)
			{
				H5::Attribute attr = obj->openAttribute(attname);
				int dimensionality = attr.getArrayType().getArrayNDims();
				hsize_t *sz = new hsize_t[dimensionality];
				attr.getArrayType().getArrayDims(sz);

				if (dimensionality == 1)
				{
					if (sz[0] != rows && sz[0] != cols) throw("Row/column mismatch in readAttrArray");
				}
				else {
					if (sz[0] != rows) throw("Rows mismatch in readAttrArray");
					if (sz[1] != cols) throw("Cols mismatch in readAttrArray");
				}
				

				//if (dimensionality == 2)
				//	value.resize(sz[0], sz[1]);
				//else if (dimensionality == 1)
				//	value.resize(sz[0]);

				std::shared_ptr<H5::AtomType> ftype = MatchAttributeType<DataType>();
				//H5::IntType ftype(H5::PredType::NATIVE_FLOAT);
				H5::ArrayType vls_type(*ftype, dimensionality, sz);

				//H5::DataSpace att_space(H5S_SCALAR);
				//H5::Attribute attr = obj->createAttribute(attname, vls_type, att_space);
				attr.read(vls_type, value);
				delete[] sz;
			}

			/// Eigen objects have a special writing function, as MSVC 2012 disallows partial template specialization.
			template <class DataType, class Container>
			void readAttrEigen(std::shared_ptr<Container> obj, const char* attname, DataType &value)
			{
				H5::Attribute attr = obj->openAttribute(attname);
				int dimensionality = attr.getArrayType().getArrayNDims();
				hsize_t *sz = new hsize_t[dimensionality];
				attr.getArrayType().getArrayDims(sz);

				if (dimensionality == 2)
					value.resize(sz[0], sz[1]);
				else if (dimensionality == 1)
					value.resize(sz[0], 1);

				std::shared_ptr<H5::AtomType> ftype = MatchAttributeType<typename DataType::Scalar>();
				//H5::IntType ftype(H5::PredType::NATIVE_FLOAT);
				H5::ArrayType vls_type(*ftype, dimensionality, sz);

				//H5::DataSpace att_space(H5S_SCALAR);
				//H5::Attribute attr = obj->createAttribute(attname, vls_type, att_space);
				attr.read(vls_type, value.data());
				delete[] sz;
			}

			/// Attribute reading for complex objects
			template <class DataType, class Container>
			void readAttrComplex(std::shared_ptr<Container> obj, const char* attname,
				DataType *value, size_t rows, size_t cols)
			{
				Eigen::Matrix<typename DataType::value_type, Eigen::Dynamic, Eigen::Dynamic> mr, mi;
				mr.resize(rows, cols); mi.resize(rows, cols);
				
				std::string attR(attname); attR.append("_r");
				std::string attI(attname); attI.append("_i");
				readAttrEigen<Eigen::Matrix<typename DataType::value_type,
					Eigen::Dynamic, Eigen::Dynamic>, Container>
					(obj, attR.c_str(), mr);
				readAttrEigen<Eigen::Matrix<typename DataType::value_type,
					Eigen::Dynamic, Eigen::Dynamic>, Container>
					(obj, attI.c_str(), mi);

				for (size_t i = 0; i< rows; ++i)
				for (size_t j = 0; j < cols; ++j)
				{
				value[i*cols + j] = std::complex<typename DataType::value_type>(mr(i, j), mi(i, j));
				}
			}


			DLEXPORT_SDBR bool attrExists(std::shared_ptr<H5::H5Object> obj, const char* attname);

			/// Convenience function to either open or create a group
			DLEXPORT_SDBR std::shared_ptr<H5::Group> openOrCreateGroup(
				std::shared_ptr<H5::CommonFG> base, const char* name);

			/// Convenience function to check if a given group exists
			DLEXPORT_SDBR bool groupExists(std::shared_ptr<H5::CommonFG> base, const char* name);

			/// Convenience function to check if a symbolic link exists, and if the object being 
			/// pointed to also exists.
			/// \returns std::pair<bool,bool> refers to, respectively, if a symbolic link is found and 
			/// if the symbolic link is good.
			DLEXPORT_SDBR std::pair<bool, bool> symLinkExists(std::shared_ptr<H5::CommonFG> base, const char* name);

			/// \brief Convenience function to open a group, if it exists
			/// \returns nullptr is the group does not exist.
			DLEXPORT_SDBR std::shared_ptr<H5::Group> openGroup(std::shared_ptr<H5::CommonFG> base, const char* name);

			/// Convenience function to check if a given dataset exists
			DLEXPORT_SDBR bool datasetExists(std::shared_ptr<H5::CommonFG> base, const char* name);

			/// Convenience function to write an Eigen object, in the correct format
			template <class DataType, class Container>
			std::shared_ptr<H5::DataSet> addDatasetEigen(std::shared_ptr<Container> obj, const char* name, 
				const DataType &value, std::shared_ptr<H5::DSetCreatPropList> iplist = nullptr)
			{
				using namespace H5;
				hsize_t sz[] = { (hsize_t) value.rows(), (hsize_t) value.cols() };
				int dimensionality = 2;
				DataSpace fspace(dimensionality, sz);
				std::shared_ptr<H5::AtomType> ftype = MatchAttributeType<typename DataType::Scalar>();

				std::shared_ptr<DSetCreatPropList> plist;
				if (iplist) plist = iplist;
				else
				{
					plist = std::shared_ptr<DSetCreatPropList>(new DSetCreatPropList);
					if (!isStrType<DataType>())
					{
						int fillvalue = -1;
						plist->setFillValue(PredType::NATIVE_INT, &fillvalue);
						if (useZLIB()) {
							plist->setChunk(2, sz);
							plist->setDeflate(6);
						}
					}
				}

				std::shared_ptr<DataSet> dataset(new DataSet(obj->createDataSet(name, *(ftype.get()), 
					fspace, *(plist.get())   )));
				Eigen::Matrix<typename DataType::Scalar, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> rowmajor(value);
				dataset->write(rowmajor.data(), *(ftype.get()));
				return dataset;
			}

			/// Convenience function to write a complex-valued Eigen dataset in a consistent manner
			template <class DataType, class Container>
			void addDatasetEigenComplexMethodA(std::shared_ptr<Container> obj, const char* name, 
				const DataType &value, std::shared_ptr<H5::DSetCreatPropList> iplist = nullptr)
			{
				using namespace H5;
				hsize_t sz[] = { (hsize_t) value.rows(), (hsize_t) value.cols() };
				int dimensionality = 2;
				DataSpace fspace(dimensionality, sz);
				std::shared_ptr<H5::AtomType> ftype = MatchAttributeType<typename DataType::Scalar::value_type>();

				std::shared_ptr<DSetCreatPropList> plist;
				if (iplist) plist = iplist;
				else
				{
					plist = std::shared_ptr<DSetCreatPropList>(new DSetCreatPropList);
					if (!isStrType<DataType>())
					{
						int fillvalue = -1;
						plist->setFillValue(PredType::NATIVE_INT, &fillvalue);
						if (useZLIB()) {
							plist->setChunk(2, sz);
							plist->setDeflate(6);
						}
					}
				}

				std::string snameReal(name); snameReal.append("_real");
				std::string snameImag(name); snameImag.append("_imag");
				std::shared_ptr<DataSet> datasetReal(new DataSet(obj->createDataSet(snameReal.c_str(), *(ftype.get()), 
					fspace, *(plist.get()))));
				std::shared_ptr<DataSet> datasetImag(new DataSet(obj->createDataSet(snameImag.c_str(), *(ftype.get()), 
					fspace, *(plist.get()))));
				Eigen::Matrix<typename DataType::Scalar::value_type, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> rmreal(value.real());
				Eigen::Matrix<typename DataType::Scalar::value_type, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> rmimag(value.imag());
				datasetReal->write(rmreal.data(), *(ftype.get()));
				datasetImag->write(rmimag.data(), *(ftype.get()));
			}

			template <class DataType, class Container>
			std::shared_ptr<H5::DataSet> addDatasetArray(std::shared_ptr<Container> obj, const char* name, size_t rows, size_t cols, 
				const DataType *values, std::shared_ptr<H5::DSetCreatPropList> iplist = nullptr)
			{
				using namespace H5;
				hsize_t sz[] = { (hsize_t) rows, (hsize_t) cols };
				int dimensionality = 2;
				if (cols == 1) dimensionality = 1;
				DataSpace fspace(dimensionality, sz);
				std::shared_ptr<H5::AtomType> ftype = MatchAttributeType<DataType>();
				std::shared_ptr<DSetCreatPropList> plist;
				if (iplist) plist = iplist;
				else
				{
					plist = std::shared_ptr<DSetCreatPropList>(new DSetCreatPropList);
					if (!isStrType<DataType>())
					{
						int fillvalue = -1;
						plist->setFillValue(PredType::NATIVE_INT, &fillvalue);
						if (useZLIB()) {
							plist->setChunk(2, sz);
							plist->setDeflate(6);
						}
					}
				}

				std::shared_ptr<DataSet> dataset(new DataSet(obj->createDataSet(name, *(ftype.get()), 
					fspace, *(plist.get()))));
				dataset->write(values, *(ftype.get()));
				return dataset;
			}

			template <class DataType, class Container>
			std::shared_ptr<H5::DataSet> addDatasetArray(std::shared_ptr<Container> obj, const char* name, size_t rows, 
				const DataType *values, std::shared_ptr<H5::DSetCreatPropList> iplist = nullptr)
			{
				return addDatasetArray(obj, name, rows, 1, values, iplist);
			}



			/// Convenience function to read an Eigen object, in the correct format
			template <class DataType, class Container>
			std::shared_ptr<H5::DataSet> readDatasetEigen(std::shared_ptr<Container> obj, const char* name,
				DataType &value) //, std::shared_ptr<H5::DSetCreatPropList> iplist = nullptr)
			{
				using namespace H5;
				std::shared_ptr<H5::DataSet> dataset(new H5::DataSet(obj->openDataSet(name)));
				H5T_class_t type_class = dataset->getTypeClass();
				DataSpace fspace = dataset->getSpace();
				int rank = fspace.getSimpleExtentNdims();

				//ArrayType a = dataset.getArrayType();
				//int dimensionality = a.getArrayNDims();
				//a.getArrayDims(sz);
				hsize_t *sz = new hsize_t[rank];
				int dimensionality = fspace.getSimpleExtentDims( sz, NULL);

				if (dimensionality == 2)
					value.resize(sz[0], sz[1]);
				else if (dimensionality == 1)
				{
					// Odd, but it keeps row and column-vectors separate.
					if (value.cols() == 1)
						value.resize(sz[0], 1);
					else value.resize(1, sz[0]);
				}


				//DataSpace fspace(dimensionality, sz);
				std::shared_ptr<H5::AtomType> ftype = MatchAttributeType<typename DataType::Scalar>();

				/*
				std::shared_ptr<H5::DSetCreatPropList> iplist = nullptr;
				std::shared_ptr<DSetCreatPropList> plist;
				if (iplist) plist = iplist;
				else
				{
				plist = std::shared_ptr<DSetCreatPropList>(new DSetCreatPropList);
				if (!isStrType<DataType>())
				{
				int fillvalue = -1;
				plist->setFillValue(PredType::NATIVE_INT, &fillvalue);
				}
				}
				*/

				//std::shared_ptr<DataSet> dataset(new DataSet(obj->createDataSet(name, *(ftype.get()), 
				//	fspace, *(plist.get())   )));
				Eigen::Matrix<typename DataType::Scalar, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> rowmajor(value);
				dataset->read(rowmajor.data(), *(ftype.get()));
				//dataset->write(rowmajor.data(), *(ftype.get()));
				value = rowmajor;
				delete[] sz;
				return dataset;
			}

			template <class Container>
			std::shared_ptr<H5::DataSet> readDatasetDimensions(std::shared_ptr<Container> obj, const char* name, std::vector<size_t> &out)
			{
				using namespace H5;

				std::shared_ptr<H5::DataSet> dataset(new H5::DataSet(obj->openDataSet(name)));
				H5T_class_t type_class = dataset->getTypeClass();
				DataSpace fspace = dataset->getSpace();
				int rank = fspace.getSimpleExtentNdims();

				hsize_t *sz = new hsize_t[rank];
				int dimensionality = fspace.getSimpleExtentDims( sz, NULL);
				for (size_t i = 0; i < rank; ++i)
					out.push_back(sz[i]);

				delete[] sz;
				return dataset;
			}

			template <class DataType, class Container>
			std::shared_ptr<H5::DataSet> readDatasetArray(std::shared_ptr<Container> obj, const char* name,
				DataType *values)
			{
				using namespace H5;

				std::shared_ptr<H5::DataSet> dataset(new H5::DataSet(obj->openDataSet(name)));
				H5T_class_t type_class = dataset->getTypeClass();
				DataSpace fspace = dataset->getSpace();
				/*
				int rank = fspace.getSimpleExtentNdims();

				hsize_t *sz = new hsize_t[rank];
				int dimensionality = fspace.getSimpleExtentDims( sz, NULL);

				if (dimensionality == 2)
				value.resize(sz[0], sz[1]);
				else if (dimensionality == 1)
				{
				// Odd, but it keeps row and column-vectors separate.
				if (value.cols() == 1)
				value.resize(sz[0], 1);
				else value.resize(1, sz[0]);
				}
				*/

				//DataSpace fspace(dimensionality, sz);
				std::shared_ptr<H5::AtomType> ftype = MatchAttributeType<DataType>();

				dataset->read(values, *(ftype.get()));
				//delete[] sz;
				return dataset;
			}

			/// \brief Add column names to table.
			/// \param num is the number of columns
			/// \param stride allows name duplication (for vectors)
			template <class Container>
			void addNames(std::shared_ptr<Container> obj, const std::string &prefix, size_t num, const std::function<std::string(int)> &s, size_t stride = 0, size_t mCols = 0)
			{
				size_t nstride = stride;
				if (!nstride) nstride = 1;
				for (size_t i = 0; i < num; ++i)
				{
					size_t j = i / nstride;
					std::string lbl = s((int)j);
					std::ostringstream fldname;
					fldname << prefix;
					fldname.width(2);
					fldname.fill('0');
					fldname << std::right << i << "_NAME";
					std::string sfldname = fldname.str();
					addAttr<std::string, Container>(obj, sfldname.c_str(), lbl);
				}
			}


			/// \brief Add column names to table.
			/// \param num is the number of columns
			/// \param stride allows name duplication (for vectors)
			template <class Container>
			void addColNames(std::shared_ptr<Container> obj, size_t num, const std::function<std::string(int)> &s, size_t stride = 0, size_t mCols = 0)
			{
				size_t nstride = stride;
				if (!nstride) nstride = 1;
				for (size_t i = 0; i < num; ++i)
				{
					size_t j = i / nstride;
					std::string lbl = s((int)j);
					std::ostringstream fldname;
					fldname << "COLUMN_";
					fldname.width(2);
					fldname.fill('0');
					fldname << std::right << i << "_NAME";
					std::string sfldname = fldname.str();
					if (stride)
					{
						size_t k = i % nstride;
						if (!mCols) {
							if (k == 0) lbl.append("_X");
							if (k == 1) lbl.append("_Y");
							if (k == 2) lbl.append("_Z");
							if (k == 3) lbl.append("_R");
						}
						else {
							size_t row = k / mCols;
							size_t col = k % mCols;
							std::ostringstream iappend;
							iappend << "_" << row << "_" << col;
							lbl.append(iappend.str());
						}
					}
					addAttr<std::string, Container>(obj, sfldname.c_str(), lbl);
				}
			}


			/// Creates a property list with the compression + chunking as specified
			DLEXPORT_SDBR std::shared_ptr<H5::DSetCreatPropList> make_plist(size_t rows, size_t cols, bool compress = true);
		}
	}
}
