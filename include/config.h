#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>

using namespace std;
using namespace cereal;

namespace netxpert {

    /**
    * \Enum for Geometry Handling.
    **/
    enum GEOMETRY_HANDLING
    {
        NoGeometry = 0,
        StraightLines = 1,
        RealGeometry = 2
    };

    /**
    * \Enum for TestCases, that can be started per entry "TestCase" in Config file.
    **/
    enum TESTCASE
    {
        NetworkBuilder = 0,
        ShortestPathTreeCOM = 1,
        ODMatrixCOM = 2,
        TransportationCOM = 3,
        MSTCOM = 4,
        IsolinesCOM = 5,
        DumpInternalArcsToDB = 6,
        NetworkConvert = 7,
        TestFileGDBWriter = 8,
        TestSpatiaLiteWriter = 9,
        TestAddNodes = 10,
        TestCreateRouteGeometries = 11
    };

    /**
    * \Enum for the type of the output DB in which the solver result is been written.
    **/
    enum RESULT_DB_TYPE
    {
        SpatiaLiteDB = 0,
        ESRI_FileGDB = 1
    };
    /**
    * \Enum for the Log Level.
    **/
    enum LOG_LEVEL {
           All = -1,
           Debug = 0,
           Info = 1,
           Warning = 2,
           Error = 3,
           Fatal = 4   };
    /**
    * \Enum for the type of the Shortest Path Tree algorithms.
    **/
    enum SPTAlgorithm {

        Dijkstra_MCFClass = 0,
        LQueue_MCFClass = 1,
        LDeque_MCFClass = 2,
        Dijkstra_Heap_MCFClass = 3,
        Dijkstra_2Heap_LEMON = 4} ;
    /**
    * \Enum for the type of the Minimum Cost Flow algorithms.
    **/
    enum MCFAlgorithm {
        NetworkSimplex_MCF = 0,
        NetworkSimplex_LEMON = 1
        } ;
    /**
    * \Enum for the type of the Minimum Spanning Tree algorithms.
    **/
    enum MSTAlgorithm {
        Kruskal_QuickGraph = 0, //.NET!
        Prim_QuickGraph = 1,    //.NET!
        Kruskal_LEMON = 2};

    /**
    * \Storage for the configuration of NetXpert
    **/
    struct Config
    {
        string SQLiteDBPath; //!< Member variable "sqliteDBPath"
        int SQLiteVersion; //!< Member variable "sqliteVersion"
        string ResultDBPath; //!< Member variable "resultDBPath"
        RESULT_DB_TYPE ResultDBType;//!< Member variable "resultDBType"
        bool SPTAllDests;//!< Member variable "sptAllDests"
        int SPTHeapCard; //!< Member variable "sptHeapCard"
        SPTAlgorithm SptAlgorithm; //!< Member variable "sptAlgorithm"
        MCFAlgorithm McfAlgorithm; //!< Member variable "mcfAlgorithm"
        MSTAlgorithm MstAlgorithm; //!< Member variable "mstAlgorithm"
        bool IsDirected; //!< Member variable "isDirected"
        string ArcsTableName; //!< Member variable "arcsTableName"
        string ArcsGeomColumnName; //!< Member variable "arcsGeomColumnName"
        string ArcIDColumnName; //!< Member variable "arcIDColumnName"
        string FromNodeColumnName; //!< Member variable "fromNodeColumnName"
        string ToNodeColumnName; //!< Member variable "toNodeColumnName"
        string CostColumnName; //!< Member variable "costColumnName"
        string CapColumnName; //!< Member variable "capColumnName"
        string OnewayColumnName;//!< Member variable "onewayColumnName"
        string NodesTableName; //!< Member variable "nodesTableName"
        string NodesGeomColumnName; //!< Member variable "nodesGeomColumnName"
        string NodeIDColumnName;//!< Member variable "nodeIDColumnName"
        string NodeSupplyColumnName;//!< Member variable "nodeSupplyColumnName"
        string BarrierPolyTableName;//!< Member variable "barrierPolyTableName"
        string BarrierPolyGeomColumnName;//!< Member variable "barrierPolyGeomColumnName"
        string BarrierLineTableName;//!< Member variable "barrierLineTableName"
        string BarrierLineGeomColumnName;//!< Member variable "barrierLineGeomColumnName"
        string BarrierPointTableName;//!< Member variable "barrierPointTableName"
        string BarrierPointGeomColumnName;//!< Member variable "barrierPointGeomColumnName"
        int Treshold; //!< Member variable "treshold" for distance search: closest edge of network to given point
        bool UseSpatialIndex;//!< Member variable "useSpatialIndex"
        bool LoadDBIntoMemory;//!< Member variable "loadDBIntoMemory"
        int NumberOfTests;//!< Member variable "numberOfTests"
        string SpatiaLiteHome;//!< Member variable "spatiaLiteHome"
        string SpatiaLiteCoreName;//!< Member variable "spatiaLiteCoreName"
        GEOMETRY_HANDLING GeometryHandling;//!< Member variable "geometryHandling"
        TESTCASE TestCase;//!< Member variable "testCase"
        bool CleanNetwork;//!< Member variable "cleanNetwork"
        LOG_LEVEL LogLevel;
        string LogFileFullPath;

