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

#ifndef NETSIMPLEXLEM_H
#define NETSIMPLEXLEM_H

#include "imcflow.h"
#include "lemon/smart_graph.h"
#include "lemon/network_simplex.h"
#include "lemon/adaptors.h"
#include <stdio.h>
#include <limits.h>

using namespace lemon;
using namespace netxpert::data;

namespace netxpert {
    namespace core {
    /**
    *  \Class Core Solver for the Minimum Cost Flow Problem with the Network Simplex algorithm of LEMON.
    */
    class NS_LEM : public netxpert::core::IMinCostFlow
    {
        typedef lemon::NetworkSimplex<netxpert::data::filtered_graph_t,
                                      netxpert::data::flow_t,
                                      netxpert::data::cost_t> netsimplex_t;

        public:
            NS_LEM();

            /* LEMON friendly interface */
            void LoadNet(const uint32_t nmax,  const uint32_t mmax,
                      lemon::FilterArcs<netxpert::data::graph_t,
                                              netxpert::data::graph_t::ArcMap<bool>>* sg,
                      netxpert::data::graph_t::ArcMap<netxpert::data::cost_t>* _costMap,
                      netxpert::data::graph_t::ArcMap<netxpert::data::capacity_t>* _capMap,
                      netxpert::data::graph_t::NodeMap<supply_t>* _supplyMap);
            const uint32_t GetArcCount();
            const uint32_t GetNodeCount();
            void SolveMCF();
            const double GetOptimum() const;
            std::vector<netxpert::data::arc_t> GetMCFArcs();
            netxpert::data::graph_t::ArcMap<netxpert::data::flow_t>* GetMCFFlow();
            netxpert::data::graph_t::ArcMap<netxpert::data::cost_t>* GetMCFCost();
            /* end of LEMON friendly interface */

            const int GetMCFStatus();

            void PrintResult();
            ~NS_LEM();

        protected:
            uint32_t nmax; //max count nodes
            uint32_t mmax; //max count arcs

        private:
            netxpert::data::filtered_graph_t* g;
            netxpert::data::graph_t::ArcMap<capacity_t>* capacityMap;
            netxpert::data::graph_t::ArcMap<cost_t>* costMap;
            netxpert::data::filtered_graph_t::ArcMap<flow_t>* flowMap;
            netxpert::data::graph_t::NodeMap<supply_t>* supplyMap;
            std::unique_ptr<netsimplex_t> nsimplex;
            netsimplex_t::ProblemType status;
    };
} //namespace core
} //namespace netxpert

#endif // NETSIMPLEXLEM_H
