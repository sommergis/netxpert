#ifndef DATA_H
#define DATA_H

#include "geos/geom/Coordinate.h"
#include "geos/geom/Geometry.h"
#include "geos/geom/MultiLineString.h"
#include "geos/geom/LineString.h"
#include <unordered_map>
#include <vector>
#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/vector.hpp>
//#include <boost/bimap.hpp>

//Dictionary<Tuple<unsigned int, unsigned int>, Tuple<string, double, double>> internalArcData;
//TODO: data structure: LEMON? structs? maps?

namespace netxpert {

    namespace data {

    /**
    * \const Value that shall be used instead of Infinity for arc values (e.g. capacity).
    **/
    const double DOUBLE_INFINITY = 999999;
    const double DOUBLE_NULL = -1;

    /**
    * \Enum
    * Enum that reflects the type of the netXpert Solver
    **/
    enum NetXpertSolver
    {
        UndefinedNetXpertSolver = -1,
        ShortestPathTreeSolver = 0,
        ODMatrixSolver = 1,
        TransportationSolver = 2,
        MinCostFlowSolver = 3,
        MinSpanningTreeSolver = 4,
        TransshipmentSolver = 5,
        NetworkBuilderResult = 6,
        IsolinesSolver = 7
    };

    /**
    * \Enum
    * Enum that reflects the type of the ArcID Column in the netxpert database. Used for building
    * the correct sql statements (e.g. SQL IN Clauses): text or numbers (double or int).
    **/
    enum ArcIDColumnDataType
    {
        Number = 0, //double or int
        Std_String = 1
    };
    /**
    * \Enum
    * Enum that reflects the type of the node that was added to break an edge of the network.
    * Needed for building the total geometry of the route, if the network has been broken up through
    * additional start or end nodes.
    **/
    enum AddedNodeType
    {
        UndefinedAddedNodeType = 0,
        StartArc = 1,
        EndArc = 2
    };
    /**
    * \Enum
    * Enum that reflects the type of the Minimum Cost Flow instance.
    **/
    enum MinCostFlowInstanceType
    {
        MCFUndefined = 0,     ///< undefined MCF problem type
        MCFBalanced = 1,      ///< balanced MCF problem
        MCFExtrasupply = 2,   ///< MCF problem with more supply than demand
        MCFExtrademand = 3    ///< MCF problem with more demand than supply
    };

    /**
    * \Enum
    * Enum that reflects the status of the Minimum Cost Flow solver.
    **/
    enum MCFSolverStatus
    {
       MCFUnSolved = -1 ,     ///< no solution available
       MCFOK = 0 ,            ///< optimal solution found
       MCFStopped = 1,        ///< optimization stopped
       MCFUnfeasible = 2,     ///< problem is unfeasible
       MCFUnbounded = 3,      ///< problem is unbounded
       MCFError = 4           ///< error in the solver
    };

    /**
    * \Enum
    * Enum that reflects the location of a given point on a line.
    **/
    enum StartOrEndLocationOfLine
    {
        Intermediate = 0,     ///< points location is somewhere between the start and end point of the line
        Start = 1,            ///< point is identical to the start point of the line
        End = 2               ///< point is identical to the end point of the line
    };

    typedef std::string ExtArcID;
    typedef std::string ExtNodeID;

    struct ExtClosestArcAndPoint
    {
        std::string extArcID;
        std::string extFromNode;
        std::string extToNode;
        double cost;
        double capacity;
        geos::geom::Coordinate closestPoint;
        std::shared_ptr<geos::geom::Geometry> arcGeom;
    };

    /**
    * \Custom data type for storing external nodes tuple <fromNode,toNode>
    **/
    struct ExternalArc
    {
        std::string extFromNode;
        std::string extToNode;

        template<class Archive>
        void serialize( Archive & ar )
        {
            ar( extFromNode, extToNode );
        }
    };

