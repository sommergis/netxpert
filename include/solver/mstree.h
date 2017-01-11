#ifndef MINSPANTREE_H
#define MINSPANTREE_H

#include "isolver.h"
#include "imstree.h"
#include "mstlem.h"

namespace netxpert {
    /**
    * \Class Solver for the Minimum Spanning Tree Problem
    */
    class MinimumSpanningTree : public netxpert::ISolver
    {
        public:
            /** Default constructor */
            MinimumSpanningTree(netxpert::cnfg::Config& cnfg);

            /** Default destructor */
            virtual ~MinimumSpanningTree() {}

            void Solve(std::string net);
            void Solve(netxpert::Network& net);

            netxpert::cnfg::MSTAlgorithm GetAlgorithm();
            void SetAlgorithm(netxpert::cnfg::MSTAlgorithm mstAlgorithm);

            double GetOptimum() const;
            std::vector<netxpert::data::InternalArc> GetMinimumSpanningTree() const;
            void SaveResults(const std::string& resultTableName, const netxpert::data::ColumnMap& cmap) const;

        private:
            //raw pointer ok, no dynamic allocation (new())
            //smart pointers will not work, because Network is passed by reference and
            //shall be assigned to the class member this->net
            //with smart pointers there are double frees on clean up -> memory errors
            //raw pointers will not leak int this case even without delete in the deconstructor
            Network* net;
            netxpert::cnfg::Config NETXPERT_CNFG;
            std::vector<netxpert::data::InternalArc> minimumSpanTree;
            netxpert::cnfg::MSTAlgorithm algorithm;
            std::unique_ptr<netxpert::core::IMinSpanTree> mst;
            void solve (netxpert::Network& net);
            bool validateNetworkData(netxpert::Network& net);
            void convertInternalNetworkToSolverData(netxpert::Network& net, std::vector<unsigned int>& sNds,
                                                    std::vector<unsigned int>& eNds, std::vector<double>& supply,
                                                    std::vector<double>& caps, std::vector<double>& costs);
    };
}
#endif // MINSPANTREE_H