        /**
        * Serialize struct members to json
        **/
        template <class Archive>
        void serialize( Archive & ar ){
            ar(
                CEREAL_NVP(SQLiteDBPath),
                CEREAL_NVP(SQLiteVersion),
                CEREAL_NVP(ResultDBPath),
                CEREAL_NVP(ResultDBType),
                CEREAL_NVP(SPTAllDests),
                CEREAL_NVP(SPTHeapCard),
                CEREAL_NVP(SptAlgorithm),
                CEREAL_NVP(McfAlgorithm),
                CEREAL_NVP(MstAlgorithm),
                CEREAL_NVP(IsDirected),
                CEREAL_NVP(ArcsTableName),
                CEREAL_NVP(ArcsGeomColumnName),
                CEREAL_NVP(ArcIDColumnName),
                CEREAL_NVP(FromNodeColumnName),
                CEREAL_NVP(ToNodeColumnName),
                CEREAL_NVP(CostColumnName),
                CEREAL_NVP(CapColumnName),
                CEREAL_NVP(NodesTableName),
                CEREAL_NVP(NodesGeomColumnName),
                CEREAL_NVP(NodeIDColumnName),
                CEREAL_NVP(NodeSupplyColumnName),
                CEREAL_NVP(BarrierPolyTableName),
                CEREAL_NVP(BarrierPolyGeomColumnName),
                CEREAL_NVP(BarrierLineTableName),
                CEREAL_NVP(BarrierLineGeomColumnName),
                CEREAL_NVP(BarrierPointTableName),
                CEREAL_NVP(BarrierPointGeomColumnName),
                CEREAL_NVP(Treshold),
                CEREAL_NVP(UseSpatialIndex),
                CEREAL_NVP(CleanNetwork),
                CEREAL_NVP(LoadDBIntoMemory),
                CEREAL_NVP(NumberOfTests),
                CEREAL_NVP(SpatiaLiteHome),
                CEREAL_NVP(SpatiaLiteCoreName),
                CEREAL_NVP(OnewayColumnName),
                CEREAL_NVP(TestCase),
                CEREAL_NVP(GeometryHandling),
                CEREAL_NVP(LogLevel),
                CEREAL_NVP(LogFileFullPath) );
        }
    };


    /**
    * \Class that reads a configuration file for NetXpert
    **/
    class ConfigReader
    {
        public:
            ConfigReader() {}
            ~ConfigReader() {}
            Config GetConfigFromJSON(string jsonString);
            void GetConfigFromJSONFile(string fileName, Config& cnfg);
    };
}

#endif // CONFIG_H