    /**
    *\Custom data type for storing external node supply <extNodeID,supply>
    **/
    struct ExtNodeSupply
    {
        netxpert::data::ExtNodeID extNodeID;
        double supply;

        template<class Archive>
        void serialize( Archive & ar )
        {
            ar( cereal::make_nvp("nodeid", extNodeID),
                cereal::make_nvp("supply", supply) );
        }
    };

    /**
    * \Custom data type for storing external arc of SPTree/ODMatrix <extFromNode,extToNode,cost>
    **/
    struct ExtSPTreeArc
    {
        netxpert::data::ExtArcID extArcID;
        netxpert::data::ExternalArc extArc;
        double cost;

        template<class Archive>
        void serialize( Archive & ar )
        {
            ar( cereal::make_nvp("arcid", extArcID),
                cereal::make_nvp("fromNode",extArc.extFromNode),
                cereal::make_nvp("toNode",extArc.extToNode),
                cereal::make_nvp("cost",cost) );
        }
    };

    /*struct ExtODMatrix
    {
        std::vector<ExtODMatrixArc> data;
        template<class Archive>
        void serialize( Archive & ar)
        {
            ar( cereal::make_nvp("odmatrix",data) );
        }
    };

    struct ExtNodeSupplies
    {
        std::vector<ExtNodeSupply> data;

        template<class Archive>
        void serialize( Archive & ar )
        {
            ar( cereal::make_nvp("supply",data));
        }
    };*/

    typedef std::vector<netxpert::data::ExtSPTreeArc> ExtSPTArcs;
    typedef std::vector<netxpert::data::ExtNodeSupply> ExtNodeSupplies;

    struct ExtTransportationData
    {
        netxpert::data::ExtSPTArcs odm;
        netxpert::data::ExtNodeSupplies supply;

        template<class Archive>
        void serialize( Archive & ar )
        {
            ar( cereal::make_nvp("odmatrix",odm),
                cereal::make_nvp("supply",supply));
        }
    };

    /**
    * \Custom data type for storing nodes tuple <fromNode,toNode>
    **/
    struct InternalArc
    {
        unsigned int fromNode;
        unsigned int toNode;

      bool operator==(const InternalArc& p2) const {
        const InternalArc& p1=(*this);
        return p1.fromNode == p2.fromNode && p1.toNode == p2.toNode;
      }
    };
    /**
    * \Custom data type for storing ODPair tuple <origin,dest>
    **/
    struct ODPair
    {
        unsigned int origin;
        unsigned int dest;

        bool operator==(const ODPair& p2) const {
          const ODPair& p1=(*this);
          return p1.origin == p2.origin && p1.dest == p2.dest;
        }
        template<class Archive>
        void serialize( Archive & ar )
        {
            ar( cereal::make_nvp("origin",origin),
                cereal::make_nvp("destination",dest) );
        }
    };
    /**
    * \Custom data type for storing tuple <oldArcID,cost,capacity>
    **/
    struct DuplicateArcData
    {
        std::string extArcID;
        double cost;
    };
    /**
    * \Custom data type for storing tuple <oldArcID,cost,capacity>
    **/
    struct ArcData
    {
        std::string extArcID;
        double cost;
        double capacity;
    };
    /**
    * \Custom data type for storing tuple <oldArcID,cost,capacity,flow>
    **/
    struct ArcDataAndFlow
    {
        std::string oldArcID;
        double cost;
        double capacity;
        double flow;
    };
    /**
    * \Custom data type for storing tuple <fromNode,toNode,flow,cost>
    **/
    struct FlowCost
    {
        netxpert::data::InternalArc intArc;
        double flow;
        double cost;
    };

    /**
    * \Custom data type for storing tuple <CompressedPath,cost>
    **/
    typedef std::pair<std::vector<unsigned int>,double> CompressedPath;
    /**
    * \Custom data type for storing tuple <CompressedPath,flow>
    **/
    struct DistributionArc
    {
        netxpert::data::CompressedPath path;
        double flow;
    };


