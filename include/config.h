#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <stdexcept>
#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/map.hpp>
#include <omp.h>

namespace netxpert {

    static const std::string Version()
    {
        return "0.9.4";
    };

    namespace cnfg {

    /**
    * 80% of available CPU Power (=number of threads) is used
    * (can be overridden by setting environment variable
    *  OMP_NUM_THREADS to a higher value)
    */
    //#ifdef OMP_H
	//static int LOCAL_NUM_THREADS = std::floor(omp_get_max_threads() * 0.8);
	//#else
	#ifdef DEBUG
        static int LOCAL_NUM_THREADS = 1; //std::floor(omp_get_max_threads() * 0.8);
    #endif // DEBUG
    #ifndef DEBUG
        static int LOCAL_NUM_THREADS = std::floor(omp_get_max_threads() * 0.8);
    #endif
	//#endif // OMP_H

    /**
    * \Enum Geometry Handling.
    **/
    enum GEOMETRY_HANDLING
    {
        NoGeometry = 0,
        StraightLines = 1,
        RealGeometry = 2
    };

    /**
    * \Enum TestCases, that can be started per entry "TestCase" in Config file.
    **/
    enum TESTCASE
    {
        TestNetworkBuilder = 0,
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
        TestCreateRouteGeometries = 11,
        MCFCOM = 12,
        TransportationCOMExt = 13,
        ODMatrixCOM2 = 14
    };

    /**
    * \Enum Type of the output DB in which the solver result is been written.
    **/
    enum RESULT_DB_TYPE
    {
        SpatiaLiteDB = 0,
        ESRI_FileGDB = 1
    };

    /**
    * \Enum Type of the Log Level.
    **/
    enum LOG_LEVEL {
       LogAll = -1,
       LogDebug = 0,
       LogInfo = 1,
       LogWarning = 2,
       LogError = 3,
       LogFatal = 4,
       LogQuiet = 5
    };
    /**
    * \Enum Type of the Shortest Path Tree algorithms.
    **/
    enum SPTAlgorithm {

        Dijkstra_MCFClass = 0, //Buggy
        LQueue_MCFClass = 1,
        LDeque_MCFClass = 2,
        Dijkstra_Heap_MCFClass = 3,
        Dijkstra_2Heap_LEMON = 4,
        Bijkstra_2Heap_LEMON = 5,   //EXPERIMENTAL
        Dijkstra_dheap_BOOST = 6,   //EXPERIMENTAL
        ODM_LEM_2Heap = 7           //EXPERIMENTAL
    } ;
    /**
    * \Enum Type of the Minimum Cost Flow algorithms.
    **/
    enum MCFAlgorithm {
        NetworkSimplex_MCF = 0,
        NetworkSimplex_LEMON = 1
        } ;
    /**
    * \Enum Type of the Minimum Spanning Tree algorithms.
    **/
    enum MSTAlgorithm {
        //Kruskal_QuickGraph = 0, //.NET!
        //Prim_QuickGraph = 1,    //.NET!
        Kruskal_LEMON = 2};

    /**
    * \Class Storage for the configuration of NetXpert
    **/
    struct Config
    {
        std::string NetXDBPath; //!< Member variable "netxDBPath"
        std::string ResultDBPath; //!< Member variable "resultDBPath"
        netxpert::cnfg::RESULT_DB_TYPE ResultDBType;//!< Member variable "resultDBType"
        std::string ResultTableName; //!< Member variable "resultTableName"
        bool SPTAllDests;//!< Member variable "sptAllDests"
        int SPTHeapCard; //!< Member variable "sptHeapCard"
        netxpert::cnfg::SPTAlgorithm SptAlgorithm; //!< Member variable "sptAlgorithm"
        netxpert::cnfg::MCFAlgorithm McfAlgorithm; //!< Member variable "mcfAlgorithm"
        netxpert::cnfg::MSTAlgorithm MstAlgorithm; //!< Member variable "mstAlgorithm"
        bool IsDirected; //!< Member variable "isDirected"
        std::string ArcsTableName; //!< Member variable "arcsTableName"
        std::string ArcsGeomColumnName; //!< Member variable "arcsGeomColumnName"
        std::string ArcIDColumnName; //!< Member variable "arcIDColumnName"
        std::string FromNodeColumnName; //!< Member variable "fromNodeColumnName"
        std::string ToNodeColumnName; //!< Member variable "toNodeColumnName"
        std::string CostColumnName; //!< Member variable "costColumnName"
        std::string CapColumnName; //!< Member variable "capColumnName"
        std::string OnewayColumnName;//!< Member variable "onewayColumnName"
        std::string NodesTableName; //!< Member variable "nodesTableName"
        std::string NodesGeomColumnName; //!< Member variable "nodesGeomColumnName"
        std::string NodeIDColumnName;//!< Member variable "nodeIDColumnName"
        std::string NodeSupplyColumnName;//!< Member variable "nodeSupplyColumnName"
        std::string BarrierPolyTableName;//!< Member variable "barrierPolyTableName"
        std::string BarrierPolyGeomColumnName;//!< Member variable "barrierPolyGeomColumnName"
        std::string BarrierLineTableName;//!< Member variable "barrierLineTableName"
        std::string BarrierLineGeomColumnName;//!< Member variable "barrierLineGeomColumnName"
        std::string BarrierPointTableName;//!< Member variable "barrierPointTableName"
        std::string BarrierPointGeomColumnName;//!< Member variable "barrierPointGeomColumnName"
        int Treshold; //!< Member variable "treshold" for distance search: closest edge of network to given point
        bool UseSpatialIndex;//!< Member variable "useSpatialIndex"
        bool LoadDBIntoMemory;//!< Member variable "loadDBIntoMemory"
        int NumberOfTests;//!< Member variable "numberOfTests"
        std::string SpatiaLiteHome;//!< Member variable "spatiaLiteHome"
        std::string SpatiaLiteCoreName;//!< Member variable "spatiaLiteCoreName"
        netxpert::cnfg::GEOMETRY_HANDLING GeometryHandling;//!< Member variable "geometryHandling"
        netxpert::cnfg::TESTCASE TestCase;//!< Member variable "testCase"
        netxpert::cnfg::LOG_LEVEL LogLevel;
        bool CleanNetwork;//!< Member variable "cleanNetwork"
        std::string LogFileFullPath;

        /**
        * Serialize struct members to json
        **/
        template <class Archive>
        void serialize( Archive & ar ){
            ar(
                CEREAL_NVP(NetXDBPath),
                CEREAL_NVP(ResultDBPath),
                CEREAL_NVP(ResultDBType),
                CEREAL_NVP(ResultTableName),
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
    * \Class Class reads a configuration file for NetXpert
    **/
    class ConfigReader
    {
        public:
            ConfigReader() {}
            ~ConfigReader() {}
            Config GetConfigFromJSON(std::string jsonString);
            void GetConfigFromJSONFile(std::string fileName, netxpert::cnfg::Config& cnfg);
    };
} //namespace cnfg
} //namespace netxpert

#endif // CONFIG_H
