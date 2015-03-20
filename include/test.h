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


namespace NetXpert
{
    /**
    * \Static test functions for NetXpert
    **/
    namespace Test
    {
        void NetworkConvert(NetXpert::Config& cnfg);
        void TestFileGDBWriter(Config& cnfg);
        void TestSpatiaLiteWriter(Config& cnfg);
        void TestAddStartNode(Config& cnfg);
    }
}

#endif // TEST_H
