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

#ifndef ODMATRIX_H
#define ODMATRIX_H

#include "isolver.h"
#include "isptree.h"
#include "sptlem.h"
//#include "sptbgl.h"
#include "data.h"
#include "lemon-net.h"
#include <omp.h>

namespace netxpert {

    /**
    * \brief Solver for computing an Origin Destination Matrix
    * \todo Check for duplicate code -> SPT Solver - Inheritance?
    */
    class OriginDestinationMatrix : public netxpert::ISolver
    {
        public:
            ///\brief Constructor
            OriginDestinationMatrix(netxpert::cnfg::Config& cnfg);
            ///\brief Empty destructor
            ~OriginDestinationMatrix() {}
            ///\brief Computes the origin destination matrix
            ///\warning Not implemented! Should be removed.
            void Solve(std::string net);
            ///\brief Computes the origin destination matrix on the given network.
            ///
            /// Origins and destinations have to be set with \ref SetOrigins() and \ref SetDestinations()
            /// prior calling this method.
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
            const netxpert::cnfg::GEOMETRY_HANDLING GetGeometryHandling() const;
            ///\brief Sets the handling of the geometry output
            void SetGeometryHandling(netxpert::cnfg::GEOMETRY_HANDLING geomHandling);
            ///\brief Gets the origin nodes
            std::vector<netxpert::data::node_t> GetOrigins() const;
            ///\brief Gets the internal node IDs of the origin nodes
            /// Simple Wrapper for SWIG
            std::vector<uint32_t> GetOriginIDs() const {
                std::vector<uint32_t> result;
                for (auto& orig : this->GetOrigins() ) {
                    result.push_back(this->net->GetNodeID(orig));
                }
                return result;
            };
            ///\brief Sets the origin nodes
            void SetOrigins(std::vector<netxpert::data::node_t>& origs);
            ///\brief Sets the origin nodes per ID
            /// Simple Wrapper for SWIG
            void SetOrigins(const std::vector<uint32_t>& origs) {
                std::vector<netxpert::data::node_t> newOrigs;
                for (auto& orig : origs)
                    newOrigs.push_back(this->net->GetNodeFromID(orig));

                this->SetOrigins(newOrigs);
            };
            ///\deprecated Use unclear!
            void SetOrigins(std::vector<std::pair<netxpert::data::node_t, std::string>>& origs);
            ///\brief Sets the destination nodes
            void SetDestinations(std::vector<netxpert::data::node_t>& dests);
            ///\brief Sets the destination nodes per IDs
            /// Simple Wrapper for SWIG
            void SetDestinations(const std::vector<uint32_t>& dests) {
                std::vector<netxpert::data::node_t> newDests;
                for (auto& e : dests)
                    newDests.push_back(this->net->GetNodeFromID(e));

                this->SetDestinations(newDests);
            };
            ///\deprecated Use unclear!
            void SetDestinations(std::vector<std::pair<netxpert::data::node_t, std::string>>& dests);
            ///\brief Gets the destination nodes
            std::vector<netxpert::data::node_t> GetDestinations() const;
            ///\brief Gets the internal IDs of the destination nodes
            /// Simple Wrapper for SWIG
            std::vector<uint32_t> GetDestinationIDs() const;
            ///\brief Gets the internal node objects of all reached nodes
            std::vector<netxpert::data::node_t> GetReachedDests() const;
            ///\brief Gets the internal node IDs of all reached nodes
            /// Simple Wrapper for SWIG
            std::vector<uint32_t> GetReachedDestIDs() const;
            ///\brief Gets all shortest paths of the odm solver
            std::map<netxpert::data::ODPair, netxpert::data::CompressedPath> GetShortestPaths() const;
            ///\brief Gets the origin destination matrix of the odm solver
            std::map<netxpert::data::ODPair, netxpert::data::cost_t> GetODMatrix() const;
            ///\brief Gets the overall optimum of the solver
            const double GetOptimum() const;
            ///\brief Saves the results of the odm solver with the configured RESULT_DB_TYPE (SpatiaLite, FileGDB).
            ///\todo Implement JSON RESULT_DB_TYPE
            void SaveResults(const std::string& resultTableName,
                             const netxpert::data::ColumnMap& cmap) const;


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
            std::vector<netxpert::data::node_t> originNodes;
            std::map<netxpert::data::ODPair, netxpert::data::CompressedPath> shortestPaths;
            std::map<netxpert::data::ODPair, double> odMatrix;
            netxpert::cnfg::GEOMETRY_HANDLING geometryHandling;
            netxpert::cnfg::SPTAlgorithm algorithm;
            std::shared_ptr<netxpert::core::ISPTree> spt;
            void solve (netxpert::data::InternalNet& net,
                        std::vector<netxpert::data::node_t>& originNodes,
                        std::vector<netxpert::data::node_t>& destinationNodes,
                        bool isDirected);
            bool validateNetworkData(netxpert::data::InternalNet& net,
				     std::vector<netxpert::data::node_t>& origs,
				     std::vector<netxpert::data::node_t>& dests);
            lemon::FilterArcs<netxpert::data::graph_t, netxpert::data::graph_t::ArcMap<bool>>
             convertInternalNetworkToSolverData(netxpert::data::InternalNet& net);
            void checkSPTHeapCard(uint32_t arcCount, uint32_t nodeCount);
    };
}
#endif // ODMATRIX_H
