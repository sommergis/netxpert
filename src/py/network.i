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
#include "networkbuilder.h"
#include "isolines.h"

/* Simple Solver Interface */

#include "sptree_simple.h"
#include "odmatrix_simple.h"
#include "mstree_simple.h"
#include "transp_simple.h"
#include "mcfp_simple.h"
#include "netbuilder_simple.h"

%}

namespace std
{
    %template(InputArcs) std::vector<netxpert::data::InputArc>;
    %template(InputNodes) std::vector<netxpert::data::InputNode>;
    %template(NewNodes) std::vector<netxpert::data::NewNode>;
    %template(ExtSPTArcs) std::vector<netxpert::data::ExtSPTreeArc>;
    %template(ExtNodeSupplies) std::vector<netxpert::data::ExtNodeSupply>;
    %template(ExtDistribution) std::vector<netxpert::data::ExtDistributionArc>;
    %template(FlowCosts) std::vector<netxpert::data::FlowCost>;

    %template(Destinations) std::vector<unsigned int>;

    %template() std::pair<unsigned int,std::string>;
    %template(ODNodes) std::vector< std::pair<unsigned int,std::string> >;

    %template() std::vector<double>;
    %template(CutOffs) std::map<std::string, std::vector<double> >;
}

namespace netxpert
{
    static const std::string Version()
    {
        return "0.9.3";
    };

    namespace data {


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
        geos::geom::Coordinate coord;
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
        netxpert::data::InternalArc intArc;
        double flow;
        double cost;
    };

    struct NetworkBuilderResultArc
    {
        netxpert::data::ExtArcID extArcID;
        netxpert::data::IntNodeID fromNode;
        netxpert::data::IntNodeID toNode;
        double cost;
        double capacity;
        std::string oneway;
        std::shared_ptr<geos::geom::Geometry> geom;
    };

    typedef std::vector<netxpert::data::InputNode> InputNodes;
    typedef std::vector<netxpert::data::InputArc> InputArcs;
    typedef std::pair<std::vector<unsigned int>,double> CompressedPath;
    typedef std::vector<netxpert::data::NewNode> NewNodes;
    typedef std::string ExtArcID;
    typedef std::string ExtNodeID;
    typedef std::vector<netxpert::data::ExtSPTreeArc> ExtSPTArcs;
    typedef std::vector<netxpert::data::ExtNodeSupply> ExtNodeSupplies;
    typedef std::vector<netxpert::data::ExtDistributionArc> ExtDistribution;
    typedef std::vector<unsigned int> Destinations;
    typedef std::vector< std::pair<unsigned int,std::string> > ODNodes;

    struct ExtNodeSupply
    {
        netxpert::data::ExtNodeID extNodeID;
        double supply;
    };

    struct ExternalArc
    {
        std::string extFromNode;
        std::string extToNode;
    };
    struct ExtSPTreeArc
    {
        netxpert::data::ExtArcID extArcID;
        netxpert::data::ExternalArc extArc;
        double cost;
    };

    struct ExtTransportationData
    {
        netxpert::data::ExtSPTArcs odm;
        netxpert::data::ExtNodeSupplies supply;
    };

