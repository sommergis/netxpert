/*
 * This file is a part of netxpert.
 *
 * Copyright (C) 2013-2017
 * Johannes Sommer, Christopher Koller
 *
 * Permission to use, modify and distribute this software is granted
 * provided that this copyright notice appears in all copies. For
 * precise terms see the accompanying LICENSE file.
 *
 * This software is provided "AS IS" with no warranty of any kind,
 * express or implied, and with no claim as to its suitability for any
 * purpose.
 *
 */

#ifndef TEST_H
#define TEST_H

#include <iostream>
//Fix for C2146 Error on Windows (lemon/adaptors.h error!)
//#include "logger.h"
#include "fgdbwriter.h" //has logger, logger has utils
#include "slitewriter.h"
#include <fstream>
#include "geos/io/WKTReader.h"
#include "geos/io/StringTokenizer.h"

#include "data.h"
#include "lemon-net.h"
#include "dbhelper.h"
#include "sptree.h"

//#include "network.h"
//#include "mstree.h"
//#include "sptree.h"
//#include "odmatrix.h"
//#include "mcflow.h"
//#include "transportation.h"
//#include "networkbuilder.h"

#include "odmatrix_simple.h"
#include "mstree_simple.h"
#include "sptree_simple.h"
#include "transp_simple.h"
#include "mcfp_simple.h"
#include "netbuilder_simple.h"

namespace netxpert
{
    namespace test
    {
//        void NetworkConvert(netxpert::cnfg::Config& cnfg);
        void LemonNetworkConvert(netxpert::cnfg::Config& cnfg);
        void TestFileGDBWriter(netxpert::cnfg::Config& cnfg);
        void TestSpatiaLiteWriter(netxpert::cnfg::Config& cnfg);
//        void TestAddNodes(netxpert::cnfg::Config& cnfg);
        void TestMST(netxpert::cnfg::Config& cnfg);
        void TestSPT(netxpert::cnfg::Config& cnfg);
        #if (defined ENABLE_CONTRACTION_HIERARCHIES)
        void TestSPTCH(netxpert::cnfg::Config& cnfg);
        #endif
        void TestODMatrix(netxpert::cnfg::Config& cnfg);
//        void TestODMatrix2(netxpert::cnfg::Config& cnfg); //experimental
//        void TestCreateRouteGeometries(netxpert::cnfg::Config& cnfg);
        void TestMCF(netxpert::cnfg::Config& cnfg);
        void TestTransportation(netxpert::cnfg::Config& cnfg);
//        void TestTransportationExt(netxpert::cnfg::Config& cnfg);
        void TestNetworkBuilder(netxpert::cnfg::Config& cnfg);
    }
}

#endif // TEST_H
