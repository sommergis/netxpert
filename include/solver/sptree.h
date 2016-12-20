#ifndef SPTREE_H
#define SPTREE_H

#include "isolver.h"
#include "isptree.h"
#include "SPTree_Dijkstra.h"
#include "SPTree_Heap.h"
#include "SPTree_LDeque.h"
#include "SPTree_LQueue.h"
#include "sptlem.h"
#include "sptlem_bijkstra.h"
#include "sptbgl.h"
#include "data.h"

namespace netxpert {

    /**
    * \Class Solver for the Shortest Path Tree Problem
    */
    class ShortestPathTree : public netxpert::ISolver
    {
        public:
            /** Default constructor */
            ShortestPathTree(netxpert::cnfg::Config& cnfg);

            /** Default destructor */
            virtual ~ShortestPathTree() {}

            void Solve(std::string net);
            void Solve(netxpert::Network& net);

            netxpert::cnfg::SPTAlgorithm GetAlgorithm() const;
            void SetAlgorithm(netxpert::cnfg::SPTAlgorithm mstAlgorithm);

            int GetSPTHeapCard() const;
            void SetSPTHeapCard(int heapCard);

            netxpert::cnfg::GEOMETRY_HANDLING GetGeometryHandling() const;
            void SetGeometryHandling(netxpert::cnfg::GEOMETRY_HANDLING geomHandling);

            unsigned int GetOrigin() const;
            void SetOrigin(unsigned int orig);

            std::vector<unsigned int> GetDestinations() const;
            void SetDestinations(std::vector<unsigned int>& dests);

            std::vector<unsigned int> GetReachedDests() const;
            std::unordered_map<netxpert::data::ODPair, netxpert::data::CompressedPath> GetShortestPaths() const;

            double GetOptimum() const;

            void SaveResults(const std::string& resultTableName,
                             const netxpert::data::ColumnMap& cmap) const;

            std::vector<netxpert::data::InternalArc> UncompressRoute(unsigned int orig, std::vector<unsigned int>& ends) const;

        private:
            netxpert::Network* net; //raw pointer ok, no dynamic allocation (new())
            bool isDirected;
            int sptHeapCard;
            double optimum;
            std::vector<unsigned int> destinationNodes;
            std::vector<unsigned int> reachedDests;
            unsigned int originNode;
            std::unordered_map<netxpert::data::ODPair, netxpert::data::CompressedPath> shortestPaths;
            netxpert::cnfg::GEOMETRY_HANDLING geometryHandling;
            netxpert::cnfg::SPTAlgorithm algorithm;
            std::unique_ptr<netxpert::core::ISPTree> spt;
            void solve (netxpert::Network& net, unsigned int originNode, unsigned int destinationNode, bool isDirected);
            void solve (netxpert::Network& net, unsigned int originNode, std::vector<unsigned int> destinationNodes, bool isDirected);
            void solve (netxpert::Network& net, unsigned int originNode, bool isDirected);
            bool validateNetworkData(netxpert::Network& net, unsigned int orig);
            bool validateNetworkData(netxpert::Network& net, unsigned int orig, unsigned int dest);
            bool validateNetworkData(netxpert::Network& net, unsigned int orig, std::vector<unsigned int>& dests);
            void convertInternalNetworkToSolverData(netxpert::Network& net, std::vector<unsigned int>& sNds,
                                                    std::vector<unsigned int>& eNds, std::vector<double>& supply,
                                                    std::vector<double>& caps, std::vector<double>& costs);
            void checkSPTHeapCard(unsigned int arcCount, unsigned int nodeCount);
            double buildCompressedRoute(std::vector<unsigned int>& route, unsigned int orig, unsigned int dest,
                                            std::unordered_map<unsigned int, unsigned int>& arcPredescessors);
            double getArcCost(const netxpert::data::InternalArc& arc);
            netxpert::cnfg::Config NETXPERT_CNFG;
    };
}
#endif // SPTREE_H
