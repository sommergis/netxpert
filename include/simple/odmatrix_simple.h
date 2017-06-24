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
            ~OriginDestinationMatrix() {}
            int Solve();
            int Solve(bool parallel);
            double GetOptimum();
            std::string GetODMatrixAsJSON();
            std::vector<netxpert::data::ExtSPTreeArc> GetODMatrix();
        private:
            netxpert::cnfg::Config NETXPERT_CNFG;
            std::unique_ptr<netxpert::OriginDestinationMatrix> solver = nullptr;
    };
  }
}
#endif // ODMATRIX_SIMPLE_H
