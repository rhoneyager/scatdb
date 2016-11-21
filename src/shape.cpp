#include <map>

#include "../scatdb/error.hpp"
#include "../scatdb/hash.hpp"
#include "../scatdb/shape/shape.hpp"
#include "../private/shapeBackend.hpp"

namespace scatdb {
	namespace shape {
		shape::shape() { p = std::shared_ptr<shapeBackend>(new shapeBackend); }
		shape::~shape() {}
		std::shared_ptr<shape> shape::generate() {
			std::shared_ptr<shape> res(new shape);
			return res;
		}
		shapeBackend::shapeBackend() : dSpacing(0) {
			data = std::shared_ptr<shapeStorage_t>(new shapeStorage_t);
			ptsonly = std::shared_ptr<shapePointsOnly_t>(new shapePointsOnly_t);
			//curHash = genHash();
			//ingest = ::scatdb::generateIngest();
			stats = std::shared_ptr<shapePointStatsStorage_t>(new shapePointStatsStorage_t);
			header = std::shared_ptr<shapeHeaderStorage_t>(new shapeHeaderStorage_t);
			cache = cache_t(new std::map<std::string, genericAlgRes_t>);
		}
		shapeBackend::~shapeBackend() {}
		hash::HASH_p shapeBackend::genHash() const {
			const int sz = (int) data->size();
			hash::HASH_p res = hash::HASH(
				data->data(),
				sz * sizeof(shapeStorage_t::Scalar));
			return res;
		}
		void shapeBackend::invalidate() {
			hash::HASH_p newHash = genHash();
			if (*(curHash.get()) == *(newHash.get())) return;
			curHash = newHash;
			genStats();
			//ingest = ::Ryan_Scat::generateIngest();
			cache->clear();
		}

		/// \todo This will eventually again encompass
		/// the max distance between points, the surface areas and volumes, 
		/// the readii of gyration, other moments, etc.
		void shapeBackend::genStats() {
			std::shared_ptr<shapePointStatsStorage_t> res(new shapePointStatsStorage_t);
			if (ptsonly->rows()) {
				for (int i = 0; i < 3; ++i) {
					auto c = ptsonly->block(0, i, ptsonly->rows(), 1);
					(*res)(i, backends::PTMINS) = c.minCoeff();
					(*res)(i, backends::PTMAXS) = c.maxCoeff();
				}
			}
			stats = res;
		}

		shapeStorage_p shape::getPoints() const {
			return p->data;
		}
		void shape::getPoints(shapeStorage_p& r) const {
			r = p->data;
		}
		void shape::getPoints(shapePointsOnly_p& r) const {
			r = p->ptsonly;
		}
		void shape::getPoints(shapePointsOnly_t& r) const {
			r = *(p->ptsonly);
		}
		
		size_t shape::numPoints() const {
			return (size_t)p->data->rows();
		}
		std::string shape::getDescription() const {
			return p->desc;
		}
		void shape::setDescription(const std::string& r) {
			p->desc = r;
		}
		double shape::getPreferredDipoleSpacing() const { return p->dSpacing; }
		void shape::setPreferredDipoleSpacing(double r) { p->dSpacing = r; }
		//ingest_ptr shape::getIngestInformation() const { return p->ingest; }
		//void shape::setIngestInformation(const ingest_ptr r) { p->ingest = r; }
		void shape::getTags(tags_t& r) const { r = p->tags; }
		void shape::setTags(const tags_t& r) { p->tags = r; }
		shapeHeaderStorage_p shape::getHeader() const { return p->header; }
		void shape::setHeader(shapeHeaderStorage_p r) { p->header = r; }
		shapePointStatsStorage_p shape::getStats() const { return p->stats; }
		void shape::getStats(shapePointStatsStorage_p r) const { r = p->stats; }
		void shape::getStats(shapePointStatsStorage_t& r) const { r = *(p->stats.get()); }
		hash::HASH_p shape::hash() const { return p->curHash; }
		cache_t shape::cache() const { return p->cache; }

		void shape::setPoints(const shapeStorage_p r) {
			p->data = r;
			std::shared_ptr<shapePointsOnly_t> ponly(new shapePointsOnly_t);
			*(ponly.get()) = r->block(0, backends::X, r->rows(), 3);
			p->ptsonly = ponly;

			p->invalidate();
		}
		void shape::setPoints(const shapePointsOnly_t r) {
			std::shared_ptr<shapePointsOnly_t> ponly(new shapePointsOnly_t);
			*(ponly.get()) = r;
			p->ptsonly = ponly;
			p->genDataFromPtsOnly();
			p->invalidate();
		}
		void shape::setPoints(const shapePointsOnly_p r) {
			p->ptsonly = r;
			p->genDataFromPtsOnly();
			p->invalidate();
		}
		void shapeBackend::genDataFromPtsOnly() {
			std::shared_ptr<shapeStorage_t> res(new shapeStorage_t);
			res->resize(ptsonly->rows(), backends::NUM_SHAPECOLS);
			res->block(0, backends::X, res->rows(), 3) = ptsonly->block(0,0,res->rows(),3);
			Eigen::Matrix<shapeStorage_t::Scalar, Eigen::Dynamic, 1> ids;
			ids.resize(res->rows());
			ids.setLinSpaced(0, (shapeStorage_t::Scalar)res->rows());
			res->block(0, backends::ID, res->rows(), 1) = ids;
			res->block(0, backends::IX, res->rows(), 3).setOnes();
			data = res;
		}
	}
}