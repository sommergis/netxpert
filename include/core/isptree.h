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
    /**
    * \brief Core solvers of netXpert
    **/
    namespace core {
    /**
    * \brief Abstract Class (Interface) for all Shortest Path Tree Solvers in netxpert core.
    */
    class ISPTree
    {
        public:
            /** Default destructor */
            virtual ~ISPTree() {}

            /* LEMON friendly interface */
            virtual void LoadNet(const uint32_t nmax,  const uint32_t mmax,
                                    lemon::FilterArcs<netxpert::data::graph_t,
                                              netxpert::data::graph_t::ArcMap<bool>>* sg,
                      netxpert::data::graph_t::ArcMap<netxpert::data::cost_t>* cm)=0;
            virtual const uint32_t GetArcCount()=0;
            virtual const uint32_t GetNodeCount()=0;
            virtual void SolveSPT(netxpert::data::cost_t treshold = -1, bool bidirectional = false )=0;
            virtual void SetOrigin( netxpert::data::node_t NewOrg )=0;
            virtual void SetDest( netxpert::data::node_t NewDst )=0;
            virtual bool Reached( netxpert::data::node_t NodeID )=0;
            virtual const std::vector<netxpert::data::node_t> GetPredecessors(netxpert::data::node_t _dest)=0;
            virtual const std::vector<netxpert::data::arc_t> GetPath(netxpert::data::node_t _dest)=0;
            virtual const netxpert::data::cost_t GetDist(netxpert::data::node_t _dest)=0;
            /* end of LEMON friendly interface */

    };
} //namespace core
} //namespace netxpert
#endif // ISPTREE_H
