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

%module pynetxpert

// we must define this macro here again if wanted (SWIG!)
//#define NETX_ENABLE_CONTRACTION_HIERARCHIES

%include "typemaps.i"
/* for uint32_t type */
%include "stdint.i"
%include "std_string.i"
%include "std_map.i"
%include "std_vector.i"
%include "std_pair.i"

%{
#include "stdint.h"
#include "lemon-net.hpp"
#include "dbhelper.hpp"
#include "isolver.hpp"
#include "sptree.hpp"
#include "odmatrix.hpp"
#include "mstree.hpp"
#include "isolines.hpp"
#include "mcflow.hpp"
#include "transportation.hpp"

/* Simple Solver Interface */

#include "sptree_simple.hpp"
#include "odmatrix_simple.hpp"
#include "mstree_simple.hpp"
#include "mcfp_simple.hpp"
#include "transp_simple.hpp"
#include "netbuilder_simple.hpp"

/* UTILS */
#include "utils.hpp"
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

    /* netxpert::data::node_t does not work here */
    %template(Nodes) std::vector<uint32_t>;

    %template() std::pair<uint32_t,std::string>;
    %template(ODNodes) std::vector< std::pair<uint32_t,std::string> >;

    %template() std::vector<double>;
    %template(CutOffs) std::map<std::string, std::vector<double> >;
}

namespace netxpert
{
    static const std::string Version();

    namespace data {

    const double DOUBLE_INFINITY = 999999;
    const double DOUBLE_NULL = -1;


    typedef lemon::SmartDigraph graph_t;

    /*typedef lemon::SmartDigraph::Node node_t;
    typedef lemon::SmartDigraph::Arc arc_t;*/

    typedef double cost_t;
    typedef double capacity_t;
    typedef double supply_t;
    typedef uint32_t  intarcid_t;
    typedef uint32_t  extarcid_t;


    /**
    * \Custom data type for storing tuple <Arc,arcGeom,cost,capacity>
    **/
    template<typename arc_t>
    struct IntNetSplittedArc
    {
        arc_t       arc;
        cost_t      cost;
        capacity_t  capacity;
        std::shared_ptr<geos::geom::MultiLineString> arcGeom;
    };

    enum MCFSolverStatus : int16_t {
       MCFUnSolved = -1 ,     ///< no solution available
       MCFOK = 0 ,            ///< optimal solution found
       MCFStopped = 1,        ///< optimization stopped
       MCFUnfeasible = 2,     ///< problem is unfeasible
       MCFUnbounded = 3,      ///< problem is unbounded
       MCFError = 4           ///< error in the solver
    };

    enum MinCostFlowInstanceType : int16_t {
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

    struct InputArc
    {
        std::string extArcID;
        std::string extFromNode;
        std::string extToNode;
        cost_t      cost;
        capacity_t  capacity;
        std::string oneway;
    };

    struct InputNode
    {
        std::string extNodeID;
        supply_t    nodeSupply;
    };

    struct NewNode
    {
        std::string             extNodeID;
        geos::geom::Coordinate  coord;
        supply_t                supply;
    };

    struct ODPair
    {
        lemon::SmartDigraph::Node origin;
        lemon::SmartDigraph::Node dest;
    };

    /* TODO
    struct InternalArc
    {
        uint32_t fromNode;
        uint32_t toNode;
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
    };*/

    typedef std::vector<netxpert::data::InputNode> InputNodes;
    typedef std::vector<netxpert::data::InputArc> InputArcs;
    typedef std::pair<std::vector<lemon::SmartDigraph::Arc>, netxpert::data::cost_t> CompressedPath;
    typedef std::vector<netxpert::data::NewNode> NewNodes;
    typedef std::string ExtArcID;
    typedef std::string ExtNodeID;

    typedef std::vector<netxpert::data::ExtSPTreeArc> ExtSPTArcs;
    typedef std::vector<netxpert::data::ExtNodeSupply> ExtNodeSupplies;
    typedef std::vector<netxpert::data::ExtDistributionArc> ExtDistribution;

