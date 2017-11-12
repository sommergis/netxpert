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

#ifndef ISOLINES_H
#define ISOLINES_H

#include "isolver.h"
#include "isptree.h"
#include "sptlem.h"
#include "data.h"

namespace netxpert {

    /**
    * \brief Solver for calculating Isolines, i.e. bands of accessible areas in a network.
    *
    * \warning Experimental!
    *
    * Concept: solve the 1 - all SPT problem and cut off the routes at the defined cut off values;
    *          do this for all origins.
    *
    */
    class Isolines : public netxpert::ISolver
    {
        public:
            Isolines(netxpert::cnfg::Config& cnfg);
            virtual ~Isolines() {}
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
                std::vector<netxpert::data::node_t> result;
                for (auto& orig : origs) {
                    result.push_back(this->net->GetNodeFromID(orig));
                }
                this->SetOrigins(result);
            };
            std::vector<netxpert::data::node_t> GetOrigins() const;
            /** Simple Wrapper for SWIG **/
            std::vector<uint32_t> GetOriginIDs() const {
                std::vector<uint32_t> result;
                for (auto& orig : this->GetOrigins() ) {
                    result.push_back(this->net->GetNodeID(orig));
                }
                return result;
            };

            std::map<netxpert::data::ExtNodeID, std::vector<double> > GetCutOffs();
            void SetCutOffs(std::map<netxpert::data::ExtNodeID, std::vector<double> >& cutOffs);

            const double GetOptimum() const;

            void SaveResults(const std::string& resultTableName,
                             const netxpert::data::ColumnMap& cmap) const;

            std::map<netxpert::data::ODPair, netxpert::data::CompressedPath> GetShortestPaths() const;

        private:
            netxpert::InternalNet* net; //raw pointer ok, no dynamic allocation (new())
            bool isDirected;
            int sptHeapCard;
            double optimum;
            std::vector<netxpert::data::node_t> originNodes;
            std::map<netxpert::data::ODPair, netxpert::data::CompressedPath> shortestPaths;
            std::unordered_map<netxpert::data::ExtNodeID, std::vector<double> > cutOffs;
            netxpert::cnfg::GEOMETRY_HANDLING geometryHandling;
            netxpert::cnfg::SPTAlgorithm algorithm;
            std::unique_ptr<netxpert::core::ISPTree> spt;
            void solve (netxpert::InternalNet& net, std::vector<netxpert::data::node_t> originNodes, bool isDirected);
            bool validateNetworkData(netxpert::InternalNet& net, std::vector<netxpert::data::node_t>& origs);

            lemon::FilterArcs<netxpert::data::graph_t, netxpert::data::graph_t::ArcMap<bool>>
             convertInternalNetworkToSolverData(netxpert::InternalNet& net);

            void checkSPTHeapCard(uint32_t arcCount, uint32_t nodeCount);

            netxpert::cnfg::Config NETXPERT_CNFG;
    };
}
#endif // ISOLINES_H
