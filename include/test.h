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
#include "odmatrix2.h"
#include "mcflow.h"
#include "transportation.h"
#include "networkbuilder.h"

#include "odmatrix_simple.h"
#include "mstree_simple.h"
#include "sptree_simple.h"
#include "transp_simple.h"
#include "mcfp_simple.h"
#include "netbuilder_simple.h"

namespace netxpert
{
    /**
    * \Static Test functions for NetXpert
    **/
    namespace test
    {
        void NetworkConvert(netxpert::cnfg::Config& cnfg);
        void TestFileGDBWriter(netxpert::cnfg::Config& cnfg);
        void TestSpatiaLiteWriter(netxpert::cnfg::Config& cnfg);
        void TestAddNodes(netxpert::cnfg::Config& cnfg);
        void TestMST(netxpert::cnfg::Config& cnfg);
        void TestSPT(netxpert::cnfg::Config& cnfg);
        void TestODMatrix(netxpert::cnfg::Config& cnfg);
        void TestODMatrix2(netxpert::cnfg::Config& cnfg); //experimental
        void TestCreateRouteGeometries(netxpert::cnfg::Config& cnfg);
        void TestMCF(netxpert::cnfg::Config& cnfg);
        void TestTransportation(netxpert::cnfg::Config& cnfg);
        void TestTransportationExt(netxpert::cnfg::Config& cnfg);
        void TestNetworkBuilder(netxpert::cnfg::Config& cnfg);
    }
}

#endif // TEST_H