    typedef std::vector< uint32_t > Nodes;
    typedef std::vector< std::pair<lemon::SmartDigraph::Node,std::string> > ODNodes;

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

    enum GEOMETRY_HANDLING : int16_t {
        NoGeometry = 0,
        StraightLines = 1,
        RealGeometry = 2
    };

    enum TESTCASE : int16_t {
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

    enum RESULT_DB_TYPE : int16_t {
        SpatiaLiteDB = 0,
        ESRI_FileGDB = 1,
        JSON = 2,
        GooglePolyLine = 3
    };

    enum LOG_LEVEL : int16_t {
           LogAll = -1,
           LogDebug = 0,
           LogInfo = 1,
           LogWarning = 2,
           LogError = 3,
           LogFatal = 4,
           LogQuiet = 5   };

    enum SPTAlgorithm : int16_t {
        Dijkstra_2Heap_LEMON = 4,
        Bijkstra_2Heap_LEMON = 5,
        Dijkstra_dheap_BOOST = 6
    } ;

    enum MCFAlgorithm : int16_t {
        NetworkSimplex_LEMON = 1
    } ;

    enum MSTAlgorithm : int16_t {
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
        int Threshold; //!< Member variable "threshold" for distance search: closest edge of network to given point
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
    };

    class ConfigReader {
      public:
        ConfigReader() {}
        ~ConfigReader() {}
        netxpert::cnfg::Config GetConfigFromJSON(std::string jsonString);
        netxpert::data::ColumnMap GetColumnMap(netxpert::cnfg::Config& cnfg);
    };

    }/* namespace netxpert::cnfg */

    class NetworkBuilder
    {
        public:

            NetworkBuilder(netxpert::cnfg::Config& cnfg);

            ~NetworkBuilder()  {};

            /**
            * Loads the Edge Data into a Graph.
            * Caution:
            *     - There is no check for planarity of the input!
            *     - Multilinestrings that cannot be merged as a Linestring will throw an exception
            **/
            void LoadData();
            void SaveResults(const std::string& resultTableName, const netxpert::data::ColumnMap& cmap) const;
            void BuildNetwork();
            std::unordered_map<uint32_t, netxpert::data::NetworkBuilderResultArc> GetBuiltNetwork();
    };

    /* Not that not all of the InternalNet functionality is exposed through SWIG */
    namespace data {

    %rename(Network) InternalNet;
    class InternalNet
    {
        public:
            InternalNet(const netxpert::data::InputArcs& arcsTbl,
                        const netxpert::data::ColumnMap& _map = netxpert::data::ColumnMap(),
                        const netxpert::cnfg::Config& cnfg = netxpert::cnfg::Config(),
                        const netxpert::data::InputNodes& nodesTbl = netxpert::data::InputNodes(),
                        const bool autoClean = true);

            const uint32_t
             AddStartNode(std::string extArcID,
                                      double x, double y, netxpert::data::supply_t supply,
                                      int threshold,
                                      const netxpert::data::ColumnMap& cmap,
                                      bool withCapacity);
            const uint32_t
             AddEndNode(std::string extArcID,
                                    double x, double y, netxpert::data::supply_t supply,
                                    int threshold,
                                    const netxpert::data::ColumnMap& cmap,
                                    bool withCapacity);

            std::vector< std::pair<uint32_t, std::string> >
             LoadStartNodes(std::vector<netxpert::data::NewNode> newNodes, const int threshold,
                            const std::string arcsTableName, const std::string geomColumnName,
                            const netxpert::data::ColumnMap& cmap, const bool withCapacity);

            std::vector< std::pair<uint32_t, std::string> >
             LoadEndNodes(std::vector<netxpert::data::NewNode> newNodes, const int threshold,
                          const std::string arcsTableName, const std::string geomColumnName,
                          const netxpert::data::ColumnMap& cmap, const bool withCapacity);
            void
             Reset();

