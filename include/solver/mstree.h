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

#ifndef MINSPANTREE_H
#define MINSPANTREE_H

#include "isolver.h"
#include "imstree.h"
#include "mstlem.h"
#include "data.h"
#include "lemon-net.h"

namespace netxpert {

    /**
    * \brief Solver for the Minimum Spanning Tree Problem
    */
    class MinimumSpanningTree : public netxpert::ISolver
    {
        public:
            /** Default constructor */
            MinimumSpanningTree(netxpert::cnfg::Config& cnfg);

            /** Default destructor */
            ~MinimumSpanningTree() {}

            void Solve(std::string net);
            void Solve(netxpert::data::InternalNet& net);

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
            netxpert::data::InternalNet* net;
            netxpert::cnfg::Config NETXPERT_CNFG;
            std::vector<netxpert::data::arc_t> minimumSpanTree;
            netxpert::cnfg::MSTAlgorithm algorithm;
            std::unique_ptr<netxpert::core::IMinSpanTree> mst;
            void solve (netxpert::data::InternalNet& net);
            bool validateNetworkData(netxpert::data::InternalNet& net);
            lemon::FilterArcs<netxpert::data::graph_t, netxpert::data::graph_t::ArcMap<bool>>
             convertInternalNetworkToSolverData(netxpert::data::InternalNet& net);

    };
}
#endif // MINSPANTREE_H
