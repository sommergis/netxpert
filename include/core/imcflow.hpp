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

#ifndef IMINCOSTFLOW_H
#define IMINCOSTFLOW_H

#include <stdint.h>
#include <memory>
#include <vector>
#include "lemon-net.hpp"

namespace netxpert {

    namespace core {
    /**
    * \brief Pure virtual (i.e. abstract) class for all Minimum Cost Flow Problem Solvers in netxpert core.
    */
    class IMinCostFlow
    {
        public:
            /// Virtual Destructor
            virtual ~IMinCostFlow(){}

            /* LEMON friendly interface */
            ///\brief Loads the network into the core solver
            virtual void LoadNet(const uint32_t nmax,  const uint32_t mmax,
                      lemon::FilterArcs<netxpert::data::graph_t,
                                              netxpert::data::graph_t::ArcMap<bool>>* sg,
                      netxpert::data::graph_t::ArcMap<netxpert::data::cost_t>* _costMap,
                      netxpert::data::graph_t::ArcMap<netxpert::data::capacity_t>* _capMap,
                      netxpert::data::graph_t::NodeMap<netxpert::data::supply_t>* _supplyMap)=0;
            ///\brief Gets the count of the arcs of the internal graph
            virtual const uint32_t GetArcCount()=0;
            ///\brief Gets the count of the nodes of the internal graph
            virtual const uint32_t GetNodeCount()=0;
            ///\brief Solves the MCF problem
            virtual void SolveMCF()=0;
            ///\brief Gets the overall optimum of the problem
            virtual const double GetOptimum() const =0;
            ///\brief Gets the result set of arcs
            virtual std::vector<netxpert::data::arc_t> GetMCFArcs()=0;
            ///\brief Gets a arc map with the resulting flow for each arc
            virtual netxpert::data::graph_t::ArcMap<netxpert::data::flow_t>* GetMCFFlow()=0;
            ///\brief Gets a arc map with the resulting cost for each arc
            virtual netxpert::data::graph_t::ArcMap<netxpert::data::cost_t>* GetMCFCost()=0;
            /* end of LEMON friendly interface */
            ///\brief Gets the status of the internal MCF solver
            virtual const int GetMCFStatus()=0;
    };
} //namespace core
}//namespace netxpert
#endif // IMINCOSTFLOW_H
