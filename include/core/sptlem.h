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

#ifndef SPT_LEM_H
#define SPT_LEM_H

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <vector>

//Contraction Hierarchies
#include <iostream>
#include "lemon/smart_graph.h"
//#include "lemon/static_graph.h"
#include "lemon/dijkstra.h"
#include "lemon/random.h"
#include "lemon/list_graph.h"
#include "lemon/time_measure.h"

#include "bijkstra.h"

#if (defined ENABLE_CONTRACTION_HIERARCHIES)
    #include "CHInterface.h"
    #include "CH/DefaultPriority.h"
#endif

//Heap
#include "lemon/quad_heap.h"
#include "lemon/dheap.h"
#include "lemon/fib_heap.h"

//Filter of Arcs
#include "lemon/adaptors.h"

#include "isptree.h"

using namespace lemon;
using namespace netxpert::data;

namespace netxpert {
    namespace core {

    template <typename T>
    class Inf {
    public:
        Inf() {}
        operator T() { return( std::numeric_limits<T>::max() ); }
    };

    //typedef FibHeap<SmartDigraph::ArcMap<double>, SmartDigraph::NodeMap<double>> FibonacciHeap;
    //using DijkstraInternal = Dijkstra<SmartDigraph, SmartDigraph::ArcMap<double>>;

    /**
    *  \Class Core Solver for the Shortest Path Tree Problem
    *   with binary Heap structure as default heap and
    *   Dijkstra's algorithm of LEMON.
    */
    class SPT_LEM : public netxpert::core::ISPTree
    {
//        typedef lemon::FilterArcs<netxpert::data::graph_t,
//                                              netxpert::data::graph_t::ArcMap<bool>> netxpert::data::filtered_graph_t;

//        typedef lemon::Dijkstra<filtered_graph_t, graph_t::ArcMap<cost_t>>
//                        ::SetPredMap<lemon::concepts::ReadWriteMap<node_t,arc_t>>::Create dijkstra_t;

        typedef lemon::Dijkstra<filtered_graph_t, graph_t::ArcMap<cost_t>> dijkstra_t;
        typedef lemon::Bijkstra<filtered_graph_t, graph_t::ArcMap<cost_t>> bijkstra_t;

        public:
            SPT_LEM(bool Drctd = true);
            SPT_LEM (SPT_LEM const &) {} //copy constrcutor
            ~SPT_LEM(); //deconstructor

            /* LEMON friendly interface */
            void LoadNet(const uint32_t nmax,  const uint32_t mmax,
                      //const lemon::FilterArcs<const netxpert::data::graph_t, const netxpert::data::graph_t::ArcMap<bool>>& sg,
                      //const netxpert::data::graph_t::ArcMap<netxpert::data::cost_t>& cm);
                      netxpert::data::filtered_graph_t* sg,
                      netxpert::data::graph_t::ArcMap<netxpert::data::cost_t>* cm);
            const uint32_t GetArcCount();
            const uint32_t GetNodeCount();
            void SolveSPT(netxpert::data::cost_t treshold = -1, bool bidirectional = true);
            void SetOrigin( netxpert::data::node_t _origin );
            void SetDest( netxpert::data::node_t _dest );
            bool Reached( netxpert::data::node_t _node );
            const std::vector<netxpert::data::node_t> GetPredecessors(netxpert::data::node_t _dest);
            const std::vector<netxpert::data::arc_t> GetPath(netxpert::data::node_t _dest);
            const netxpert::data::cost_t GetDist(netxpert::data::node_t _dest);
            /* end of LEMON friendly interface */

            /* Contraction Hierarchies */
            #if (defined ENABLE_CONTRACTION_HIERARCHIES)
            void LoadNet_CH(CHInterface<DefaultPriority>* _chm,
//                            graph_ch_t* _chg,
                            netxpert::data::graph_ch_t::ArcMap<netxpert::data::cost_t>* _chCostMap,
//                            filtered_graph_t::NodeMap<graph_ch_t::Node>* _nm);
                            netxpert::data::graph_t::NodeMap<netxpert::data::graph_ch_t::Node>* _nm,
                            netxpert::data::graph_ch_t::NodeMap<netxpert::data::graph_t::Node>* _cnm);
//                            netxpert::data::graph_ch_t::ArcMap<netxpert::data::graph_t::Arc>* _am);
            void SolveSPT_CH();
            const std::vector<netxpert::data::arc_t> GetPath_CH();
            const netxpert::data::cost_t GetDist_CH();
            bool Reached_CH();
            #endif
            /* end of Contraction Hierarchies */

            //void PrintResult();

        protected:
            uint32_t nmax; //max count nodes
            uint32_t mmax; //max count arcs

        private:
            //using DijkstraInternal = Dijkstra<StaticDigraph, StaticDigraph::ArcMap<double>>;
            //using DijkstraInternal = Dijkstra<SmartDigraph, SmartDigraph::ArcMap<double>>;
            bool isDrctd;
            bool allDests;
            bool bidirectional = false;

            //DijkstraInternal* dijk;
            /*lemon::Dijkstra<netxpert::data::graph_t, netxpert::data::graph_t::ArcMap<cost_t>>* dijk;
            netxpert::data::graph_t g;
            netxpert::data::graph_t::NodeMap<cost_t>* distMap;
            netxpert::data::graph_t::ArcMap<cost_t>* costMap;
            netxpert::data::node_t orig;
            netxpert::data::node_t dest;
            std::vector<netxpert::data::node_t> nodes;*/

            std::unique_ptr<dijkstra_t> dijk;
            std::unique_ptr<bijkstra_t> bijk;

            netxpert::data::filtered_graph_t* g;
            netxpert::data::graph_t::NodeMap<cost_t>* distMap;
            //lemon::concepts::ReadWriteMap<node_t,arc_t>* predMap;
            netxpert::data::graph_t::ArcMap<cost_t>* costMap;
            netxpert::data::node_t orig;
            netxpert::data::node_t dest;
            //std::vector<netxpert::data::node_t> nodes;

            //Contraction Hierarchies
            #if (defined ENABLE_CONTRACTION_HIERARCHIES)
            bool hasContractionHierarchies = false;
            graph_ch_t* chg;
            CHInterface<DefaultPriority>* chManager;
            netxpert::data::graph_ch_t::ArcMap<netxpert::data::cost_t>* chCostMap;
            netxpert::data::graph_t::NodeMap<graph_ch_t::Node>* chNodeRefMap;
//            netxpert::data::graph_ch_t::ArcMap<graph_t::Arc>* chArcRefMap;
            netxpert::data::graph_ch_t::NodeMap<graph_t::Node>* chNodeCrossRefMap;
            #endif

            //Static graph
            /*lemon::Dijkstra<StaticDigraph, StaticDigraph::ArcMap<cost_t>>* dijk;
            StaticDigraph g;
            StaticDigraph::NodeMap<cost_t>* distMap;
            StaticDigraph::ArcMap<cost_t>* length;
            StaticDigraph::Node orig;
            StaticDigraph::Node dest;
            std::vector<StaticDigraph::Node> nodes;*/
            //->
            //uint32_t* predecessors;
            //uint32_t* arcPredecessors;
    };
} //namespace core
} //namespace netxpert

#endif // SPT_LEM_H
