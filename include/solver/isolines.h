#ifndef ISOLINES_H
#define ISOLINES_H

#include "isolver.h"
#include "isptree.h"
#include "SPTree_Dijkstra.h"
#include "SPTree_Heap.h"
#include "SPTree_LDeque.h"
#include "SPTree_LQueue.h"
#include "sptlem.h"
#include "sptlem_bijkstra.h"
#include "data.h"

namespace netxpert {

    /**
    * \Class Solver for calculating Isolines (= bands of accessible areas in a network)
    *
    * Concept: solve the 1 - all SPT problem and cut off the routes at the defined cut off values;
    *          do this for all origins.
    *
    */
    class Isolines : public ISolver
    {
        public:
            Isolines(Config& cnfg);
            virtual ~Isolines() {}
            void Solve(std::string net);
            void Solve(Network& net);

            SPTAlgorithm GetAlgorithm() const;
            void SetAlgorithm(SPTAlgorithm mstAlgorithm);

            int GetSPTHeapCard() const;
            void SetSPTHeapCard(int heapCard);

            GEOMETRY_HANDLING GetGeometryHandling() const;
            void SetGeometryHandling(GEOMETRY_HANDLING geomHandling);

            std::vector< std::pair<unsigned int,std::string> > GetOrigins() const;
            void SetOrigins(std::vector< std::pair<unsigned int,std::string> >& origs);

            std::map<ExtNodeID, std::vector<double> > GetCutOffs();
            void SetCutOffs(std::map<ExtNodeID, std::vector<double> >& cutOffs);

            double GetOptimum() const;

            void SaveResults(const std::string& resultTableName,
                             const netxpert::ColumnMap& cmap) const;

            std::unordered_map<ODPair, CompressedPath> GetShortestPaths() const;

            std::vector<InternalArc> UncompressRoute(unsigned int orig, std::vector<unsigned int>& ends) const;

        private:
            Network* net; //raw pointer ok, no dynamic allocation (new())
            bool isDirected;
            int sptHeapCard;
            double optimum;
            std::vector< std::pair<unsigned int,std::string> > originNodes;
            std::unordered_map<ODPair, CompressedPath> shortestPaths;
            std::unordered_map<ExtNodeID, std::vector<double> > cutOffs;
            GEOMETRY_HANDLING geometryHandling;
            SPTAlgorithm algorithm;
            std::unique_ptr<ISPTree> spt;
            void solve (Network& net, vector<unsigned int> originNodes, bool isDirected);
            bool validateNetworkData(Network& net, vector<unsigned int>& origs);
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
#endif // ISOLINES_H
