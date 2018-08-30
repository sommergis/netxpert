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

#ifndef TRANSPORTATION_H
#define TRANSPORTATION_H

#include "mcflow.h"
#include "odmatrix.h"

namespace netxpert {

    /**
    * \brief Solver for the Transportation Problem
    * \todo check for inheritance of ODMatrix Solver
    */
    class Transportation : public netxpert::MinCostFlow
    {
        public:
            ///\brief Constructor
            Transportation(netxpert::cnfg::Config& cnfg);
            ///\brief Destructor
            ///
            ///\todo Do we need to clean up in destructor, because of new alloc of an instance of Network in Solve()?
            ~Transportation();
            ///\brief Sets the origin nodes
            void SetOrigins(std::vector<netxpert::data::node_t>& origs);
            ///\brief Sets the origin nodes per ID<br>
            /// Simple Wrapper for SWIG
            void SetOrigins(const std::vector<uint32_t>& origs) {
                std::vector<netxpert::data::node_t> newOrigs;
                for (auto& orig : origs)
                    newOrigs.push_back(this->net->GetNodeFromID(orig));

                this->SetOrigins(newOrigs);
            };
            ///\brief Gets the origin nodes
            std::vector<netxpert::data::node_t> GetOrigins() const;
            ///\brief Gets the internal IDs of the origin nodes<br>
            /// Simple Wrapper for SWIG
            std::vector<uint32_t> GetOriginIDs() const {
                std::vector<uint32_t> result;
                for (auto& orig : this->GetOrigins() )
                    result.push_back(this->net->GetNodeID(orig));

                return result;
            };
            ///\brief Sets the destination nodes
            void SetDestinations(std::vector<netxpert::data::node_t>& dests);
            ///\brief Sets the destination nodes per IDs<br>
            /// Simple Wrapper for SWIG
            void SetDestinations(const std::vector<uint32_t>& dests) {
                std::vector<netxpert::data::node_t> newDests;
                for (auto& e : dests)
                    newDests.push_back(this->net->GetNodeFromID(e));

                this->SetDestinations(newDests);
            };
            ///\brief Gets the destination nodes
            std::vector<netxpert::data::node_t> GetDestinations() const;
            ///\brief Gets the internal IDs of the destination nodes<br>
            /// Simple Wrapper for SWIG
            std::vector<uint32_t> GetDestinationIDs() const {
                std::vector<uint32_t> result;
                for (auto& n : this->destinationNodes)
                    result.push_back(this->net->GetNodeID(n));

                return result;
            }

            //std::unordered_map<ExtArcID, ExtODMatrixArc> GetExtODMatrix() const;
            //std::vector<ExtODMatrixArc> GetExtODMatrix() const;
            //void SetExtODMatrix(std::unordered_map<ExtArcID, ExtODMatrixArc> _extODMatrix);
            ///\brief Sets an external Origin Destination Matrix as Input for the Transportation Solver
            void SetExtODMatrix(std::vector<netxpert::data::ExtSPTreeArc> _extODMatrix);
            //void SetExtODMatrix(string _extODMatrixJSON);
            ///\brief Gets a pointer to the internal network
            netxpert::data::InternalNet* GetNetwork();
            //std::vector<ExtNodeSupply> GetNodeSupply() const;
            ///\brief Sets the external Mapping of node IDs and their supply or demand values
            void SetExtNodeSupply(std::vector<netxpert::data::ExtNodeSupply> _nodeSupply);
            //void SetNodeSupply(string _nodeSupplyJSON);
            ///\brief Gets the computed distribution of the Transportation Solver (internal types and IDs)
            std::map<netxpert::data::ODPair, netxpert::data::DistributionArc> GetDistribution() const;
            //simplified json output
            ///\brief Gets the computed optimum and distribution of the Transportation Solver as JSON string (original IDs)
            ///\todo check for GetResultsAsJSON() in other solvers! rename? or implement the other variant
            /// requires a special data struct TransportationResult for serialization
            std::string GetSolverJSONResult() const;
            ///\brief Gets the computed distribution of the Transportation Solver as simple variant (original IDs)
            std::vector<netxpert::data::ExtDistributionArc> GetExtDistribution() const;
            ///\brief Gets the computed distribution of the Transportation Solver as JSON string (original IDs)
            ///\todo check for GetResultsAsJSON() in other solvers!
            /// requires a special data struct ExtDistributionArc for serialization
            std::string GetJSONExtDistribution() const;
            ///\brief Saves the results of the odm solver with the configured RESULT_DB_TYPE (SpatiaLite, FileGDB).
            ///\todo Implement JSON RESULT_DB_TYPE
            void SaveResults(const std::string& resultTableName,
                             const netxpert::data::ColumnMap& cmap) const;

            /**
            * \brief Computes the transportation problem.
            *
            * Solves the Transportation Problem with the predefined ODMatrix and NodeSupply property. Solves on pure
            * attribute data only i.e. output is always without geometry. (GEOMETRY_HANDLING is always set to "NoGeometry")
            */
            void Solve();
            /**
            * \brief Computes the transportation problem on the given network.
            *
            *  Solves the Transportation Problem with the given network and all origin and destination nodes.
            *  Uses the netXpert OriginDestinationMatrix Solver internally.
            */
            void Solve(netxpert::data::InternalNet& net);

        private:
            //raw pointer ok, no dynamic allocation (new())
            //smart pointers will not work, because Network is passed by reference and
            //shall be assigned to the class member this->net
            //with smart pointers there are double frees on clean up -> memory errors
            //raw pointers will not leak int this case even without delete in the deconstructor
            netxpert::data::InternalNet* net;

            std::vector<netxpert::data::node_t> destinationNodes;
            std::vector<netxpert::data::node_t> originNodes;
            std::map<netxpert::data::ODPair, netxpert::data::cost_t> odMatrix;
            std::vector<netxpert::data::ExtSPTreeArc> extODMatrix;
            std::map<netxpert::data::ODPair, netxpert::data::DistributionArc> distribution;
            std::map<netxpert::data::ExtNodeID, netxpert::data::supply_t> nodeSupply;
            std::vector<netxpert::data::ExtNodeSupply> extNodeSupply;
    };
}
#endif // TRANSPORTATION_H
