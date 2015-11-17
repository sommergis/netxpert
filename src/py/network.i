%module pynetxpert

%include "typemaps.i"
%include "std_string.i"
%include "std_map.i"
%include "std_vector.i"
%include "std_pair.i"

%{
#include "network.h"
#include "dbhelper.h"
#include "isolver.h"
#include "mstree.h"
#include "sptree.h"
#include "odmatrix.h"
#include "mcflow.h"
#include "transportation.h"

/* Simple Solver Interface */

#include "sptree_simple.h"
#include "odmatrix_simple.h"
#include "mstree_simple.h"
#include "transp_simple.h"
#include "mcfp_simple.h"
%}

namespace std
{
    %template(InputArcs) std::vector<netxpert::InputArc>;
    %template(InputNodes) std::vector<netxpert::InputNode>;
    %template(NewNodes) std::vector<netxpert::NewNode>;
    %template(ExtSPTArcs) std::vector<netxpert::ExtSPTreeArc>;
    %template(ExtNodeSupplies) std::vector<netxpert::ExtNodeSupply>;
    %template(ExtDistribution) std::vector<netxpert::ExtDistributionArc>;
    %template(FlowCosts) std::vector<netxpert::FlowCost>;
    /*%template(ODNodes) std::vector< std::pair<unsigned int,std::string> >;*/
}