    struct ExtDistributionArc
    {
        netxpert::data::ExtArcID arcid;
        netxpert::data::ExternalArc extArc;
        double cost;
        double flow;
    };

    }/* namespace netxpert::data */

    namespace cnfg {

    enum GEOMETRY_HANDLING
    {
        NoGeometry = 0,
        StraightLines = 1,
        RealGeometry = 2
    };

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
           LogFatal = 4,
           LogQuiet = 5   };

    enum SPTAlgorithm {

        Dijkstra_MCFClass = 0,
        LQueue_MCFClass = 1,
        LDeque_MCFClass = 2,
        Dijkstra_Heap_MCFClass = 3,
        Dijkstra_2Heap_LEMON = 4,
        Bijkstra_2Heap_LEMON = 5,
        Dijkstra_dheap_BOOST = 6
    } ;

    enum MCFAlgorithm {
        NetworkSimplex_MCF = 0,
        NetworkSimplex_LEMON = 1
    } ;

    enum MSTAlgorithm {
        //Kruskal_QuickGraph = 0, //.NET!
        //Prim_QuickGraph = 1,    //.NET!
        Kruskal_LEMON = 2
    };

    /**
    * \Storage for the configuration of netxpert
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
        netxpert::cnfg::GEOMETRY_HANDLING GeometryHandling;//!< Member variable "geometryHandling"
        std::string OnewayColumnName;//!< Member variable "onewayColumnName"
        netxpert::cnfg::TESTCASE TestCase;//!< Member variable "testCase"
        bool CleanNetwork;//!< Member variable "cleanNetwork"
        netxpert::cnfg::LOG_LEVEL LogLevel;
        std::string LogFileFullPath;
    };
    }/* namespace netxpert::cnfg */


    class Network
    {
        public:
            Network(netxpert::data::InputArcs arcsTbl, netxpert::data::ColumnMap _map, netxpert::cnfg::Config& cnfg);
            Network(netxpert::data::InputArcs arcsTbl, netxpert::data::InputNodes nodesTbl,
                    netxpert::data::ColumnMap _map, netxpert::cnfg::Config& cnfg);
            void ConvertInputNetwork(bool autoClean);

            unsigned int AddStartNode(std::string extArcID,
                                      double x, double y, double supply,
                                      int treshold, const netxpert::data::ColumnMap& cmap, bool withCapacity);
            unsigned int AddEndNode(std::string extArcID,
                                      double x, double y, double supply,
                                      int treshold, const netxpert::data::ColumnMap& cmap, bool withCapacity);

            std::vector< std::pair<unsigned int, std::string> > LoadStartNodes(const std::vector<netxpert::data::NewNode>& newNodes, int treshold,
                                                            std::string arcsTableName, std::string geomColumnName,
                                                                netxpert::data::ColumnMap& cmap, bool withCapacity);
            std::vector< std::pair<unsigned int, std::string> > LoadEndNodes(const std::vector<netxpert::data::NewNode>& newNodes, int treshold,
                                                            std::string arcsTableName, std::string geomColumnName,
                                                                netxpert::data::ColumnMap& cmap, bool withCapacity);

            std::string GetOriginalNodeID(unsigned int internalNodeID);
            std::string GetOriginalStartOrEndNodeID(unsigned int internalNodeID);

            void Reset();
    };

    namespace utils {

    class LOGGER
    {
        protected:
            LOGGER() {}
        public:
            ~LOGGER() {}
            static void Initialize(netxpert::cnfg::Config& cnfg);
            static bool IsInitialized;
    };
    }

    namespace io {

    class DBHELPER
    {
        protected:
             DBHELPER(){}
        public:
            static void Initialize(netxpert::cnfg::Config& cnfg);
            static bool IsInitialized;
            static void CommitCurrentTransaction();
            static void OpenNewTransaction();
            static netxpert::data::InputArcs LoadNetworkFromDB(std::string _tableName, netxpert::data::ColumnMap _map);
            static netxpert::data::NewNodes LoadNodesFromDB(std::string _tableName, std::string geomColName, const netxpert::data::ColumnMap& _map);

            static void CloseConnection();
            ~DBHELPER();
    };
    }

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
            MinimumSpanningTree(netxpert::cnfg::Config& cnfg);
            virtual ~MinimumSpanningTree();

            void Solve(std::string net);
            void Solve(Network& net);

            netxpert::cnfg::MSTAlgorithm GetAlgorithm();
            void SetAlgorithm(netxpert::cnfg::MSTAlgorithm mstAlgorithm);

            double GetOptimum();

            void SaveResults(const std::string& resultTableName,
                 const netxpert::data::ColumnMap& cmap) const;

            std::vector<netxpert::data::InternalArc> GetMinimumSpanningTree() const;
    };

    class OriginDestinationMatrix : public ISolver
    {
        public:
            OriginDestinationMatrix(netxpert::cnfg::Config& cnfg);
            virtual ~OriginDestinationMatrix();

            void Solve(std::string net);
            void Solve(Network& net);

            netxpert::cnfg::SPTAlgorithm GetAlgorithm() const;
            void SetAlgorithm(netxpert::cnfg::SPTAlgorithm mstAlgorithm);

            int GetSPTHeapCard() const;
            void SetSPTHeapCard(int heapCard);

            netxpert::cnfg::GEOMETRY_HANDLING GetGeometryHandling() const;
            void SetGeometryHandling(netxpert::cnfg::GEOMETRY_HANDLING geomHandling);

            std::vector<unsigned int> GetOrigins() const;
			/*SWIG will leerzeichen bei manchen spitzen Klammern */
            void SetOrigins(std::vector< std::pair<unsigned int,std::string> >& origs);

            std::vector<unsigned int> GetDestinations() const;
			/*SWIG will leerzeichen bei manchen spitzen Klammern */
            void SetDestinations(std::vector< std::pair<unsigned int,std::string> >& dests);

            std::vector<unsigned int> GetReachedDests() const;
            std::unordered_map<netxpert::data::ODPair, netxpert::data::CompressedPath> GetShortestPaths() const;
            std::unordered_map<netxpert::data::ODPair, double> GetODMatrix() const;

            double GetOptimum() const;

            void SaveResults(const std::string& resultTableName,
                 const netxpert::data::ColumnMap& cmap) const;

            std::vector<netxpert::data::InternalArc> UncompressRoute(unsigned int orig, std::vector<unsigned int>& ends) const;
    };

    class ShortestPathTree : public ISolver
    {
        public:
            ShortestPathTree(netxpert::cnfg::Config& cnfg);
            virtual ~ShortestPathTree();

            void Solve(std::string net);
            void Solve(Network& net);

            netxpert::cnfg::SPTAlgorithm GetAlgorithm() const;
            void SetAlgorithm(netxpert::cnfg::SPTAlgorithm mstAlgorithm);

            int GetSPTHeapCard() const;
            void SetSPTHeapCard(int heapCard);

            netxpert::cnfg::GEOMETRY_HANDLING GetGeometryHandling() const;
            void SetGeometryHandling(netxpert::cnfg::GEOMETRY_HANDLING geomHandling);

            unsigned int GetOrigin() const;
            void SetOrigin(unsigned int orig);

            std::vector<unsigned int> GetDestinations() const;
            void SetDestinations(std::vector<unsigned int>& dests);

            std::vector<unsigned int> GetReachedDests() const;
            std::unordered_map<netxpert::data::ODPair, netxpert::data::CompressedPath> GetShortestPaths() const;

            double GetOptimum() const;

            void SaveResults(const std::string& resultTableName,
                             const netxpert::data::ColumnMap& cmap) const;

            std::vector<netxpert::data::InternalArc> UncompressRoute(unsigned int orig, std::vector<unsigned int>& ends) const;
    };

    class Isolines : public ISolver
    {
        public:
            Isolines(netxpert::cnfg::Config& cnfg);
            virtual ~Isolines();
            void Solve(std::string net);
            void Solve(Network& net);

            netxpert::cnfg::SPTAlgorithm GetAlgorithm() const;
            void SetAlgorithm(netxpert::cnfg::SPTAlgorithm mstAlgorithm);

            int GetSPTHeapCard() const;
            void SetSPTHeapCard(int heapCard);

            netxpert::cnfg::GEOMETRY_HANDLING GetGeometryHandling() const;
            void SetGeometryHandling(netxpert::cnfg::GEOMETRY_HANDLING geomHandling);

            std::vector< std::pair<unsigned int,std::string> > GetOrigins() const;
            void SetOrigins(std::vector< std::pair<unsigned int,std::string> >& origs);

            std::map<netxpert::data::ExtNodeID, std::vector<double> > GetCutOffs();
            void SetCutOffs(std::map<std::string, std::vector<double> >& cutOffs);

            double GetOptimum() const;

            void SaveResults(const std::string& resultTableName,
                             const netxpert::data::ColumnMap& cmap) const;

            std::unordered_map<netxpert::data::ODPair, netxpert::data::CompressedPath> GetShortestPaths() const;

            std::vector<netxpert::data::InternalArc> UncompressRoute(unsigned int orig, vector<unsigned int>& ends) const;
    };

    %feature("notabstract") MinCostFlow;
    class MinCostFlow : public ISolver
    {
        public:
            MinCostFlow(netxpert::cnfg::Config& cnfg);
            virtual ~MinCostFlow();
            void Solve(string net);
            void Solve(Network& net);
            bool IsDirected;
            std::vector<netxpert::data::FlowCost> GetMinCostFlow() const;
            netxpert::cnfg::MCFAlgorithm GetAlgorithm() const;
            void SetAlgorithm(netxpert::cnfg::MCFAlgorithm mcfAlgorithm);
            /*netxpert::MCFSolverStatus GetSolverStatus() const;*/
            double GetOptimum() const;
            void SaveResults(const std::string& resultTableName,
                 const netxpert::data::ColumnMap& cmap) const;
    };

    %feature("notabstract") Transportation;
    class Transportation : public MinCostFlow
    {
        public:
            Transportation(netxpert::cnfg::Config& cnfg);
            virtual ~Transportation();

            void Solve();
            void Solve(Network& net);

            std::vector<unsigned int> GetOrigins() const;
            void SetOrigins(std::vector<unsigned int>  origs);

            std::vector<unsigned int> GetDestinations() const;
            void SetDestinations(std::vector<unsigned int>& dests);

            void SetExtODMatrix(std::vector<netxpert::data::ExtSPTreeArc> _extODMatrix);
            void SetExtNodeSupply(std::vector<netxpert::data::ExtNodeSupply> _nodeSupply);

            /*std::unordered_map<netxpert::data::ODPair, netxpert::data::DistributionArc> GetDistribution() const;*/

            netxpert::data::ExtDistribution GetExtDistribution() const;
            std::string GetJSONExtDistribution() const;

            std::string GetSolverJSONResult() const;

            std::vector<netxpert::data::InternalArc> UncompressRoute(unsigned int orig,
                                                                     std::vector<unsigned int>& ends) const;

            void SaveResults(const std::string& resultTableName,
                 const netxpert::data::ColumnMap& cmap) const;
    };

    class NetworkBuilder
    {
        public:
            NetworkBuilder(netxpert::cnfg::Config& cnfg);
            virtual ~NetworkBuilder()  {};
            void LoadData();
            void SaveResults(const std::string& resultTableName, const netxpert::data::ColumnMap& cmap) const;
            std::unordered_map<unsigned int, netxpert::data::NetworkBuilderResultArc> GetBuiltNetwork();
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
        /* std::vector<netxpert::data::ExtDistributionArc> GetDistribution(); */
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
        std::string GetMinimumCostFlowAsJSON();
 };
}

%rename(NetworkBuilderSimple) netxpert::simple::NetworkBuilder;
namespace netxpert::simple {
 class NetworkBuilder
 {
    public:
        NetworkBuilder(std::string jsonCnfg);
        virtual ~NetworkBuilder();
        int Build();
        std::string GetBuiltNetworkAsJSON();
 };
}




/* Add nicer __str__() methods */
%extend netxpert::data::InputNode {
   char *__str__() {
       static char tmp [1024];
       sprintf(tmp,"InputNode('%s',%g)", $self->extNodeID.c_str(), $self->nodeSupply);
       return tmp;
   }
};
%extend netxpert::data::InputArc {
   char *__str__() {
       static char tmp [1024];
       sprintf(tmp,"InputArc('%s',%s,%s,%g,%g,'%s')", $self->extArcID.c_str(), $self->extFromNode.c_str(),
                                        $self->extToNode.c_str(), $self->cost, $self->capacity,
                                        $self->oneway.c_str());
       return tmp;
   }
};
