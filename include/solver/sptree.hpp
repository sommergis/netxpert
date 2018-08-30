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

#include "isolver.hpp"
#include "isptree.hpp"
#include "sptlem.hpp"
//#include "sptbgl.hpp"
#include "data.hpp"
#include "lemon-net.hpp"

namespace netxpert {

    /**
    * \brief Solver for the Shortest Path Tree Problem
    *
    * \li Initialization with a netxpert::cnfg::Config object in Constructor
    * \li call of SetOrigin() method
    * \li optional: call of SetDestinations() method defines 1-n destinations
    *    (if no destination is set with this method, the whole shortest path tree is computed from the origin)
    * \li call of \ref Solve( \ref netxpert::data::InternalNet ) method for the computation; the network must be constructed prior to this step
    * \li optional: GetOptimum() for getting the overall optimum
    * \li optional: GetShortestPaths() for getting the actual result of the SPT computation
    * \li optional: SaveResults() or alternate GetResultsAsJSON()
    *
    * all other setters and getters can be used for overriding the configuration from the constructor
    **/
    class ShortestPathTree : public netxpert::ISolver
    {
        public:
            ///\brief Constructor
            ShortestPathTree(netxpert::cnfg::Config& cnfg);
            ///\brief Empty destructor
            ~ShortestPathTree() {}
            ///\brief Computes the shortest path problem
            ///\warning Not implemented! Should be removed.
            void Solve(std::string net);
            ///\brief Computes the shortest path problem on the given network.
            ///
            /// At least an origin has to be set with \ref SetOrigin() prior calling this method.
            /// Normally the destinations should be set with \ref SetDestinations().
            void Solve(netxpert::data::InternalNet& net);
            ///\brief Gets the type of spt algorithm used in the solver
            const netxpert::cnfg::SPTAlgorithm GetAlgorithm() const;
            ///\brief Sets the type of spt algorithm to use in the solver
            void SetAlgorithm(netxpert::cnfg::SPTAlgorithm sptAlgorithm);
            ///\brief Gets the heap ariety of the spt solver
            const int GetSPTHeapCard() const;
            ///\brief Sets the heap ariety for the spt solver
            ///\deprecated Unclear if supported in the future
            void SetSPTHeapCard(int heapCard);
            ///\brief Gets the handling of the geometry output
            const netxpert::cnfg::GEOMETRY_HANDLING GetGeometryHandling() const;
            ///\brief Sets the handling of the geometry output
            void SetGeometryHandling(netxpert::cnfg::GEOMETRY_HANDLING geomHandling);
            ///\brief Gets the origin node
            const netxpert::data::node_t GetOrigin() const;
            ///\brief Gets the internal ID of the origin node
            /// Simple Wrapper for SWIG
            const uint32_t GetOriginID() const {
                return this->net->GetNodeID(this->originNode);
            };
            ///\brief Sets the origin node
            void SetOrigin(const netxpert::data::node_t orig);
            ///\brief Sets the origin node per ID
            /// Simple Wrapper for SWIG
            void SetOrigin(const uint32_t orig) {
                this->SetOrigin(this->net->GetNodeFromID(orig));
            };
            ///\brief Gets the destination nodes
            std::vector<netxpert::data::node_t> GetDestinations() const;
            ///\brief Gets the internal IDs of all destination nodes
            /// Simple Wrapper for SWIG
            std::vector<uint32_t> GetDestinationIDs() const;
            ///\brief Sets the destination nodes
            void SetDestinations(const std::vector<netxpert::data::node_t>& dests);
            ///\brief Sets the destination nodes per IDs
            /// Simple Wrapper for SWIG
            void SetDestinations(const std::vector<uint32_t>& dests);
            ///\brief Gets the internal node objects of all reached nodes
            std::vector<netxpert::data::node_t> GetReachedDests() const;
            ///\brief Gets the internal node IDs of all reached nodes
            std::vector<uint32_t> GetReachedDestIDs() const;
            ///\brief Gets all shortest paths of the spt solver
            std::map<netxpert::data::ODPair, netxpert::data::CompressedPath> GetShortestPaths() const;
            ///\brief Gets the overall optimum of the solver
            const double GetOptimum() const;
            ///\brief Saves the results of the SPT solver with the configured RESULT_DB_TYPE (SpatiaLite, FileGDB or JSON).
            void SaveResults(const std::string& resultTableName,
                             const netxpert::data::ColumnMap& cmap);
            ///\brief Gets the SPT results as JSON String with the original from and to nodes
            const std::string GetResultsAsJSON();

        private:
            //raw pointer ok, no dynamic allocation (new())
            //smart pointers will not work, because Network is passed by reference and
            //shall be assigned to the class member this->net
            //with smart pointers there are double frees on clean up -> memory errors
            //raw pointers will not leak in this case even without delete in the deconstructor
            netxpert::data::InternalNet* net;
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
            void solve (netxpert::data::InternalNet& net,
                        netxpert::data::node_t originNode,
                        netxpert::data::node_t destinationNode,
                        bool isDirected);
            void solve (netxpert::data::InternalNet& net,
                        netxpert::data::node_t originNode,
                        std::vector<netxpert::data::node_t> destinationNodes,
                        bool isDirected);
            void solve (netxpert::data::InternalNet& net,
                        netxpert::data::node_t originNode,
                        bool isDirected);
            bool validateNetworkData(netxpert::data::InternalNet& net,
                                     netxpert::data::node_t orig);
            bool validateNetworkData(netxpert::data::InternalNet& net,
                                     netxpert::data::node_t orig,
                                     netxpert::data::node_t dest);
            bool validateNetworkData(netxpert::data::InternalNet& net,
                                     netxpert::data::node_t orig,
                                     std::vector<netxpert::data::node_t>& dests);

            lemon::FilterArcs<netxpert::data::graph_t, netxpert::data::graph_t::ArcMap<bool>>
             convertInternalNetworkToSolverData(netxpert::data::InternalNet& net);

            void checkSPTHeapCard(uint32_t arcCount, uint32_t nodeCount);

            std::string
             processTotalArcIDs();

            /*double buildCompressedRoute(std::vector<netxpert::data::node_t>& route, netxpert::data::node_t orig, netxpert::data::node_t dest,
                                            std::unordered_map<netxpert::data::node_t, netxpert::data::node_t>& arcPredescessors);

            double getArcCost(const netxpert::data::InternalArc& arc);*/
    };
}
#endif // SPTREE_H
