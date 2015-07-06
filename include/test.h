#ifndef TEST_H
#define TEST_H

#include <iostream>
#include "logger.h"
#include "fgdbwriter.h"
#include "slitewriter.h"
#include <fstream>
#include <geos/io/WKTReader.h>
#include <geos/io/StringTokenizer.h>

#include "data.h"
#include "network.h"
#include "dbhelper.h"
#include "mstree.h"
#include "sptree.h"
#include "odmatrix.h"

namespace netxpert
{
    /**
    * \Static test functions for NetXpert
    **/
    namespace Test
    {
        void NetworkConvert(netxpert::Config& cnfg);
        void TestFileGDBWriter(Config& cnfg);
        void TestSpatiaLiteWriter(Config& cnfg);
        void TestAddNodes(Config& cnfg);
        void TestMST(Config& cnfg);
        void TestSPT(Config& cnfg);
        void TestODMatrix(Config& cnfg);
        void TestCreateRouteGeometries(Config& cnfg);
    }
}

#endif // TEST_H
