#ifndef TRANSPORTATION_H
#define TRANSPORTATION_H

#include "mcflow.h"

namespace netxpert {
    //TODO: check for inheritance of ODMatrix Solver
    /**
    * \Class Solver for the Transportation Problem
    */
    class Transportation : public MinCostFlow
    {
        public:
            Transportation(Config& cnfg);
            virtual ~Transportation();

            std::vector<unsigned int> GetOrigins() const;
            void SetOrigins(std::vector<unsigned int>  origs);

            std::vector<unsigned int> GetDestinations() const;
            void SetDestinations(std::vector<unsigned int>& dests);

            std::unordered_map<ExtArcID, ExtODMatrixArc> GetExtODMatrix() const;
            void SetExtODMatrix(std::unordered_map<ExtArcID, ExtODMatrixArc> _extODMatrix);

            std::unordered_map<ExtNodeID, double> GetNodeSupply() const;
            void SetNodeSupply(std::unordered_map<ExtNodeID, double> _nodeSupply);

            std::unordered_map<ODPair, DistributionArc> GetDistribution() const;
            std::vector<InternalArc> UncompressRoute(unsigned int orig, std::vector<unsigned int>& ends) const;

            std::unique_ptr<Network> network;

            /**
            * Solves the Transportation Problem with the defined ODMatrix and NodeSupply property. Solves on pure
            * attribute data only i.e. output is always without geometry. (GEOMETRY_HANDLING is always set to "NoGeometry")
            */
            void Solve();
            /**
            *  Solves the Transportation Problem with the given network and all origin and destination nodes.
            *  Uses the NetXpert OriginDestinationMatrix Solver internally.
            */
            void Solve(Network& net);

        private:

            std::vector<unsigned int> destinationNodes;
            std::vector<unsigned int> originNodes;

            std::unordered_map<ODPair, double> odMatrix;
            std::unordered_map<ExtArcID, ExtODMatrixArc> extODMatrix;
            std::unordered_map<ODPair, DistributionArc> distribution;
            std::unordered_map<ExtNodeID, double> nodeSupply;

            std::pair<double,double> getFlowCostData(const std::vector<FlowCost>& fc, const ODPair& key) const;
    };
}
#endif // TRANSPORTATION_H
