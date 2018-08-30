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

#include "isolver.hpp"
#include "imcflow.hpp"
#include "nslem.hpp"

namespace netxpert {
    /**
    * \brief Solver for the Minimum Cost Flow Problem
    *
    * \li Initialization with a netxpert::data::InternalNet object in Constructor
    * \li call of \ref Solve( \ref netxpert::data::InternalNet ) method for the computation; the network must be constructed prior to this step
    * \li optional: GetSolverStatus() for getting the status of the solver \sa netxpert::data::MCFSolverStatus
    * \li optional: GetOptimum() for getting the overall optimum
    * \li optional: GetMinCostFlow() for getting the actual result of the MCF computation
    * \li optional: SaveResults()
    *
    * all other setters and getters can be used for overriding the configuration from the constructor
    */
    class MinCostFlow : public netxpert::ISolver
    {
        public:
            ///\brief Constructor
            MinCostFlow(netxpert::cnfg::Config& cnfg);
            ///\brief Virtual Empty destructor
            virtual ~MinCostFlow() {}
            ///\brief Computes the minimum cost flow problem
            ///\warning Not implemented! Should be removed.
            void Solve(std::string net);
            ///\brief Computes the minimum cost flow problem on the given network.
            void Solve(netxpert::data::InternalNet& net);
            ///\todo Implement Getter/Setter for property
            bool IsDirected;
            ///\brief Gets the arcs with flow and cost of the result of the minimum cost flow solver
            std::vector<netxpert::data::FlowCost> GetMinCostFlow() const;
            ///\brief Gets the type of mcf algorithm used in the solver
            const netxpert::cnfg::MCFAlgorithm GetAlgorithm() const;
            ///\brief Sets the type of mcf algorithm to use in the solver
            void SetAlgorithm(netxpert::cnfg::MCFAlgorithm mcfAlgorithm);
            ///\brief Gets the status of the mcf solver
            const netxpert::data::MCFSolverStatus GetSolverStatus() const;
            ///\brief Gets the overall optimum of the solver
            const double GetOptimum() const;
            ///\brief Saves the results of the MCF solver with the configured RESULT_DB_TYPE (SpatiaLite, FileGDB).
            void SaveResults(const std::string& resultTableName,
                             const netxpert::data::ColumnMap& cmap);
            ///\brief Gets the MCF results as JSON String
            const std::string GetResultsAsJSON();

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
            std::string processTotalArcIDs();

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
