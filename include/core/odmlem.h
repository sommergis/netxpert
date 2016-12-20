#ifndef ODM_LEM_2HEAP_H
#define ODM_LEM_2HEAP_H

#include "lemon/smart_graph.h"
#include "lemon/dijkstra.h"
#include "bijkstra.h"
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <vector>

#include "isptree.h"

using namespace lemon;

namespace netxpert {
    namespace core {

    /**
    *  \Class Core Solver for the Multi source shortest Path Tree Problem with binary Heap structure and
    *   Dijkstra's algorithm of LEMON.
    *   EXPERIMENTAL --> solver/odmatrix2.h
    */
    class ODM_LEM_2Heap : public netxpert::core::ISPTree
    {
        public:
            /** Default constructor */
            ODM_LEM_2Heap(unsigned int nmx = 0 , unsigned int mmx = 0 , bool Drctd = true);
            ODM_LEM_2Heap(ODM_LEM_2Heap const &) {};
            /** Default destructor */
            virtual ~ODM_LEM_2Heap();
            /* MCFClass SPTree Interface */
            void ShortestPathTree();
            void LoadNet( unsigned int nmx , unsigned int mmx , unsigned int pn , unsigned int pm ,
                  double *pU , double *pC , double *pDfct ,
                  unsigned int *pSn , unsigned int *pEn );
            unsigned int MCFnmax( void );
            unsigned int MCFmmax( void );
            void SetOrigin( unsigned int NewOrg );
            void SetDest( unsigned int NewDst );
            bool Reached( unsigned int NodeID );
            unsigned int* ArcPredecessors( void );
            unsigned int* Predecessors( void );
            void GetArcPredecessors ( unsigned int *outArcPrd );
            void GetPredecessors( unsigned int *outPrd );

            ISPTreePtr create () const        // Virtual constructor (creation)
            {
                return ISPTreePtr(new ODM_LEM_2Heap() );
            }
            ISPTreePtr clone () const        // Virtual constructor (copying)
            {
                return ISPTreePtr(new ODM_LEM_2Heap(*this));
            }
            /* end of MCFClass SPTree Interface */
            std::vector<std::pair<unsigned int, unsigned int>>
              GetPath ( unsigned int s, unsigned int t );

        protected:
            unsigned int nmax; //max count nodes
            unsigned int mmax; //max count arcs

        private:
            using DijkstraInternal = Dijkstra<SmartDigraph, SmartDigraph::ArcMap<double>>;

            bool isDrctd;
            bool allDests;
            SmartDigraph g;
            DijkstraInternal* dijk;
            SmartDigraph::ArcMap<double>* length;
            SmartDigraph::Node orig;
            std::vector< SmartDigraph::Node > dests;
            std::vector<typename SmartDigraph::Node> nodes;
            unsigned int* predecessors;
            unsigned int* arcPredecessors;
    };
} //namespace core
} //namespace netxpert
#endif // ODM_LEM_2HEAP_H
