#ifndef SPTREE_H
#define SPTREE_H

#include "isolver.h"
#include "isptree.h"
#include "SPTree_Dijkstra.h"
#include "SPTree_Heap.h"
#include "SPTree_LDeque.h"
#include "SPTree_LQueue.h"
#include "sptlem.h"
#include "data.h"

namespace netxpert {

    /**
    * \Class Solver for the Shortest Path Tree Problem
    */
    class ShortestPathTree : public ISolver
    {
        public:
            /** Default constructor */
            ShortestPathTree(Config& cnfg);

            /** Default destructor */
            virtual ~ShortestPathTree() {}

            void Solve(string net);
            void Solve(Network& net);

            SPTAlgorithm GetAlgorithm() const;
            void SetAlgorithm(SPTAlgorithm mstAlgorithm);

            int GetSPTHeapCard() const;
            void SetSPTHeapCard(int heapCard);

            GEOMETRY_HANDLING GetGeometryHandling() const;
            void SetGeometryHandling(GEOMETRY_HANDLING geomHandling);

            unsigned int GetOrigin() const;
            void SetOrigin(unsigned int orig);

            vector<unsigned int> GetDestinations() const;
            void SetDestinations(vector<unsigned int>& dests);

            vector<unsigned int> GetReachedDests() const;
            unordered_map<ODPair, CompressedPath> GetShortestPaths() const;

            double GetOptimum() const;

            void SaveResults(const std::string& resultTableName,
                             const netxpert::ColumnMap& cmap) const;

            vector<InternalArc> UncompressRoute(unsigned int orig, vector<unsigned int>& ends) const;

        private:
            Network* net; //raw pointer ok, no dynamic allocation (new())
            bool isDirected;
            int sptHeapCard;
            double optimum;
            vector<unsigned int> destinationNodes;
            vector<unsigned int> reachedDests;
            unsigned int originNode;
            unordered_map<ODPair, CompressedPath> shortestPaths;
            GEOMETRY_HANDLING geometryHandling;
            SPTAlgorithm algorithm;
            unique_ptr<ISPTree> spt;
            void solve (Network& net, unsigned int originNode, unsigned int destinationNode, bool isDirected);
            void solve (Network& net, unsigned int originNode, vector<unsigned int> destinationNodes, bool isDirected);
            void solve (Network& net, unsigned int originNode, bool isDirected);
            bool validateNetworkData(Network& net, unsigned int orig);
            bool validateNetworkData(Network& net, unsigned int orig, unsigned int dest);
            bool validateNetworkData(Network& net, unsigned int orig, vector<unsigned int>& dests);
            void convertInternalNetworkToSolverData(Network& net, vector<unsigned int>& sNds,
                                                    vector<unsigned int>& eNds, vector<double>& supply,
                                                    vector<double>& caps, vector<double>& costs);
            void checkSPTHeapCard(unsigned int arcCount, unsigned int nodeCount);
            double buildCompressedRoute(vector<unsigned int>& route, unsigned int orig, unsigned int dest,
                                            unordered_map<unsigned int, unsigned int>& arcPredescessors);
            double getArcCost(const InternalArc& arc);
            Config NETXPERT_CNFG;
    };
}
#endif // SPTREE_H
