#ifndef IMINCOSTFLOW_H
#define IMINCOSTFLOW_H

#include <stdint.h>
#include <memory>
#include <vector>
#include "lemon/concepts/path.h"
#include "lemon/adaptors.h"
#include "lemon-net.h"

namespace netxpert {
    namespace core {
    /**
    * \Class Abstract Class (Interface) for all Minimum Cost Flow Problem Solvers in netxpert core.
    */
    class IMinCostFlow
    {
        public:
            virtual ~IMinCostFlow(){}

            /* LEMON friendly interface */
            virtual void LoadNet(const uint32_t nmax,  const uint32_t mmax,
                      lemon::FilterArcs<netxpert::data::graph_t,
                                              netxpert::data::graph_t::ArcMap<bool>>* sg,
                      netxpert::data::graph_t::ArcMap<netxpert::data::cost_t>* _costMap,
                      netxpert::data::graph_t::ArcMap<netxpert::data::capacity_t>* _capMap,
                      netxpert::data::graph_t::NodeMap<netxpert::data::supply_t>* _supplyMap)=0;
            virtual const uint32_t GetArcCount()=0;
            virtual const uint32_t GetNodeCount()=0;
            virtual void SolveMCF()=0;
            virtual const double GetOptimum() const =0;
            virtual std::vector<netxpert::data::arc_t> GetMCFArcs()=0;
            virtual netxpert::data::graph_t::ArcMap<netxpert::data::flow_t>* GetMCFFlow()=0;
            virtual netxpert::data::graph_t::ArcMap<netxpert::data::cost_t>* GetMCFCost()=0;
            /* end of LEMON friendly interface */
            virtual const int GetMCFStatus()=0;
    };
} //namespace core
}//namespace netxpert
#endif // IMINCOSTFLOW_H