    struct ExtDistributionArc
    {
        netxpert::data::ExtArcID arcid;
        netxpert::data::ExternalArc extArc;
        double cost;
        double flow;

        template<class Archive>
        void serialize( Archive & ar )
        {
            ar( cereal::make_nvp("arcid", arcid),
                cereal::make_nvp("fromNode",extArc.extFromNode),
                cereal::make_nvp("toNode",extArc.extToNode),
                cereal::make_nvp("cost",cost),
                cereal::make_nvp("flow",flow)   );
        }
    };

    typedef std::vector<netxpert::data::ExtDistributionArc> ExtDistribution;

    /**
    * \Custom data type for storing the result of the Transpotation Solver in JSON-Format.
    **/
    struct TransportationResult
    {
        double optimum;
        std::vector<netxpert::data::ExtDistributionArc> dist;

        template<class Archive>
        void serialize( Archive & ar )
        {
            ar( cereal::make_nvp("optimum", optimum),
                cereal::make_nvp("distribution", dist)   );
        }
    };

    /**
    * \Custom data type for storing the result of the MST Solver in JSON-Format.
    **/
    struct MSTResult
    {
        double optimum;
        std::vector<netxpert::data::ExternalArc> mst;

        template<class Archive>
        void serialize( Archive & ar )
        {
            ar( cereal::make_nvp("optimum", optimum),
                cereal::make_nvp("mst", mst)   );
        }
    };

    /**
    * \Custom data type for storing the result of the SPT Solver in JSON-Format.
    **/
    struct SPTResult
    {
        double optimum;
        std::vector<netxpert::data::ExtSPTreeArc> spt;

        template<class Archive>
        void serialize( Archive & ar )
        {
            ar( cereal::make_nvp("optimum", optimum),
                cereal::make_nvp("spt", spt)   );
        }
    };

    /**
    * \Custom data type for storing tuple <extNodeID,coord>
    **/
    struct AddedPoint
    {
        std::string extNodeID;
        geos::geom::Coordinate coord;
    };
    /**
    * \Custom data type for storing tuple <extNodeID,supply>
    **/
    struct NodeSupply
    {
        std::string extNodeID;
        double supply;
    };

    /**
    * \Custom data type for storing new nodes data <extNodeID,<coord, supply>
    **/
    struct NewNode
    {
        std::string extNodeID;
        geos::geom::Coordinate coord;
        double supply;
    };

    /**
    * \Custom data type for storing tuple <arcGeom,nodeType,cost,capacity>
    **/
    struct NewArc
    {
        std::shared_ptr<geos::geom::LineString> arcGeom;
        //LineString& arcGeom;
        //LineString* arcGeom;
        netxpert::data::AddedNodeType nodeType;
        double cost;
        double capacity;
    };

    struct SwappedOldArc
    {
        netxpert::data::InternalArc ftNode;
        double cost;
        double capacity;
    };

    /**
    * \Custom data type for storing tuple <fromNode,toNode,arcGeom,cost,capacity>
    **/
    struct SplittedArc
    {
        netxpert::data::InternalArc ftNode;
        double cost;
        double capacity;
        std::shared_ptr<geos::geom::MultiLineString> arcGeom;
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

    struct InputNode
    {
        std::string extNodeID;
        double nodeSupply;
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

    typedef unsigned int IntNodeID;

    typedef std::unordered_map<netxpert::data::InternalArc, netxpert::data::ArcData> Arcs;
    //typedef boost::bimap< FTNode, ArcData > Arcs;
    typedef std::unordered_map<netxpert::data::IntNodeID, netxpert::data::AddedPoint> AddedPoints;
    typedef std::unordered_map<netxpert::data::IntNodeID, netxpert::data::NodeSupply> NodeSupplies;

    //TODO find structure of NewArcs / OldArcs
    //Dictionary<Tuple<uint, uint>, Tuple<IGeometry, AddedNodeType, double, double>> newEdges;
    typedef std::unordered_map<netxpert::data::InternalArc, netxpert::data::NewArc> NewArcs; //container for the new parts of arcs that where splitted
    //Dictionary<Tuple<uint, uint>, Tuple<string, double, double>> oldEdges;
    //Query in both directions necessary --> bimap?

