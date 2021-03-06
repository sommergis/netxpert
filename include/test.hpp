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
//Watch for C2146 Error on Windows (lemon/adaptors.h error!)
#include "fgdbwriter.hpp" //has logger, logger has utils
#include "slitewriter.hpp"
#include <fstream>
#include "geos/io/WKTReader.h"
#include "geos/io/StringTokenizer.h"

#include "data.hpp"
#include "lemon-net.hpp"
#include "dbhelper.hpp"
#include "sptree.hpp"

#include "odmatrix_simple.hpp"
#include "mstree_simple.hpp"
#include "sptree_simple.hpp"
#include "transp_simple.hpp"
#include "mcfp_simple.hpp"
#include "netbuilder_simple.hpp"

namespace netxpert
{
    namespace test
    {
        void LemonNetworkConvert(netxpert::cnfg::Config& cnfg);
        void TestFileGDBWriter(netxpert::cnfg::Config& cnfg);
        void TestSpatiaLiteWriter(netxpert::cnfg::Config& cnfg);
        void TestAddNodes(netxpert::cnfg::Config& cnfg);
        void TestMST(netxpert::cnfg::Config& cnfg);
        void TestSPT(netxpert::cnfg::Config& cnfg);
        #if (defined NETX_ENABLE_CONTRACTION_HIERARCHIES)
        void TestSPTCH(netxpert::cnfg::Config& cnfg);
        #endif
        void TestODMatrix(netxpert::cnfg::Config& cnfg);
        void TestCreateRouteGeometries(netxpert::cnfg::Config& cnfg);
        void TestMCF(netxpert::cnfg::Config& cnfg);
        void TestTransportation(netxpert::cnfg::Config& cnfg);
//        void TestTransportationExt(netxpert::cnfg::Config& cnfg);
        void TestNetworkBuilder(netxpert::cnfg::Config& cnfg);
    }
}

#endif // TEST_H
