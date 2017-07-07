#ifndef SPT_LEM_CH_H
#define SPT_LEM_CH_H

//Heap
#include "lemon/quad_heap.h"
#include "lemon/dheap.h"
#include "lemon/fib_heap.h"

#include "lemon/adaptors.h"
#include "isptree.h"

using namespace lemon;
using namespace netxpert::data;

namespace netxpert {
    namespace core {

    /**
    *  \Class Core Solver for the Shortest Path Tree Problem
    *   with Contraction Hierarchies of LEMON.
    */
    class SPT_LEM_CH : public netxpert::core::ISPTree
    {
        public:
            /** Default constructor */
            SPT_LEM_CH(bool isDrctd = true);
            /** Default destructor */
            ~SPT_LEM_CH();
            /** Copy constructor
             *  \param other Object to copy from
             */
            SPT_LEM_CH(const SPT_LEM_CH& other);
            /** Assignment operator
             *  \param other Object to assign from
             *  \return A reference to this
             */
            SPT_LEM_CH& operator=(const SPT_LEM_CH& other);

            /* LEMON friendly interface */
            void LoadNet(const uint32_t nmax,  const uint32_t mmax,
                                    lemon::FilterArcs<netxpert::data::graph_t,
                                              netxpert::data::graph_t::ArcMap<bool>>* sg,
                      netxpert::data::graph_t::ArcMap<netxpert::data::cost_t>* cm);
            const uint32_t GetArcCount();
            const uint32_t GetNodeCount();
            void SolveSPT(netxpert::data::cost_t treshold = -1, bool bidirectional = false );
            void SetOrigin( netxpert::data::node_t NewOrg );
            void SetDest( netxpert::data::node_t NewDst );
            bool Reached( netxpert::data::node_t NodeID );
            const std::vector<netxpert::data::node_t> GetPredecessors(netxpert::data::node_t _dest);
            const std::vector<netxpert::data::arc_t> GetPath(netxpert::data::node_t _dest);
            const netxpert::data::cost_t GetDist(netxpert::data::node_t _dest);
            /* end of LEMON friendly interface */

            /* Specials for Contraction Hierarchies */


        protected:
            uint32_t nmax; //max count nodes
            uint32_t mmax; //max count arcs

        private:
            bool isDrctd;
            bool allDests;
            bool bidirectional = false;

            std::unique_ptr<dijkstra_t> dijk;

            netxpert::data::filtered_graph_t* g;
            netxpert::data::graph_t::NodeMap<cost_t>* distMap;
            //lemon::concepts::ReadWriteMap<node_t,arc_t>* predMap;
            netxpert::data::graph_t::ArcMap<cost_t>* costMap;
            netxpert::data::node_t orig;
            netxpert::data::node_t dest;
    };
    } //namespace netxpert::core
} //namespace netxpert

#endif // SPT_LEM_CH_H
