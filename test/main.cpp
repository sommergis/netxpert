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

#include <iostream>
#include "logger.h"
#include "fgdbwriter.h"
#include "slitewriter.h"
#include <fstream>

#include "data.h"
#include "dbhelper.h"
#include "lemon-net.h"
#include "test.h"

using namespace std;
using namespace netxpert;
using namespace netxpert::cnfg;
using namespace netxpert::utils;
using namespace cereal;

int main(int argc, char** argv)
{
    string inFile = "";
    if( argc == 2 )
    {
        inFile = argv[1];
    }
    else
    {
        //inFile = "./ODMatrixCnfg.json";
        //inFile = "/home/hahne/dev/netxpert/test/bin/Debug/SPTreeCnfg_small.json";
        //inFile = "/home/hahne/dev/netxpert/test/bin/Debug/TestCreateGeom.json";
        //inFile = "/home/hahne/dev/netxpert/test/bin/Debug/TestSpatialiteWriter.json";
//        inFile = "/home/hahne/dev/netxpert1_0/test/bin/Debug/ODMatrixCnfg_small.json";
//        inFile = "/home/hahne/dev/netxpert1_0/test/bin/Debug/SPTCnfg_spt_lines_1_points_1_small.json";
//        inFile = "/home/hahne/dev/netxpert1_0/test/bin/Debug/SPTCnfg_spt_lines_4_points_4.json";
        //inFile = "/home/hahne/dev/netxpert1_0/test/bin/Release/ODMatrixCnfg_Big.json";
        //inFile = "/home/hahne/dev/netxpert1_0/test/bin/Release/MSTCnfg_small.json";
        inFile = "/home/hahne/dev/netxpert1_0/test/bin/Debug/SPTreeCnfg_small_3.json";
        //inFile = "/home/hahne/dev/netxpert1_0/test/bin/Debug/TestFGDBWriter.json";
        //inFile = "/home/hahne/dev/netxpert1_0/test/bin/Release/SPTCnfg_small.json";
//        inFile = "/home/hahne/dev/netxpert1_0/test/bin/Debug/TranspCnfg_small.json";
//        inFile = "/home/hahne/dev/netxpert1_0/test/bin/Debug/SPTCnfg_small.json";
        //inFile = "/home/hahne/dev/netxpert1_0/test/bin/Debug/NetworkBuilder_small.json";
//        inFile = "/home/hahne/dev/netxpert1_0/test/bin/Release/MCFCnfg_small.json";
    }

    Config cnfg;
    try
    {
        ConfigReader reader;
        reader.GetConfigFromJSONFile(inFile, cnfg);
    }
    catch (std::exception& e)
    {
        cout << "Fehler beim Config init: " << e.what() << endl;
    }
    try
    {
        LOGGER::Initialize(cnfg);
        LOGGER::LogInfo("Logging gestartet!");
    }
    catch (std::exception& e)
    {
        cout << "Fehler beim Logger init." << e.what() << endl;
    }
    try
    {
        LOGGER::LogInfo("Testing Lemon Network..");
        netxpert::test::LemonNetworkConvert(cnfg);

//        LOGGER::LogInfo("Testing Network..");
//        netxpert::test::NetworkConvert(cnfg);

        //Data test
        /*Arcs t;
        FTNode n {n.fromNode = 1, n.toNode = 2};
        ArcData a;
        a.extArcID = "1";
        a.cost = 1.0;
        a.capacity = 99999.9;
        t.insert(make_pair(n, a));

        t.insert({{1,3},{"2", 3.0, 99999.9}});

        auto val1 = t.at(n);
        auto val2 = t.at({1,3});

        AddedPoints aPmap;
        NodeID newNodeId = 1;
        AddedPoint aP;
        aP.oldArcID = "2";
        aP.coord = geos::geom::Coordinate(1.0, 1.5);
        aPmap.insert(make_pair(newNodeId, aP));

        //Network test
        InputArc arc1 = {"1", 1, 2, 1.0, 99999.9, ""};
        InputArc arc2 = {"2", 2, 3, 3.0, 99999.9, ""};
        InputArc arc3 = {"3", 2, 3, 2.0, 99999.9, ""};
        InputArc arc4 = {"4", 3, 3, 4.0, -1, ""};
        InputNode node {"1", 2.0};*/



        //TEST CASES
        switch (cnfg.TestCase)
        {
//        case TESTCASE::TestNetworkBuilder:
//            netxpert::test::TestNetworkBuilder(cnfg);
//            break;
//        case TESTCASE::NetworkConvert:
//            netxpert::test::NetworkConvert(cnfg);
//            break;



//        case TESTCASE::TestFileGDBWriter:
//            netxpert::test::TestFileGDBWriter(cnfg);
//            break;
//        case TESTCASE::TestSpatiaLiteWriter:
//            netxpert::test::TestSpatiaLiteWriter(cnfg);
//            break;




//        case TESTCASE::TestAddNodes:
//            netxpert::test::TestAddNodes(cnfg);
//            break;
//        case TESTCASE::MSTCOM:
//            netxpert::test::TestMST(cnfg);
//            break;

        case TESTCASE::ShortestPathTreeCOM:
            #if (defined ENABLE_CONTRACTION_HIERARCHIES)
            netxpert::test::TestSPTCH(cnfg);
            break;
            #endif
            netxpert::test::TestSPT(cnfg);
            break;
        case TESTCASE::ODMatrixCOM:
            netxpert::test::TestODMatrix(cnfg);
            break;
//        case TESTCASE::TestCreateRouteGeometries:
//            netxpert::test::TestCreateRouteGeometries(cnfg);
//            break;
        case TESTCASE::MCFCOM:
            netxpert::test::TestMCF(cnfg);
            break;
        case TESTCASE::TransportationCOM:
            netxpert::test::TestTransportation(cnfg);
            break;
//        case TESTCASE::TransportationCOMExt:
//            netxpert::test::TestTransportationExt(cnfg);
//            break;
//        case TESTCASE::ODMatrixCOM2:
//            netxpert::test::TestODMatrix2(cnfg);
//            break;
        }

        /*
                        //only test connection to sqlite
                        if (!DBHELPER::IsInitialized)
                        {
                            DBHELPER::Initialize(cnfg);
                        }
                        DBHELPER::OpenNewTransaction();
                        DBHELPER::CommitCurrentTransaction();
                        DBHELPER::CloseConnection();
        */

    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( ex.what() );
    }
    return 0;
}