namespace netxpert
{
    static const std::string Version()
    {
        return "0.9.0";
    };

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
        std::string extFromNode;
        std::string extToNode;
        double cost;
        double capacity;
        std::string oneway;
    };

    struct NewNode
    {
        std::string extNodeID;
        Coordinate coord;
        double supply;
    };
    struct ODPair
    {
        unsigned int origin;
        unsigned int dest;
    };

    struct InternalArc
    {
        unsigned int fromNode;
        unsigned int toNode;
    };

    struct FlowCost
    {
        InternalArc intArc;
        double flow;
        double cost;
    };

    typedef std::vector<netxpert::InputNode> InputNodes;
    typedef std::vector<netxpert::InputArc> InputArcs;
    typedef std::pair<std::vector<unsigned int>,double> CompressedPath;
    typedef std::vector<netxpert::NewNode> NewNodes;
    typedef std::string ExtArcID;
    typedef std::string ExtNodeID;
    typedef std::vector<netxpert::ExtSPTreeArc> ExtSPTArcs;
    typedef std::vector<netxpert::ExtNodeSupply> ExtNodeSupplies;
    typedef std::vector<netxpert::ExtDistributionArc> ExtDistribution;

    typedef std::vector< std::pair<unsigned int,std::string> >& ODNodes;

    struct ExtNodeSupply
    {
        ExtNodeID extNodeID;
        double supply;
    };

    struct ExternalArc
    {
        std::string extFromNode;
        std::string extToNode;
    };
    struct ExtSPTreeArc
    {
        ExtArcID extArcID;
        ExternalArc extArc;
        double cost;
    };

    struct ExtTransportationData
    {
        ExtSPTArcs odm;
        ExtNodeSupplies supply;
    };

    struct ExtDistributionArc
    {
        ExtArcID arcid;
        ExternalArc extArc;
        double cost;
        double flow;
    };

    enum GEOMETRY_HANDLING
    {
        NoGeometry = 0,
        StraightLines = 1,
        RealGeometry = 2
    };

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
        TestCreateRouteGeometries = 11,
        MCFCOM = 12,
        TransportationCOMExt = 13
    };

    enum RESULT_DB_TYPE
    {
        SpatiaLiteDB = 0,
        ESRI_FileGDB = 1
    };

    enum LOG_LEVEL {
           LogAll = -1,
           LogDebug = 0,
           LogInfo = 1,
           LogWarning = 2,
           LogError = 3,
           LogFatal = 4   };

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
        //Kruskal_QuickGraph = 0, //.NET!
        //Prim_QuickGraph = 1,    //.NET!
        Kruskal_LEMON = 2
    };

    enum MCFSolverStatus
    {
       MCFUnSolved = -1 ,     ///< no solution available
       MCFOK = 0 ,            ///< optimal solution found
       MCFStopped = 1,        ///< optimization stopped
       MCFUnfeasible = 2,     ///< problem is unfeasible
       MCFUnbounded = 3,      ///< problem is unbounded
       MCFError = 4           ///< error in the solver
    };
    enum MinCostFlowInstanceType
    {
        MCFUndefined = 0,
        MCFBalanced = 1,
        MCFExtrasupply = 2,
        MCFExtrademand = 3
    };

    /**
    * \Storage for the configuration of netxpert
    **/
    struct Config
    {
        std::string NetXDBPath; //!< Member variable "netxDBPath"
        std::string ResultDBPath; //!< Member variable "resultDBPath"
        RESULT_DB_TYPE ResultDBType;//!< Member variable "resultDBType"
        std::string ResultTableName; //!< Member variable "resultTableName"
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
            /*Network(InputArcs arcsTbl, InputNodes nodesTbl, ColumnMap _map, Config& cnfg);*/
            void ConvertInputNetwork(bool autoClean);

            /* TODO SWIG 3.0 kann keine unique_ptr */
            //unsigned int AddStartNode(const NewNode& newNode, int treshold, std::unique_ptr<SQLite::Statement> closestArcQry, bool withCapacity);
            //unsigned int AddEndNode(const NewNode& newNode, int treshold, std::unique_ptr<SQLite::Statement> closestArcQry, bool withCapacity);

            std::vector< std::pair<unsigned int, std::string> > LoadStartNodes(const std::vector<NewNode>& newNodes, int treshold,
                                                            std::string arcsTableName, std::string geomColumnName,
                                                                ColumnMap& cmap, bool withCapacity);
            std::vector< std::pair<unsigned int, std::string> > LoadEndNodes(const std::vector<NewNode>& newNodes, int treshold,
                                                            std::string arcsTableName, std::string geomColumnName,
                                                                ColumnMap& cmap, bool withCapacity);

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
            static NewNodes LoadNodesFromDB(std::string _tableName, std::string geomColName, const ColumnMap& _map);

            static void CloseConnection();
            ~DBHELPER();
    };

    class ISolver
    {
        public:
            virtual ~ISolver() {}
            virtual void Solve(std::string net) = 0;
            virtual void Solve(Network& net) = 0;
    };

    class MinimumSpanningTree : public ISolver
    {
        public:
            MinimumSpanningTree(Config& cnfg);
            virtual ~MinimumSpanningTree();

            void Solve(std::string net);
            void Solve(Network& net);

            MSTAlgorithm GetAlgorithm();
            void SetAlgorithm(MSTAlgorithm mstAlgorithm);

            double GetOptimum();
            std::vector<InternalArc> GetMinimumSpanningTree() const;
    };

    class OriginDestinationMatrix : public ISolver
    {
        public:
            OriginDestinationMatrix(Config& cnfg);
            virtual ~OriginDestinationMatrix();

            void Solve(std::string net);
            void Solve(Network& net);

            SPTAlgorithm GetAlgorithm() const;
            void SetAlgorithm(SPTAlgorithm mstAlgorithm);

            int GetSPTHeapCard() const;
            void SetSPTHeapCard(int heapCard);

            GEOMETRY_HANDLING GetGeometryHandling() const;
            void SetGeometryHandling(GEOMETRY_HANDLING geomHandling);

            std::vector<unsigned int> GetOrigins() const;
			/*SWIG will leerzeichen bei manchen spitzen Klammern */
            void SetOrigins(std::vector< std::pair<unsigned int,std::string> >& origs);

            std::vector<unsigned int> GetDestinations() const;
			/*SWIG will leerzeichen bei manchen spitzen Klammern */
            void SetDestinations(std::vector< std::pair<unsigned int,std::string> >& dests);

            std::vector<unsigned int> GetReachedDests() const;
            std::unordered_map<ODPair, CompressedPath> GetShortestPaths() const;
            std::unordered_map<ODPair, double> GetODMatrix() const;

            double GetOptimum() const;
            std::vector<InternalArc> UncompressRoute(unsigned int orig, std::vector<unsigned int>& ends) const;
    };

    class ShortestPathTree : public ISolver
    {
        public:
            ShortestPathTree(Config& cnfg);
            virtual ~ShortestPathTree();

            void Solve(std::string net);
            void Solve(Network& net);

            SPTAlgorithm GetAlgorithm() const;
            void SetAlgorithm(SPTAlgorithm mstAlgorithm);

            int GetSPTHeapCard() const;
            void SetSPTHeapCard(int heapCard);

            GEOMETRY_HANDLING GetGeometryHandling() const;
            void SetGeometryHandling(GEOMETRY_HANDLING geomHandling);

            unsigned int GetOrigin() const;
            void SetOrigin(unsigned int orig);

            std::vector<unsigned int> GetDestinations() const;
            void SetDestinations(std::vector<unsigned int>& dests);

            std::vector<unsigned int> GetReachedDests() const;
            std::unordered_map<ODPair, CompressedPath> GetShortestPaths() const;

            double GetOptimum() const;

            std::vector<InternalArc> UncompressRoute(unsigned int orig, std::vector<unsigned int>& ends) const;
    };

    class MinCostFlow : public ISolver
    {
        public:
            MinCostFlow(Config& cnfg);
            virtual ~MinCostFlow();
            void Solve(string net);
            void Solve(Network& net);
            bool IsDirected;
            std::vector<netxpert::FlowCost> GetMinCostFlow() const;
            netxpert::MCFAlgorithm GetAlgorithm() const;
            void SetAlgorithm(MCFAlgorithm mcfAlgorithm);
            /*netxpert::MCFSolverStatus GetSolverStatus() const;*/
            double GetOptimum() const;
    };

    %feature("notabstract") Transportation;
    class Transportation : public MinCostFlow
    {
        public:
            Transportation(Config& cnfg);
            virtual ~Transportation();

            std::vector<unsigned int> GetOrigins() const;
            void SetOrigins(std::vector<unsigned int>  origs);

            std::vector<unsigned int> GetDestinations() const;
            void SetDestinations(std::vector<unsigned int>& dests);

            void SetExtODMatrix(std::vector<ExtSPTreeArc> _extODMatrix);
            void SetExtNodeSupply(std::vector<ExtNodeSupply> _nodeSupply);

            /*std::unordered_map<ODPair, DistributionArc> GetDistribution() const;*/

            ExtDistribution GetExtDistribution() const;
            std::string GetJSONExtDistribution() const;

            std::string GetSolverJSONResult() const;

            std::vector<InternalArc> UncompressRoute(unsigned int orig, std::vector<unsigned int>& ends) const;

            /**
            * Solves the Transportation Problem with the defined ODMatrix and NodeSupply property. Solves on pure
            * attribute data only i.e. output is always without geometry. (GEOMETRY_HANDLING is always set to "NoGeometry")
            */
            void Solve();
    };
}