    typedef std::vector<netxpert::data::NewNode> NewNodes;

    typedef std::unordered_map<std::string, netxpert::data::SwappedOldArc> SwappedOldArcs; //container for the original arcs that where splitted with original key

    //typedef map<string,string> ColumnMap;
    typedef std::vector<netxpert::data::InputArc> InputArcs;
    typedef std::vector<netxpert::data::InputNode> InputNodes;

    struct NetworkBuilderInputArc
    {
        netxpert::data::ExtArcID extArcID;
        double cost;
        double capacity;
        std::string oneway;
		//Bug in VS2013: can't move for default copy
		//https://connect.microsoft.com/VisualStudio/feedback/details/858243/c-cli-compiler-error-trying-to-std-move-a-std-unique-ptr-to-parameter-taken-by-value
		//--> std::unique_ptr funktioniert nicht als data member
		std::shared_ptr<geos::geom::Geometry> geom;
    };
    typedef std::vector<netxpert::data::NetworkBuilderInputArc> NetworkBuilderInputArcs;

    struct NetworkBuilderResultArc
    {
        netxpert::data::ExtArcID extArcID;
        netxpert::data::IntNodeID fromNode;
        netxpert::data::IntNodeID toNode;
        double cost;
        double capacity;
        std::string oneway;
		//Bug in VS2013: can't move for default copy
		//https://connect.microsoft.com/VisualStudio/feedback/details/858243/c-cli-compiler-error-trying-to-std-move-a-std-unique-ptr-to-parameter-taken-by-value
		//--> std::unique_ptr funktioniert nicht als data member
        std::shared_ptr<geos::geom::Geometry> geom;

		//explicit assignment operator due to MSVC bug in VS2013
		/*
		NetworkBuilderResultArc& operator=(NetworkBuilderResultArc &&data)
		{
			geom = std::move(data.geom);
			return *this;
		}*/
    };
    typedef std::unordered_map< unsigned int, netxpert::data::NetworkBuilderResultArc> NetworkBuilderResultArcs;

    typedef std::unordered_map< std::unique_ptr<geos::geom::Point>, netxpert::data::IntNodeID> NetworkBuilderResultNodes;

 } //namespace data
} //namespace netxpert

namespace std
{

    /**
    * \Extension for custom key type InternalArc specifying how to hash; serves as key in
    * \unordered map.
    **/
    template <>
    class hash<netxpert::data::InternalArc>
    {
      public:
        long operator()(const netxpert::data::InternalArc& x) const
        {
            hash<std::string> z;
            return z(to_string(x.fromNode) + to_string(x.toNode));
        }
    };

    /**
    * \Equal_to operator for custom key type InternalArc in unordered map.
    **/
    template <>
    class equal_to<netxpert::data::InternalArc>
    {
      public:
         bool operator()(const netxpert::data::InternalArc& a, const netxpert::data::InternalArc& b) const
         {
            return a.fromNode == b.fromNode && a.toNode == b.toNode;
         }
    };

    /**
    * \Extension for custom key type ODPair specifying how to hash; serves as key in
    * \unordered map.
    **/
    template <>
    class hash<netxpert::data::ODPair>
    {
      public:
        long operator()(const netxpert::data::ODPair& x) const
        {
            hash<std::string> z;
            return z(to_string(x.origin) + to_string(x.dest));
        }
    };

    /**
    * \Equal_to operator for custom key type ODPair in unordered map.
    **/
    template <>
    class equal_to<netxpert::data::ODPair>
    {
      public:
         bool operator()(const netxpert::data::ODPair& a, const netxpert::data::ODPair& b) const
         {
            return a.origin == b.origin && a.dest == b.dest;
         }
    };
} //namespace std
#endif // DATA_H
