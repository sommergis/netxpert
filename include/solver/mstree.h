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
    *
    * \li Initialization with a netxpert::data::InternalNet object in Constructor
    * \li call of \ref Solve( \ref netxpert::data::InternalNet ) method for the computation; the network must be constructed prior to this step
    * \li optional: GetOptimum() for getting the overall optimum
    * \li optional: GetMinimumSpanningTree() for getting the actual result of the MST computation
    * \li optional: SaveResults()
    *
    * all other setters and getters can be used for overriding the configuration from the constructor
    */
    class MinimumSpanningTree : public netxpert::ISolver
    {
        public:
            ///\brief Constructor
            MinimumSpanningTree(netxpert::cnfg::Config& cnfg);
            ///\brief Empty destructor
            ~MinimumSpanningTree() {}
            ///\brief Computes the minimum spanning tree problem
            ///\warning Not implemented! Should be removed.
            void Solve(std::string net);
            ///\brief Computes the minimum spanning tree problem on the given network.
            void Solve(netxpert::data::InternalNet& net);
            ///\brief Gets the type of mst algorithm used in the solver
            const netxpert::cnfg::MSTAlgorithm GetAlgorithm() const;
            ///\brief Sets the type of mst algorithm used in the solver
            void SetAlgorithm(netxpert::cnfg::MSTAlgorithm mstAlgorithm);
            ///\brief Gets the overall optimum of the solver
            const double GetOptimum() const;
            ///\brief Gets all the internal arcs of the network that form one minimum spanning tree
            std::vector<netxpert::data::arc_t> GetMinimumSpanningTree() const;
            ///\brief Gets all the original arc ids of the network that form one minimum spanning tree
            std::unordered_set<std::string> GetOrigMinimumSpanningTree() const;
            ///\brief Saves the results of the mst solver with the configured RESULT_DB_TYPE (SpatiaLite, FileGDB).
            ///\todo Implement JSON RESULT_DB_TYPE
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
