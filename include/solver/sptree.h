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

#ifndef SPTREE_H
#define SPTREE_H

#include "isolver.h"
#include "isptree.h"
#include "sptlem.h"
//#include "sptbgl.h"
#include "data.h"
#include "lemon-net.h"
//#include <lemon/adaptors.h>

namespace netxpert {

    /**
    * \Class Solver for the Shortest Path Tree Problem
    */
    class ShortestPathTree : public netxpert::ISolver
    {
        public:
            /** Default constructor */
            ShortestPathTree(netxpert::cnfg::Config& cnfg);

            /** Default destructor */
            ~ShortestPathTree() {}

            void Solve(std::string net);
            void Solve(netxpert::InternalNet& net);

            netxpert::cnfg::SPTAlgorithm GetAlgorithm() const;
            void SetAlgorithm(netxpert::cnfg::SPTAlgorithm sptAlgorithm);

            int GetSPTHeapCard() const;
            void SetSPTHeapCard(int heapCard);

            netxpert::cnfg::GEOMETRY_HANDLING GetGeometryHandling() const;
            void SetGeometryHandling(netxpert::cnfg::GEOMETRY_HANDLING geomHandling);

            const netxpert::data::node_t GetOrigin() const;
            /** Simple Wrapper for SWIG **/
            const uint32_t GetOriginID() const {
                return this->net->GetNodeID(this->originNode);
            };
            void SetOrigin(const netxpert::data::node_t orig);
            /** Simple Wrapper for SWIG **/
            void SetOrigin(const uint32_t orig) {
                this->SetOrigin(this->net->GetNodeFromID(orig));
            };

            std::vector<netxpert::data::node_t> GetDestinations() const;
            /** Simple Wrapper for SWIG **/
            std::vector<uint32_t> GetDestinationIDs() const;
            void SetDestinations(const std::vector<netxpert::data::node_t>& dests);
            /** Simple Wrapper for SWIG **/
            void SetDestinations(const std::vector<uint32_t>& dests);

            std::vector<netxpert::data::node_t> GetReachedDests() const;
            std::vector<uint32_t> GetReachedDestIDs() const;
            std::map<netxpert::data::ODPair, netxpert::data::CompressedPath> GetShortestPaths() const;

            const double GetOptimum() const;

            void SaveResults(const std::string& resultTableName,
                             const netxpert::data::ColumnMap& cmap) const;

        private:
            //raw pointer ok, no dynamic allocation (new())
            //smart pointers will not work, because Network is passed by reference and
            //shall be assigned to the class member this->net
            //with smart pointers there are double frees on clean up -> memory errors
            //raw pointers will not leak in this case even without delete in the deconstructor
            netxpert::InternalNet* net;
            bool isDirected;
            int sptHeapCard;
            double optimum;
            netxpert::cnfg::Config NETXPERT_CNFG;
            std::vector<netxpert::data::node_t> destinationNodes;
            std::vector<netxpert::data::node_t> reachedDests;
            netxpert::data::node_t originNode;
            std::map<netxpert::data::ODPair, netxpert::data::CompressedPath> shortestPaths;
            netxpert::cnfg::GEOMETRY_HANDLING geometryHandling;
            netxpert::cnfg::SPTAlgorithm algorithm;
            std::unique_ptr<netxpert::core::ISPTree> spt;
            void solve (netxpert::InternalNet& net,
                        netxpert::data::node_t originNode,
                        netxpert::data::node_t destinationNode,
                        bool isDirected);
            void solve (netxpert::InternalNet& net,
                        netxpert::data::node_t originNode,
                        std::vector<netxpert::data::node_t> destinationNodes,
                        bool isDirected);
            void solve (netxpert::InternalNet& net,
                        netxpert::data::node_t originNode,
                        bool isDirected);
            bool validateNetworkData(netxpert::InternalNet& net,
                                     netxpert::data::node_t orig);
            bool validateNetworkData(netxpert::InternalNet& net,
                                     netxpert::data::node_t orig,
                                     netxpert::data::node_t dest);
            bool validateNetworkData(netxpert::InternalNet& net,
                                     netxpert::data::node_t orig,
                                     std::vector<netxpert::data::node_t>& dests);

            lemon::FilterArcs<netxpert::data::graph_t, netxpert::data::graph_t::ArcMap<bool>>
             convertInternalNetworkToSolverData(netxpert::InternalNet& net);

            void checkSPTHeapCard(uint32_t arcCount, uint32_t nodeCount);
            /*double buildCompressedRoute(std::vector<netxpert::data::node_t>& route, netxpert::data::node_t orig, netxpert::data::node_t dest,
                                            std::unordered_map<netxpert::data::node_t, netxpert::data::node_t>& arcPredescessors);

            double getArcCost(const netxpert::data::InternalArc& arc);*/
    };
}
#endif // SPTREE_H