%rename(ShortestPathTreeSimple) netxpert::simple::ShortestPathTree;
namespace netxpert::simple {
 class ShortestPathTree
 {
    public:
        ShortestPathTree(std::string jsonCnfg);
        int Solve();
        double GetOptimum();
 };
}

%rename(OriginDestinationMatrixSimple) netxpert::simple::OriginDestinationMatrix;
namespace netxpert::simple {
 class OriginDestinationMatrix
 {
    public:
        OriginDestinationMatrix(std::string jsonCnfg);
        int Solve();
        double GetOptimum();
 };
}

%rename(MinimumSpanningTreeSimple) netxpert::simple::MinimumSpanningTree;
namespace netxpert::simple {
 class MinimumSpanningTree
 {
    public:
        MinimumSpanningTree(std::string jsonCnfg);
        int Solve();
        double GetOptimum();
 };
}

%rename(TransportationSimple) netxpert::simple::Transportation;
namespace netxpert::simple {
 class Transportation
 {
    public:
        Transportation(std::string jsonCnfg);
        int Solve();
        double GetOptimum();
        std::string GetDistributionAsJSON();
        /* std::vector<netxpert::ExtDistributionArc> GetDistribution(); */
 };
}

%rename(MinimumCostFlowSimple) netxpert::simple::MinCostFlow;
namespace netxpert::simple {
 class MinCostFlow
 {
    public:
        MinCostFlow(std::string jsonCnfg);
        int Solve();
        double GetOptimum();
        /* TODO */
        std::string GetMinimumCostFlowAsJSON();
        /* std::vector<netxpert::FlowCost> GetMinimumCostFlow(); */
 };
}



/* Add nicer __str__() methods */
%extend netxpert::InputNode {
   char *__str__() {
       static char tmp [1024];
       sprintf(tmp,"InputNode('%s',%g)", $self->extNodeID.c_str(), $self->nodeSupply);
       return tmp;
   }
};
%extend netxpert::InputArc {
   char *__str__() {
       static char tmp [1024];
       sprintf(tmp,"InputArc('%s',%s,%s,%g,%g,'%s')", $self->extArcID.c_str(), $self->extFromNode.c_str(),
                                        $self->extToNode.c_str(), $self->cost, $self->capacity,
                                        $self->oneway.c_str());
       return tmp;
   }
};
