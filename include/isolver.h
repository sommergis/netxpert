#ifndef ISOLVER_H
#define ISOLVER_H

#include <string>
#include "network.h"

using namespace std;

namespace netxpert {
    /**
    * \Abstract Abstract Class (Interface) for all Solvers
    **/
    class ISolver
    {
        public:
            /** Default destructor */
            virtual ~ISolver() {}
            virtual void Solve(string net) = 0;
            virtual void Solve(Network& net) = 0;
            virtual double GetOptimum() const=0;
    };
}
#endif // ISOLVER_H
