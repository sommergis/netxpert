#ifndef ODMATRIX_SIMPLE_H
#define ODMATRIX_SIMPLE_H

#include "data.h"
#include "utils.h"
#include "network.h"
#include "dbhelper.h"
#include "odmatrix.h"

namespace netxpert {
 namespace simple {

    class OriginDestinationMatrix
    {
        public:
            OriginDestinationMatrix(std::string jsonCnfg);
            int Solve();
            double GetOptimum();
            std::string GetODMatrixAsJSON();
            std::vector<netxpert::ExtSPTreeArc> GetODMatrix();
        private:
            Config NETXPERT_CNFG;
            std::unique_ptr<netxpert::OriginDestinationMatrix> solver = nullptr;
    };
  }
}
#endif // ODMATRIX_SIMPLE_H