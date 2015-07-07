#ifndef IMINSPANTREE_H
#define IMINSPANTREE_H

namespace netxpert {
    /** Abstract (Interface) Class for all Minimum Spanning Tree Solvers
    */
    class IMinSpanTree
    {
        public:
            virtual ~IMinSpanTree(){}
            virtual void GetMST(unsigned int* outStartNodes, unsigned int* outEndNodes)=0;
            virtual void LoadNet(unsigned int nmx, unsigned int mmx, unsigned int pn, unsigned int pm, double* pU,
                            double* pC, double* pDfct, unsigned int* pSn, unsigned int* pEn)=0;
            virtual unsigned int MCFmmax()=0;
            virtual unsigned int MCFnmax()=0;
            virtual void SolveMST()=0;
            virtual double MSTGetF0()=0;
    };
}
#endif // IMINSPANTREE_H
