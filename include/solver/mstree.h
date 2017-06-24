#ifndef MINSPANTREE_H
#define MINSPANTREE_H

#include "isolver.h"
#include "imstree.h"
#include "mstlem.h"
#include "data.h"
#include "lemon-net.h"
#include <lemon/adaptors.h>

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
            ~MinimumSpanningTree() {}

            void Solve(std::string net);
            void Solve(netxpert::InternalNet& net);

            netxpert::cnfg::MSTAlgorithm GetAlgorithm() const;
            void SetAlgorithm(netxpert::cnfg::MSTAlgorithm mstAlgorithm);

            const double GetOptimum() const;
            std::vector<netxpert::data::arc_t> GetMinimumSpanningTree() const;
            void SaveResults(const std::string& resultTableName,
                             const netxpert::data::ColumnMap& cmap) const;

        private:
            //raw pointer ok, no dynamic allocation (new())
            //smart pointers will not work, because Network is passed by reference and
            //shall be assigned to the class member this->net
            //with smart pointers there are double frees on clean up -> memory errors
            //raw pointers will not leak in this case even without delete in the deconstructor
            netxpert::InternalNet* net;
            netxpert::cnfg::Config NETXPERT_CNFG;
            std::vector<netxpert::data::arc_t> minimumSpanTree;
            netxpert::cnfg::MSTAlgorithm algorithm;
            std::unique_ptr<netxpert::core::IMinSpanTree> mst;
            void solve (netxpert::InternalNet& net);
            bool validateNetworkData(netxpert::InternalNet& net);
            lemon::FilterArcs<netxpert::data::graph_t, netxpert::data::graph_t::ArcMap<bool>>
             convertInternalNetworkToSolverData(netxpert::InternalNet& net);

    };
}
#endif // MINSPANTREE_H
