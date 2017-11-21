/*
 * This file is a part of netxpert.
 *
 * Copyright (C) 2013-2017
 * Johannes Sommer, Christopher Koller
 *
 * Permission to use, modify and distribute this software is granted
 * provided that this copyright notice appears in all copies. For
 * precise terms see the accompanying LICENSE file.
 *
 * This software is provided "AS IS" with no warranty of any kind,
 * express or implied, and with no claim as to its suitability for any
 * purpose.
 *
 */

#ifndef MINCOSTFLOW_H
#define MINCOSTFLOW_H

#include "isolver.h"
#include "imcflow.h"
#include "nslem.h"

namespace netxpert {
    /**
    * \brief Solver for the Minimum Cost Flow Problem
    */
    class MinCostFlow : public netxpert::ISolver
    {
        public:
            MinCostFlow(netxpert::cnfg::Config& cnfg);
            virtual ~MinCostFlow() {}
            void Solve(std::string net);
            void Solve(netxpert::data::InternalNet& net);
            bool IsDirected;
            std::vector<netxpert::data::FlowCost> GetMinCostFlow() const;
            netxpert::cnfg::MCFAlgorithm GetAlgorithm() const;
            void SetAlgorithm(netxpert::cnfg::MCFAlgorithm mcfAlgorithm);
            netxpert::data::MCFSolverStatus GetSolverStatus() const;
            const double GetOptimum() const;
            void SaveResults(const std::string& resultTableName,
                             const netxpert::data::ColumnMap& cmap) const;

        protected:
            //visible also to derived classes
            netxpert::cnfg::Config NETXPERT_CNFG;
            double optimum = 0;
            netxpert::data::MCFSolverStatus solverStatus;
            netxpert::cnfg::MCFAlgorithm algorithm;
            std::vector<netxpert::data::FlowCost> flowCost;
            std::shared_ptr<netxpert::core::IMinCostFlow> mcf;
            void solve (netxpert::data::InternalNet& net);
            bool validateNetworkData(netxpert::data::InternalNet& net);
            lemon::FilterArcs<netxpert::data::graph_t, netxpert::data::graph_t::ArcMap<bool>>
             convertInternalNetworkToSolverData(netxpert::data::InternalNet& net);
            void getSupplyNodesTypeCount(int& srcNodeCount, int& transshipNodeCount, int& sinkNodeCount );

        private:
            //private is only visible to MCF instance - not to derived classes (like TP)

            //raw pointer ok, no dynamic allocation (new())
            //smart pointers will not work, because Network is passed by reference and
            //shall be assigned to the class member this->net
            //with smart pointers there are double frees on clean up -> memory errors
            //raw pointers will not leak int this case even without delete in the deconstructor
            netxpert::data::InternalNet* net;
    };
}
#endif // MINCOSTFLOW_H