            const uint32_t
             GetArcCount();

            const uint32_t
             GetNodeCount();

            void
             ExportToDIMACS(const std::string& path);

            #ifdef NETX_ENABLE_CONTRACTION_HIERARCHIES
            void
             ComputeContraction(float contractionPercent);

            void
             ExportContractedNetwork(const std::string graphName);

            void
             ImportContractedNetwork(const std::string graphName);
            #endif
    };
    }

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
            virtual void Solve(netxpert::data::InternalNet& net) = 0;
    };

    class ShortestPathTree : public netxpert::ISolver
    {
        public:
            /** Default constructor */
            ShortestPathTree(netxpert::cnfg::Config& cnfg);

            /** Default destructor */
            virtual ~ShortestPathTree() {}

            void Solve(std::string net);
            void Solve(netxpert::data::InternalNet& net);

            netxpert::cnfg::SPTAlgorithm GetAlgorithm() const;
            void SetAlgorithm(netxpert::cnfg::SPTAlgorithm mstAlgorithm);

            int GetSPTHeapCard() const;
            void SetSPTHeapCard(int heapCard);

            netxpert::cnfg::GEOMETRY_HANDLING GetGeometryHandling() const;
            void SetGeometryHandling(netxpert::cnfg::GEOMETRY_HANDLING geomHandling);

            void SetOrigin(const uint32_t orig);
            const uint32_t GetOriginID() const;

            void SetDestinations(const std::vector<uint32_t>& dests);
            std::vector<uint32_t> GetDestinationIDs() const;

            std::vector<uint32_t> GetReachedDestIDs() const;

            std::map<netxpert::data::ODPair, netxpert::data::CompressedPath> GetShortestPaths() const;

            const double GetOptimum() const;

            void SaveResults(const std::string& resultTableName,
                             const netxpert::data::ColumnMap& cmap);

            const std::string GetResultsAsJSON();
    };

    class OriginDestinationMatrix : public netxpert::ISolver
    {
        public:
            /** Default constructor */
            OriginDestinationMatrix(netxpert::cnfg::Config& cnfg);

            /** Default destructor */
            ~OriginDestinationMatrix() {}

            void Solve(std::string net);
            void Solve(netxpert::data::InternalNet& net);

            netxpert::cnfg::SPTAlgorithm GetAlgorithm() const;
            void SetAlgorithm(netxpert::cnfg::SPTAlgorithm mstAlgorithm);

            int GetSPTHeapCard() const;
            void SetSPTHeapCard(int heapCard);

            netxpert::cnfg::GEOMETRY_HANDLING GetGeometryHandling() const;
            void SetGeometryHandling(netxpert::cnfg::GEOMETRY_HANDLING geomHandling);

            void SetOrigins(const std::vector<uint32_t>& origs);
            std::vector<uint32_t> GetOriginIDs() const;

            void SetDestinations(const std::vector<uint32_t>& dests);
            std::vector<uint32_t> GetDestinationIDs() const;

            std::vector<uint32_t> GetReachedDestIDs() const;

            std::map<netxpert::data::ODPair, netxpert::data::CompressedPath> GetShortestPaths() const;
            std::map<netxpert::data::ODPair, double> GetODMatrix() const;

            const double GetOptimum() const;

            void SaveResults(const std::string& resultTableName,
                             const netxpert::data::ColumnMap& cmap);

            const std::string GetResultsAsJSON();
    };

    class Isolines : public netxpert::ISolver
    {
        public:
            Isolines(netxpert::cnfg::Config& cnfg);
            virtual ~Isolines() {}
            void Solve(std::string net);
            void Solve(netxpert::data::InternalNet& net);

            netxpert::cnfg::SPTAlgorithm GetAlgorithm() const;
            void SetAlgorithm(netxpert::cnfg::SPTAlgorithm mstAlgorithm);

