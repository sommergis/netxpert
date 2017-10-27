#ifndef IMINSPANTREE_H
#define IMINSPANTREE_H

#include <memory>
#include <vector>
#include "lemon/concepts/path.h"
//#include "lemon/adaptors.h"
#include "lemon-net.h"

namespace netxpert {
    namespace core {
    /**
    * \Class Abstract Class (Interface) for all Minimum Spanning Tree Solvers in netxpert core.
    */
    class IMinSpanTree
    {
        public:
            /** Default destructor */
            virtual ~IMinSpanTree(){}

            /* LEMON friendly interface */
            virtual void LoadNet(const uint32_t nmax,  const uint32_t mmax,
                                    lemon::FilterArcs<netxpert::data::graph_t,
                                              netxpert::data::graph_t::ArcMap<bool>>* sg,
                                    netxpert::data::graph_t::ArcMap<netxpert::data::cost_t>* cm)=0;
            virtual const uint32_t GetArcCount()=0;
            virtual const uint32_t GetNodeCount()=0;
            virtual void SolveMST()=0;
            virtual const double GetOptimum() const =0;
            virtual std::vector<netxpert::data::arc_t> GetMST()=0;
            /* end of LEMON friendly interface */
    };
} //namespace core
}//namespace netxpert
#endif // IMINSPANTREE_H
