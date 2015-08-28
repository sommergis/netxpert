#ifndef ODMATRIX_H
#define ODMATRIX_H

#include <omp.h>

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
    * \Class Solver for computing an Origin Destination Matrix
    */
    class OriginDestinationMatrix : public ISolver
    {
        public:
            /** Default constructor */
            OriginDestinationMatrix(Config& cnfg);

            /** Default destructor */
            virtual ~OriginDestinationMatrix();

            void Solve(string net);
            void Solve(Network& net);

            SPTAlgorithm GetAlgorithm() const;
            void SetAlgorithm(SPTAlgorithm mstAlgorithm);

            int GetSPTHeapCard() const;
            void SetSPTHeapCard(int heapCard);

            GEOMETRY_HANDLING GetGeometryHandling() const;
            void SetGeometryHandling(GEOMETRY_HANDLING geomHandling);

            vector<unsigned int> GetOrigins() const;
            void SetOrigins(vector<unsigned int>& origs);
            void SetOrigins(vector<pair<unsigned int, string>>& origs);

            vector<unsigned int> GetDestinations() const;
            void SetDestinations(vector<unsigned int>& dests);
            void SetDestinations(vector<pair<unsigned int, string>>& dests);

            vector<unsigned int> GetReachedDests() const;
            unordered_map<ODPair, CompressedPath> GetShortestPaths() const;
            unordered_map<ODPair, double> GetODMatrix() const;

            double GetOptimum() const;
            vector<InternalArc> UncompressRoute(unsigned int orig, vector<unsigned int>& ends) const;

        private:
            Network* net;  //raw pointer ok, no dynamic allocation (new())
            bool isDirected;
            int sptHeapCard;
            double optimum;
            vector<unsigned int> destinationNodes;
            vector<unsigned int> reachedDests;
            vector<unsigned int> originNodes;
            unordered_map<ODPair, CompressedPath> shortestPaths;
            unordered_map<ODPair, double> odMatrix;
            GEOMETRY_HANDLING geometryHandling;
            SPTAlgorithm algorithm;
            shared_ptr<ISPTree> spt;
            void solve (Network& net, vector<unsigned int>& originNodes, vector<unsigned int>& destinationNodes, bool isDirected);
            bool validateNetworkData(Network& net, vector<unsigned int>& origs, vector<unsigned int>& dests);
            void convertInternalNetworkToSolverData(Network& net, vector<unsigned int>& sNds,
                                                    vector<unsigned int>& eNds, vector<double>& supply,
                                                    vector<double>& caps, vector<double>& costs);
            void checkSPTHeapCard(unsigned int arcCount, unsigned int nodeCount);
            double buildCompressedRoute(vector<unsigned int>& route, unsigned int orig, unsigned int dest,
                                            unordered_map<unsigned int, unsigned int>& arcPredescessors);
            double getArcCost(const InternalArc& arc);
    };
}
#endif // ODMATRIX_H
