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
//#include <lemon/adaptors.h>

#include <omp.h>
namespace netxpert {

    /**
    * \Class Solver for computing an Origin Destination Matrix
    */
    class OriginDestinationMatrix : public netxpert::ISolver
    {
        public:
            /** Default constructor */
            OriginDestinationMatrix(netxpert::cnfg::Config& cnfg);

            /** Default destructor */
            ~OriginDestinationMatrix() {}

            void Solve(std::string net);
            void Solve(netxpert::InternalNet& net);

            netxpert::cnfg::SPTAlgorithm GetAlgorithm() const;
            void SetAlgorithm(netxpert::cnfg::SPTAlgorithm mstAlgorithm);

            int GetSPTHeapCard() const;
            void SetSPTHeapCard(int heapCard);

            netxpert::cnfg::GEOMETRY_HANDLING GetGeometryHandling() const;
            void SetGeometryHandling(netxpert::cnfg::GEOMETRY_HANDLING geomHandling);

            void SetOrigins(std::vector<netxpert::data::node_t>& origs);
            /** Simple Wrapper for SWIG **/
            void SetOrigins(const std::vector<uint32_t>& origs) {
                std::vector<netxpert::data::node_t> newOrigs;
                for (auto& orig : origs)
                    newOrigs.push_back(this->net->GetNodeFromID(orig));

                this->SetOrigins(newOrigs);
            };
            //?
            void SetOrigins(std::vector<std::pair<netxpert::data::node_t, std::string>>& origs);
            std::vector<netxpert::data::node_t> GetOrigins() const;
            /** Simple Wrapper for SWIG **/
            std::vector<uint32_t> GetOriginIDs() const {
                std::vector<uint32_t> result;
                for (auto& orig : this->GetOrigins() ) {
                    result.push_back(this->net->GetNodeID(orig));
                }
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
            //?
            void SetDestinations(std::vector<std::pair<netxpert::data::node_t, std::string>>& dests);

            std::vector<netxpert::data::node_t> GetDestinations() const;
            /** Simple Wrapper for SWIG **/
            std::vector<uint32_t> GetDestinationIDs() const;

            std::vector<netxpert::data::node_t> GetReachedDests() const;
            /** Simple Wrapper for SWIG **/
            std::vector<uint32_t> GetReachedDestIDs() const;

            std::map<netxpert::data::ODPair, netxpert::data::CompressedPath> GetShortestPaths() const;
            std::map<netxpert::data::ODPair, netxpert::data::cost_t> GetODMatrix() const;

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
            std::vector<netxpert::data::node_t> originNodes;
            std::map<netxpert::data::ODPair, netxpert::data::CompressedPath> shortestPaths;
            std::map<netxpert::data::ODPair, double> odMatrix;
            netxpert::cnfg::GEOMETRY_HANDLING geometryHandling;
            netxpert::cnfg::SPTAlgorithm algorithm;
            std::shared_ptr<netxpert::core::ISPTree> spt;
            void solve (netxpert::InternalNet& net,
                        std::vector<netxpert::data::node_t>& originNodes,
                        std::vector<netxpert::data::node_t>& destinationNodes,
                        bool isDirected);
            bool validateNetworkData(netxpert::InternalNet& net,
				     std::vector<netxpert::data::node_t>& origs,
				     std::vector<netxpert::data::node_t>& dests);
            lemon::FilterArcs<netxpert::data::graph_t, netxpert::data::graph_t::ArcMap<bool>>
             convertInternalNetworkToSolverData(netxpert::InternalNet& net);
            void checkSPTHeapCard(uint32_t arcCount, uint32_t nodeCount);
    };
}
#endif // ODMATRIX_H
