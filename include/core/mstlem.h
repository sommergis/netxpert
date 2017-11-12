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
    /**
    * \brief Core solvers of netXpert
    **/
    namespace core {
    /**
    *  \brief Core Solver for the Minimum Spanning Tree Problem with LEMON.
    */
    class MST_LEM : public netxpert::core::IMinSpanTree
    {
        public:

            MST_LEM();
            MST_LEM (MST_LEM const &) {} //copy constrcutor
            ~MST_LEM(); //deconstructor

            /* LEMON friendly interface */
            void LoadNet(const uint32_t nmax,  const uint32_t mmax,
                                    lemon::FilterArcs<netxpert::data::graph_t,
                                              netxpert::data::graph_t::ArcMap<bool>>* sg,
                                    netxpert::data::graph_t::ArcMap<netxpert::data::cost_t>* cm);
            const uint32_t GetArcCount();
            const uint32_t GetNodeCount();
            void SolveMST();
            const double GetOptimum() const;
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