            int GetSPTHeapCard() const;
            void SetSPTHeapCard(int heapCard);

            netxpert::cnfg::GEOMETRY_HANDLING GetGeometryHandling() const;
            void SetGeometryHandling(netxpert::cnfg::GEOMETRY_HANDLING geomHandling);

            void SetOrigins(const std::vector<uint32_t>& origs);
            std::vector<uint32_t> GetOriginIDs() const;

            std::map<netxpert::data::ExtNodeID, std::vector<double> > GetCutOffs();
            void SetCutOffs(std::map<netxpert::data::ExtNodeID, std::vector<double> >& cutOffs);

            const double GetOptimum() const;

            void SaveResults(const std::string& resultTableName,
                             const netxpert::data::ColumnMap& cmap) const;

            std::map<netxpert::data::ODPair, netxpert::data::CompressedPath> GetShortestPaths() const;
    };

    class MinimumSpanningTree : public netxpert::ISolver
    {
        public:
            /** Default constructor */
            MinimumSpanningTree(netxpert::cnfg::Config& cnfg);

            /** Default destructor */
            ~MinimumSpanningTree() {}

            void Solve(std::string net);
            void Solve(netxpert::data::InternalNet& net);

            netxpert::cnfg::MSTAlgorithm GetAlgorithm() const;
            void SetAlgorithm(netxpert::cnfg::MSTAlgorithm mstAlgorithm);

            const double GetOptimum() const;
            std::vector<netxpert::data::arc_t> GetMinimumSpanningTree() const;
            void SaveResults(const std::string& resultTableName,
                             const netxpert::data::ColumnMap& cmap) const;
    };

    %feature("notabstract") MinCostFlow;
    class MinCostFlow : public ISolver
    {
        public:
            MinCostFlow(netxpert::cnfg::Config& cnfg);
            virtual ~MinCostFlow();
            void Solve(std::string net);
            void Solve(netxpert::data::InternalNet& net);
            bool IsDirected;
            std::vector<netxpert::data::FlowCost> GetMinCostFlow() const;
            netxpert::cnfg::MCFAlgorithm GetAlgorithm() const;
            void SetAlgorithm(netxpert::cnfg::MCFAlgorithm mcfAlgorithm);
            /*netxpert::MCFSolverStatus GetSolverStatus() const;*/
            const double GetOptimum() const;
            void SaveResults(const std::string& resultTableName,
                 const netxpert::data::ColumnMap& cmap);
            const std::string GetResultsAsJSON();
    };

    %feature("notabstract") Transportation;
    class Transportation : public MinCostFlow
    {
        public:
            Transportation(netxpert::cnfg::Config& cnfg);
            virtual ~Transportation();

            void Solve();
            void Solve(netxpert::data::InternalNet& net);

            std::vector<uint32_t> GetOriginIDs() const;
            void SetOrigins(std::vector<uint32_t>  origs);

            std::vector<uint32_t> GetDestinationIDs() const;
            void SetDestinations(std::vector<uint32_t>& dests);

            void SetExtODMatrix(std::vector<netxpert::data::ExtSPTreeArc> _extODMatrix);
            void SetExtNodeSupply(std::vector<netxpert::data::ExtNodeSupply> _nodeSupply);

            netxpert::data::ExtDistribution GetExtDistribution() const;
            std::string GetJSONExtDistribution() const;

            std::string GetSolverJSONResult() const;

            void SaveResults(const std::string& resultTableName,
                 const netxpert::data::ColumnMap& cmap) const;
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

/* Helper
namespace netxpert {
 namespace utils {

  class UTILS
  {
    protected:
      UTILS(){}
    public:
      template<typename T>
      static T DeserializeJSONtoObject(std::string _jsonString);
  };
 }
}
%template(ConvertJSONtoConfig) netxpert::utils::UTILS::DeserializeJSONtoObject<netxpert::cnfg::Config>; */






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

