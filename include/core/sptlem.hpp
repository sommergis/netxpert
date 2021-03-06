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
// LEMON contrib
#include "bijkstra.h"

#if (defined NETX_ENABLE_CONTRACTION_HIERARCHIES)
    #include "CHInterface.h"
    #include "CH/DefaultPriority.h"
#endif

// for different heaps
#include "lemon/quad_heap.h"
#include "lemon/dheap.h"
#include "lemon/fib_heap.h"

//Filter of Arcs
#include "lemon/adaptors.h"

#include "isptree.hpp"

using namespace lemon;
using namespace netxpert::data;

namespace netxpert {

    namespace core {

    /**
    * \brief Modeling Infinity for the given type T.
    **/
    template <typename T>
    class Inf {
    public:
        Inf() {}
        operator T() { return( std::numeric_limits<T>::max() ); }
    };

    // experiment for switching the heap type

    //typedef FibHeap<SmartDigraph::ArcMap<double>, SmartDigraph::NodeMap<double>> FibonacciHeap;
    //using DijkstraInternal = Dijkstra<SmartDigraph, SmartDigraph::ArcMap<double>>;

    /**
    *  \brief Core Solver for the Shortest Path Tree Problem
    *   with binary Heap structure as default heap and
    *   Dijkstra's algorithm of LEMON.
    *
    *   \b Notes on the current implementation
    *
    *   Experiments showed, that sparse networks (as street networks are) are best processed with a 2-heap
    *   and with the LEMON smart_graph regarding processing time and memory footprint. Furthermore adding arcs/nodes is possible
    *   on smart_graphs. We need this for modeling the addition of new nodes (start or end nodes) in the graph.
    *   Unfortunately removal of arcs is not possible with smart_graphs, so this implementation works with LEMON filters on arcs
    *   in order to model removing arcs (they are flagged with false in the filter map). Filter on Arcs come with a little performance
    *   penalty though.

    *   static_graph would have been better regarding processing time, but has a bigger memory footprint than smart_graphs.
    *   Furthermore you cannot add or remove arcs/nodes on static_graphs, so this would not be suitable for the application of adding new nodes to
    *   the graph.
    *
    *   See also:
    *   http://lemon.cs.elte.hu/pub/tutorial/a00015.html and
    *   http://blog.sommer-forst.de/2016/10/28/solving-the-shortest-path-problem-5-benchmarks/
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
            ///\brief Constructor
            ///\param directed: directed network (default) or unidirectional network (=doubled arcs)
            SPT_LEM(bool directed = true);
            ///\brief Empty Copy Constructor
            SPT_LEM (SPT_LEM const &) {}
            ///\brief Destructor
            ~SPT_LEM();
            /* LEMON friendly interface */
            ///\brief Loads the network into the core solver
            void LoadNet(const uint32_t nmax,  const uint32_t mmax,
                      //const lemon::FilterArcs<const netxpert::data::graph_t, const netxpert::data::graph_t::ArcMap<bool>>& sg,
                      //const netxpert::data::graph_t::ArcMap<netxpert::data::cost_t>& cm);
                      netxpert::data::filtered_graph_t* sg,
                      netxpert::data::graph_t::ArcMap<netxpert::data::cost_t>* cm);
            ///\brief Gets the count of the arcs of the internal graph
            const uint32_t GetArcCount();
            ///\brief Gets the count of the nodes of the internal graph
            const uint32_t GetNodeCount();
            ///\brief Solves the SPT Problem
            ///\param threshold: if that distance value has been exceeded, the Dijkstra search will stop
            ///\param bidirectional: use bidirectional dijkstra search or not
            void SolveSPT(netxpert::data::cost_t threshold = -1, bool bidirectional = true);
            ///\brief Sets the origin for the spt search
            void SetOrigin( netxpert::data::node_t _origin );
            ///\brief Sets the destionation node for the spt search
            void SetDest( netxpert::data::node_t _dest );
            ///\brief Tells if the given node has been reached from the origin node in the spt search
            ///\return true if reached, false if not reachable from the origin
            bool Reached( netxpert::data::node_t _node );
            ///\brief Gets all predecessor nodes from the destination node to the origin node
            ///\return the reverse path with nodes from destination to origin if found in the spt solver
            const std::vector<netxpert::data::node_t> GetPredecessors(netxpert::data::node_t _dest);
            ///\brief Gets the path represented as arcs from origin node to the destination node for the spt search
            const std::vector<netxpert::data::arc_t> GetPath(netxpert::data::node_t _dest);
            ///\brief Gets the distance from the origin node to the given destination node
            const netxpert::data::cost_t GetDist(netxpert::data::node_t _dest);
            /* end of LEMON friendly interface */

            /* Contraction Hierarchies */
            #if (defined NETX_ENABLE_CONTRACTION_HIERARCHIES)
            ///\brief Loads the contracted network into the core solver
            /// for use in contracted networks
            void LoadNet_CH(CHInterface<DefaultPriority>* _chm,
//                            graph_ch_t* _chg,
                            netxpert::data::graph_ch_t::ArcMap<netxpert::data::cost_t>* _chCostMap,
//                            filtered_graph_t::NodeMap<graph_ch_t::Node>* _nm);
                            netxpert::data::graph_t::NodeMap<netxpert::data::graph_ch_t::Node>* _nm,
                            netxpert::data::graph_ch_t::NodeMap<netxpert::data::graph_t::Node>* _cnm);
//                            netxpert::data::graph_ch_t::ArcMap<netxpert::data::graph_t::Arc>* _am);
            ///\brief Solves the SPT Problem with contraction hierarchies
            ///\warning for use in contracted networks
            void SolveSPT_CH();
            ///\brief Gets the path represented as arcs from origin node to the destination node for the spt search,
            ///\warning for use in contracted networks
            const std::vector<netxpert::data::arc_t> GetPath_CH();
            ///\brief Gets the distance from the origin node to the given destination node
            ///\warning for use in contracted networks
            const netxpert::data::cost_t GetDist_CH();
            ///\brief Tells if the given node has been reached from the origin node in the spt search
            ///\return true if reached, false if not reachable from the origin
            ///\warning for use in contracted networks
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
            #if (defined NETX_ENABLE_CONTRACTION_HIERARCHIES)
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
