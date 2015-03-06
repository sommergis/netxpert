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
#include "test.h"

using namespace std;
using namespace NetXpert;
using namespace cereal;

int main(int argc, char** argv)
{
    string inFile = "";
    if( argc == 2 ) {
        inFile = argv[1];
    }
    else
    {
        inFile = "./ODMatrixCnfg.json";
    }

    Config cnfg;
    try {
        ConfigReader reader;
        reader.GetConfigFromJSONFile(inFile, cnfg);
    }
    catch (std::exception& e)
    {
        cout << "Fehler beim Config init: " << e.what() << endl;
    }
    try {
        LOGGER::NetXpertConfig = cnfg;
        LOGGER::Initialize();
        LOGGER::LogInfo("Logging gestartet!");
    }
    catch (std::exception& e)
    {
        cout << "Fehler beim Logger init." << e.what() << endl;
    }
    try
    {
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
        InputNode node {"1", 2.0};

        /* TEST CASES */
        switch (cnfg.TestCase){

            case TESTCASE::NetworkConvert:
                NetXpert::Test::NetworkConvert(cnfg);
                break;
            case TESTCASE::TestFileGDBWriter:
                NetXpert::Test::TestFileGDBWriter(cnfg);
                break;
            case TESTCASE::TestSpatiaLiteWriter:
                NetXpert::Test::TestSpatiaLiteWriter(cnfg);
                break;
        }

    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( ex.what() );
    }
/*
    std::ofstream os("out.json");
    JSONOutputArchive archive1( os );
    //cnfg config;
    archive1( CEREAL_NVP(c) ); */

    return 0;
}

