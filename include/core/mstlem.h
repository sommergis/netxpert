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

#ifndef CORE_MSTLEM_H
#define CORE_MSTLEM_H

#include <stdio.h>
#include <string>
#include <limits.h> //UNIT_MAX
#include "imstree.h"
#include <lemon/kruskal.h>

using namespace lemon;
using namespace netxpert::data;

namespace netxpert {

    namespace core {
    /**
    *  \brief Core Solver for the Minimum Spanning Tree Problem with LEMON.
    */
    class MST_LEM : public netxpert::core::IMinSpanTree
    {
        public:
            ///\brief Empty Constructor
            MST_LEM() {}
            ///\brief Copy Constructor
            MST_LEM (MST_LEM const &) {}
            ///\brief Destructor
            ~MST_LEM();

            /* LEMON friendly interface */
            ///\brief Loads the network into the core solver
            void LoadNet(const uint32_t nmax,  const uint32_t mmax,
                                    lemon::FilterArcs<netxpert::data::graph_t,
                                              netxpert::data::graph_t::ArcMap<bool>>* sg,
                                    netxpert::data::graph_t::ArcMap<netxpert::data::cost_t>* cm);
            ///\brief Gets the count of the arcs of the internal graph
            const uint32_t GetArcCount();
            ///\brief Gets the count of the nodes of the internal graph
            const uint32_t GetNodeCount();
            ///\brief Solves the MST problem
            void SolveMST();
            ///\brief Gets the overall optimum of the problem
            const double GetOptimum() const;
            ///\brief Gets the result set of arcs
            std::vector<netxpert::data::arc_t> GetMST();
            /* end of LEMON friendly interface */

        protected:
            uint32_t nmax; //max count nodes
            uint32_t mmax; //max count arcs
            netxpert::data::filtered_graph_t* g;
            netxpert::data::filtered_graph_t::ArcMap<bool>* arcBoolMap;
            netxpert::data::graph_t::ArcMap<cost_t>* costMap;
            double totalCost;
            //Kruskal<SmartDigraph,SmartDigraph::ArcMap<double>>* mst;
            std::vector<netxpert::data::node_t> nodes;
    };
}//namespace core
}//namespace netxpert

#endif // CORE_MSTLEM_H
