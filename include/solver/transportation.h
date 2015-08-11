#ifndef TRANSPORTATION_H
#define TRANSPORTATION_H

#include "mcflow.h"
#include "odmatrix.h"

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
            void SetOrigins(std::vector<unsigned int> origs);

            std::vector<unsigned int> GetDestinations() const;
            void SetDestinations(std::vector<unsigned int> dests);

            //std::unordered_map<ExtArcID, ExtODMatrixArc> GetExtODMatrix() const;
            //std::vector<ExtODMatrixArc> GetExtODMatrix() const;
            //void SetExtODMatrix(std::unordered_map<ExtArcID, ExtODMatrixArc> _extODMatrix);
            void SetExtODMatrix(std::vector<ExtODMatrixArc> _extODMatrix);
            //void SetExtODMatrix(string _extODMatrixJSON);

            //std::vector<ExtNodeSupply> GetNodeSupply() const;
            void SetExtNodeSupply(std::vector<ExtNodeSupply> _nodeSupply);
            //void SetNodeSupply(string _nodeSupplyJSON);

            std::unordered_map<ODPair, DistributionArc> GetDistribution() const;

            //simplified json output
            std::string GetSolverJSONResult() const;

            std::vector<ExtDistributionArc> GetExtDistribution() const;
            std::string GetJSONExtDistribution() const;

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
            //std::unordered_map<ExtArcID, ExtODMatrixArc> extODMatrix;
            std::vector<ExtODMatrixArc> extODMatrix;
            std::unordered_map<ODPair, DistributionArc> distribution;
            std::unordered_map<ExtNodeID, double> nodeSupply;
            std::vector<ExtNodeSupply> extNodeSupply;

            std::pair<double,double> getFlowCostData(const std::vector<FlowCost>& fc, const ODPair& key) const;
    };
}
#endif // TRANSPORTATION_H
