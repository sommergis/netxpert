#ifndef ISPTREE_H
#define ISPTREE_H

#include <memory>

namespace netxpert {
    /**
    * \Class Abstract Class (Interface) for all Shortest Path Tree Solvers in netxpert core.
    */
    class ISPTree
    {
        public:
            typedef std::shared_ptr<ISPTree> ISPTreePtr;
            virtual ISPTreePtr create () const = 0; // Virtual constructor (creation)
            virtual ISPTreePtr clone () const = 0;  // Virtual constructor (for copying)
            /** Default destructor */
            virtual ~ISPTree() {}
            /* start of MCFClass SPTree Interface */
            virtual void LoadNet(unsigned int nmx, unsigned int mmx, unsigned int pn, unsigned int pm, double* pU,
                            double* pC, double* pDfct, unsigned int* pSn, unsigned int* pEn)=0;
            virtual unsigned int MCFmmax()=0;
            virtual unsigned int MCFnmax()=0;
            virtual void ShortestPathTree()=0;
            virtual void SetOrigin( unsigned int NewOrg )=0;
            virtual void SetDest( unsigned int NewDst )=0;
            virtual bool Reached( unsigned int NodeID )=0;
            virtual unsigned int* ArcPredecessors( void )=0;
            virtual unsigned int* Predecessors( void )=0;
            virtual void GetArcPredecessors ( unsigned int *outArcPrd )=0;
            virtual void GetPredecessors( unsigned int *outPrd )=0;

            //virtual void GetPath ( unsigned int Dst, unsigned int *outSn, unsigned int *outEn );
            /* end of MCFClass SPTree Interface */
    };
}
#endif // ISPTREE_H
