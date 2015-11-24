#ifndef TEST_H
#define TEST_H

#include <iostream>
#include "logger.h"
#include "fgdbwriter.h"
#include "slitewriter.h"
#include <fstream>
#include "geos/io/WKTReader.h"
#include "geos/io/StringTokenizer.h"

#include "data.h"
#include "utils.h"
#include "network.h"
#include "dbhelper.h"
#include "mstree.h"
#include "sptree.h"
#include "odmatrix.h"
#include "mcflow.h"
#include "transportation.h"
#include "odmatrix_simple.h"
#include "mstree_simple.h"
#include "sptree_simple.h"
#include "transp_simple.h"
#include "mcfp_simple.h"
#include "networkbuilder.h"

namespace netxpert
{
    /**
    * \Static Test functions for NetXpert
    **/
    namespace Test
    {
        void NetworkConvert(Config& cnfg);
        void TestFileGDBWriter(Config& cnfg);
        void TestSpatiaLiteWriter(Config& cnfg);
        void TestAddNodes(Config& cnfg);
        void TestMST(Config& cnfg);
        void TestSPT(Config& cnfg);
        void TestODMatrix(Config& cnfg);
        void TestCreateRouteGeometries(Config& cnfg);
        void TestMCF(Config& cnfg);
        void TestTransportation(Config& cnfg);
        void TestTransportationExt(Config& cnfg);
        void TestNetworkBuilder(Config& cnfg);
    }
}

#endif // TEST_H
