%module pyNetXpert

%include "typemaps.i"
%include "std_string.i"
%include "std_list.i"
%include "std_map.i"

%{
#include "network.h"
#include "dbhelper.h"
%}

namespace std
{
    %template(InputArcs) std::list<NetXpert::InputArc>;
    %template(InputNodes) std::list<NetXpert::InputNode>;
}

namespace NetXpert
{
    struct ColumnMap
    {
        std::string arcIDColName;
        std::string fromColName;
        std::string toColName;
        std::string costColName;
        std::string capColName;
        std::string onewayColName;
        std::string nodeIDColName;
        std::string supplyColName;
    };

    struct InputNode
    {
        std::string extNodeID;
        double nodeSupply;
    };

    struct InputArc
    {
        std::string extArcID;
        unsigned int fromNode;
        unsigned int toNode;
        double cost;
        double capacity;
        std::string oneway;
    };

    typedef std::list<NetXpert::InputNode> InputNodes;
    typedef std::list<NetXpert::InputArc> InputArcs;

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
        TestSpatiaLiteWriter = 9
    };
    enum RESULT_DB_TYPE
    {
        SpatiaLiteDB = 0,
        ESRI_FileGDB = 1
    };

    enum LOG_LEVEL {
           All = -1,
           Debug = 0,
           Info = 1,
           Warning = 2,
           Error = 3,
           Fatal = 4   };

    enum SPTAlgorithm {

        Dijkstra_MCFClass = 0,
        LQueue_MCFClass = 1,
        LDeque_MCFClass = 2,
        Dijkstra_Heap_MCFClass = 3,
        Dijkstra_2Heap_LEMON = 4} ;

    enum MCFAlgorithm {
        NetworkSimplex_MCF = 0,
        NetworkSimplex_LEMON = 1
        } ;

    enum MSTAlgorithm {
        Kruskal_QuickGraph = 0, //.NET!
        Prim_QuickGraph = 1,    //.NET!
        Kruskal_LEMON = 2};

    /**
    * \Storage for the configuration of NetXpert
    **/
    struct Config
    {
        std::string SQLiteDBPath; //!< Member variable "sqliteDBPath"
        int SQLiteVersion; //!< Member variable "sqliteVersion"
        std::string ResultDBPath; //!< Member variable "resultDBPath"
        RESULT_DB_TYPE ResultDBType;//!< Member variable "resultDBType"
        bool SPTAllDests;//!< Member variable "sptAllDests"
        int SPTHeapCard; //!< Member variable "sptHeapCard"
        SPTAlgorithm SptAlgorithm; //!< Member variable "sptAlgorithm"
        MCFAlgorithm McfAlgorithm; //!< Member variable "mcfAlgorithm"
        MSTAlgorithm MstAlgorithm; //!< Member variable "mstAlgorithm"
        bool IsDirected; //!< Member variable "isDirected"
        std::string ArcsTableName; //!< Member variable "arcsTableName"
        std::string ArcsGeomColumnName; //!< Member variable "arcsGeomColumnName"
        std::string ArcIDColumnName; //!< Member variable "edgeIDColumnName"
        std::string FromNodeColumnName; //!< Member variable "fromNodeColumnName"
        std::string ToNodeColumnName; //!< Member variable "toNodeColumnName"
        std::string CostColumnName; //!< Member variable "costColumnName"
        std::string CapColumnName; //!< Member variable "capColumnName"
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
        GEOMETRY_HANDLING GeometryHandling;//!< Member variable "geometryHandling"
        std::string OnewayColumnName;//!< Member variable "onewayColumnName"
        TESTCASE TestCase;//!< Member variable "testCase"
        bool CleanNetwork;//!< Member variable "cleanNetwork"
        LOG_LEVEL LogLevel;
        std::string LogFileFullPath;
    };

    class Network
    {
        public:
            Network(InputArcs arcsTbl, ColumnMap _map, Config& cnfg);
            Network(InputArcs arcsTbl, InputNodes nodesTbl, ColumnMap _map, Config& cnfg);
            void ConvertInputNetwork(bool autoClean);
            std::string GetOriginalNodeID(unsigned int internalNodeID);
            std::string GetOriginalStartOrEndNodeID(unsigned int internalNodeID);
    };

    class LOGGER
    {
        protected:
            LOGGER() {}
        public:
            ~LOGGER() {}
            static void Initialize(Config& cnfg);
            static bool IsInitialized;
    };

    class DBHELPER
    {
        protected:
             DBHELPER(){}
        public:
            static void Initialize(Config& cnfg);
            static bool IsInitialized;
            static void CommitCurrentTransaction();
            static void OpenNewTransaction();
            static InputArcs LoadNetworkFromDB(std::string _tableName, ColumnMap _map);
            static InputNodes LoadNodesFromDB(std::string _tableName, ColumnMap _map);
            static void CloseConnection();
            ~DBHELPER();
    };
}

/* Add nicer __str__() methods */
%extend NetXpert::InputNode {
   char *__str__() {
       static char tmp [1024];
       sprintf(tmp,"InputNode('%s',%g)", $self->extNodeID.c_str(), $self->nodeSupply);
       return tmp;
   }
};
%extend NetXpert::InputArc {
   char *__str__() {
       static char tmp [1024];
       sprintf(tmp,"InputArc('%s',%u,%u,%g,%g,'%s')", $self->extArcID.c_str(), $self->fromNode,
                                        $self->toNode, $self->cost, $self->capacity,
                                        $self->oneway.c_str());
       return tmp;
   }
};
