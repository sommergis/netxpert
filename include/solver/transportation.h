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
            virtual ~Transportation() {}

            std::vector<unsigned int> GetOrigins() const;
            void SetOrigins(std::vector<unsigned int> origs);

            std::vector<unsigned int> GetDestinations() const;
            void SetDestinations(std::vector<unsigned int> dests);

            //std::unordered_map<ExtArcID, ExtODMatrixArc> GetExtODMatrix() const;
            //std::vector<ExtODMatrixArc> GetExtODMatrix() const;
            //void SetExtODMatrix(std::unordered_map<ExtArcID, ExtODMatrixArc> _extODMatrix);
            void SetExtODMatrix(std::vector<netxpert::data::ExtSPTreeArc> _extODMatrix);
            //void SetExtODMatrix(string _extODMatrixJSON);

            //std::vector<ExtNodeSupply> GetNodeSupply() const;
            void SetExtNodeSupply(std::vector<netxpert::data::ExtNodeSupply> _nodeSupply);
            //void SetNodeSupply(string _nodeSupplyJSON);

            std::unordered_map<netxpert::data::ODPair, netxpert::data::DistributionArc> GetDistribution() const;

            //simplified json output
            std::string GetSolverJSONResult() const;

            std::vector<netxpert::data::ExtDistributionArc> GetExtDistribution() const;
            std::string GetJSONExtDistribution() const;

            std::vector<netxpert::data::InternalArc> UncompressRoute(unsigned int orig, std::vector<unsigned int>& ends) const;

            //netxpert::Network* net;
            std::unique_ptr<Network> net;

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
            void Solve(netxpert::Network& net);

        private:

            std::vector<unsigned int> destinationNodes;
            std::vector<unsigned int> originNodes;

            std::unordered_map<netxpert::data::ODPair, double> odMatrix;
            //std::unordered_map<ExtArcID, ExtODMatrixArc> extODMatrix;
            std::vector<netxpert::data::ExtSPTreeArc> extODMatrix;
            std::unordered_map<netxpert::data::ODPair, netxpert::data::DistributionArc> distribution;
            std::unordered_map<netxpert::data::ExtNodeID, double> nodeSupply;
            std::vector<netxpert::data::ExtNodeSupply> extNodeSupply;

            std::pair<double,double> getFlowCostData(const std::vector<netxpert::data::FlowCost>& fc, const netxpert::data::ODPair& key) const;
    };
}
#endif // TRANSPORTATION_H
