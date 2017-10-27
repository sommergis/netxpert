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

    //TODO: check for inheritance of ODMatrix Solver
    /**
    * \Class Solver for the Transportation Problem
    */
    class Transportation : public netxpert::MinCostFlow
    {
        public:
            Transportation(netxpert::cnfg::Config& cnfg);
            // we need a dctor, because of new alloc of an instance of Network in Solve()
            ~Transportation();

            void SetOrigins(std::vector<netxpert::data::node_t>& origs);
            /** Simple Wrapper for SWIG **/
            void SetOrigins(const std::vector<uint32_t>& origs) {
                std::vector<netxpert::data::node_t> newOrigs;
                for (auto& orig : origs)
                    newOrigs.push_back(this->net->GetNodeFromID(orig));

                this->SetOrigins(newOrigs);
            };

            std::vector<netxpert::data::node_t> GetOrigins() const;
            /** Simple Wrapper for SWIG **/
            std::vector<uint32_t> GetOriginIDs() const {
                std::vector<uint32_t> result;
                for (auto& orig : this->GetOrigins() )
                    result.push_back(this->net->GetNodeID(orig));

                return result;
            };

            void SetDestinations(std::vector<netxpert::data::node_t>& dests);
            /** Simple Wrapper for SWIG **/
            void SetDestinations(const std::vector<uint32_t>& dests) {
                std::vector<netxpert::data::node_t> newDests;
                for (auto& e : dests)
                    newDests.push_back(this->net->GetNodeFromID(e));

                this->SetDestinations(newDests);
            };

            std::vector<netxpert::data::node_t> GetDestinations() const;
            /** Simple Wrapper for SWIG **/
            std::vector<uint32_t> GetDestinationIDs() const {
                std::vector<uint32_t> result;
                for (auto& n : this->destinationNodes)
                    result.push_back(this->net->GetNodeID(n));

                return result;
            }

            //std::unordered_map<ExtArcID, ExtODMatrixArc> GetExtODMatrix() const;
            //std::vector<ExtODMatrixArc> GetExtODMatrix() const;
            //void SetExtODMatrix(std::unordered_map<ExtArcID, ExtODMatrixArc> _extODMatrix);
            void SetExtODMatrix(std::vector<netxpert::data::ExtSPTreeArc> _extODMatrix);
            //void SetExtODMatrix(string _extODMatrixJSON);

            netxpert::InternalNet* GetNetwork();

            //std::vector<ExtNodeSupply> GetNodeSupply() const;
            void SetExtNodeSupply(std::vector<netxpert::data::ExtNodeSupply> _nodeSupply);
            //void SetNodeSupply(string _nodeSupplyJSON);

            std::map<netxpert::data::ODPair, netxpert::data::DistributionArc> GetDistribution() const;

            //simplified json output
            std::string GetSolverJSONResult() const;

            std::vector<netxpert::data::ExtDistributionArc> GetExtDistribution() const;
            std::string GetJSONExtDistribution() const;

            void SaveResults(const std::string& resultTableName, const netxpert::data::ColumnMap& cmap) const;

            /**
            * Solves the Transportation Problem with the defined ODMatrix and NodeSupply property. Solves on pure
            * attribute data only i.e. output is always without geometry. (GEOMETRY_HANDLING is always set to "NoGeometry")
            */
            void Solve();
            /**
            *  Solves the Transportation Problem with the given network and all origin and destination nodes.
            *  Uses the NetXpert OriginDestinationMatrix Solver internally.
            */
            void Solve(netxpert::InternalNet& net);

        private:
            //raw pointer ok, no dynamic allocation (new())
            //smart pointers will not work, because Network is passed by reference and
            //shall be assigned to the class member this->net
            //with smart pointers there are double frees on clean up -> memory errors
            //raw pointers will not leak int this case even without delete in the deconstructor
            netxpert::InternalNet* net;

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
