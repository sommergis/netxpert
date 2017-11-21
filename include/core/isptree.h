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

#ifndef ISPTREE_H
#define ISPTREE_H

#include <memory>
#include <vector>
#include "lemon-net.h"

namespace netxpert {

    namespace core {
    /**
    * \brief Pure virtual (i.e. abstract) class for all Shortest Path Tree Solvers in netxpert core.
    */
    class ISPTree
    {
        public:
            /// Virtual Destructor
            virtual ~ISPTree() {}

            /* LEMON friendly interface */
            ///\brief Loads the network into the core solver
            virtual void LoadNet(const uint32_t nmax,  const uint32_t mmax,
                                    lemon::FilterArcs<netxpert::data::graph_t,
                                              netxpert::data::graph_t::ArcMap<bool>>* sg,
                      netxpert::data::graph_t::ArcMap<netxpert::data::cost_t>* cm)=0;
            ///\brief Gets the count of the arcs of the internal graph
            virtual const uint32_t GetArcCount()=0;
            ///\brief Gets the count of the nodes of the internal graph
            virtual const uint32_t GetNodeCount()=0;
            ///\brief Solves the SPT problem
            virtual void SolveSPT(netxpert::data::cost_t treshold = -1, bool bidirectional = false )=0;
            ///\brief Sets the origin
            virtual void SetOrigin( netxpert::data::node_t NewOrg )=0;
            ///\brief Solves the destination
            virtual void SetDest( netxpert::data::node_t NewDst )=0;
            ///\brief Checks if the given node has been reached in the calculation
            virtual bool Reached( netxpert::data::node_t NodeID )=0;
            ///\brief Gets the predecessor nodes of the given node
            virtual const std::vector<netxpert::data::node_t> GetPredecessors(netxpert::data::node_t _dest)=0;
            ///\brief Gets the path from the given destination node to the origin
            virtual const std::vector<netxpert::data::arc_t> GetPath(netxpert::data::node_t _dest)=0;
            ///\brief Gets the distance from the origin node to the given destination node
            virtual const netxpert::data::cost_t GetDist(netxpert::data::node_t _dest)=0;
            /* end of LEMON friendly interface */

    };
} //namespace core
} //namespace netxpert
#endif // ISPTREE_H
