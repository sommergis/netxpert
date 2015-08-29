#ifndef DATA_H
#define DATA_H

#include "geos/geom/Coordinate.h"
#include "geos/geom/Geometry.h"
#include "geos/geom/MultiLineString.h"
#include "geos/geom/LineString.h"
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <vector>
#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/vector.hpp>
//#include <boost/bimap.hpp>
//#include <pair>

//Dictionary<Tuple<unsigned int, unsigned int>, Tuple<string, double, double>> internalArcData;
//TODO: data structure: LEMON? structs? maps?
using namespace std;
using namespace geos::geom;

namespace netxpert {

    /**
    * \const Value that shall be used instead of Infinity for arc values (e.g. capacity).
    **/
    const double DOUBLE_INFINITY = 999999;
    const double DOUBLE_NULL = -1;

    enum ArcIDColumnDataType
    {
        Number = 0, //double or int
        Std_String = 1
    };
    /**
    * \Enum that reflects the type of the node that was added to break an edge of the network.
    * \Needed for building the total geometry of the route, if the network has been broken up through
    * \additional start or end nodes.
    **/
    enum AddedNodeType
    {
        Undefined = 0,
        StartArc = 1,
        EndArc = 2
    };
    /**
    * \Enum that reflects the type of the Minimum Cost Flow instance.
    **/
    enum MinCostFlowInstanceType
    {
        MCFUndefined = 0,
        MCFBalanced = 1,
        MCFExtrasupply = 2,
        MCFExtrademand = 3
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

    enum StartOrEndLocationOfLine
    {
        Intermediate = 0,
        Start = 1,
        End = 2
    };

    typedef string ExtArcID;
    typedef string ExtNodeID;

    struct ExtClosestArcAndPoint
    {
        string extArcID;
        string extFromNode;
        string extToNode;
        double cost;
        double capacity;
        Coordinate closestPoint;
        shared_ptr<Geometry> arcGeom;
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
        ExtNodeID extNodeID;
        double supply;

        template<class Archive>
        void serialize( Archive & ar )
        {
            ar( cereal::make_nvp("nodeid", extNodeID),
                cereal::make_nvp("supply",supply) );
        }
    };

    /**
    * \Custom data type for storing external arc of SPTree/ODMatrix <extFromNode,extToNode,cost>
    **/
    struct ExtSPTreeArc
    {
        ExtArcID extArcID;
        ExternalArc extArc;
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

    typedef std::vector<netxpert::ExtSPTreeArc> ExtSPTArcs;
    typedef std::vector<netxpert::ExtNodeSupply> ExtNodeSupplies;

    struct ExtTransportationData
    {
        ExtSPTArcs odm;
        ExtNodeSupplies supply;

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
        string extArcID;
        double cost;
    };
    /**
    * \Custom data type for storing tuple <oldArcID,cost,capacity>
    **/
    struct ArcData
    {
        string extArcID;
        double cost;
        double capacity;
    };
    /**
    * \Custom data type for storing tuple <oldArcID,cost,capacity,flow>
    **/
    struct ArcDataAndFlow
    {
        string oldArcID;
        double cost;
        double capacity;
        double flow;
    };
    /**
    * \Custom data type for storing tuple <fromNode,toNode,flow,cost>
    **/
    struct FlowCost
    {
        InternalArc intArc;
        double flow;
        double cost;
    };

    /**
    * \Custom data type for storing tuple <CompressedPath,cost>
    **/
    typedef pair<vector<unsigned int>,double> CompressedPath;
    /**
    * \Custom data type for storing tuple <CompressedPath,flow>
    **/
    struct DistributionArc
    {
        CompressedPath path;
        double flow;
    };


    struct ExtDistributionArc
    {
        ExtArcID arcid;
        ExternalArc extArc;
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

    typedef std::vector<netxpert::ExtDistributionArc> ExtDistribution;

    /**
    * \Custom data type for storing the result of the Transpotation Solver in JSON-Format.
    **/
    struct TransportationResult
    {
        double optimum;
        std::vector<ExtDistributionArc> dist;

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
        std::vector<netxpert::ExternalArc> mst;

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
        std::vector<netxpert::ExtSPTreeArc> spt;

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
        string extNodeID;
        Coordinate coord;
    };
    /**
    * \Custom data type for storing tuple <extNodeID,supply>
    **/
    struct NodeSupply
    {
        string extNodeID;
        double supply;
    };

    /**
    * \Custom data type for storing new nodes data <extNodeID,<coord, supply>
    **/
    struct NewNode
    {
        string extNodeID;
        Coordinate coord;
        double supply;
    };

    /**
    * \Custom data type for storing tuple <arcGeom,nodeType,cost,capacity>
    **/
    struct NewArc
    {
        shared_ptr<LineString> arcGeom;
        //LineString& arcGeom;
        //LineString* arcGeom;
        AddedNodeType nodeType;
        double cost;
        double capacity;
    };

    struct SwappedOldArc
    {
        InternalArc ftNode;
        double cost;
        double capacity;
    };

    /**
    * \Custom data type for storing tuple <fromNode,toNode,arcGeom,cost,capacity>
    **/
    struct SplittedArc
    {
        InternalArc ftNode;
        double cost;
        double capacity;
        shared_ptr<MultiLineString> arcGeom;
    };

    struct InputArc
    {
        string extArcID;
        string extFromNode;
        string extToNode;
        double cost;
        double capacity;
        string oneway;
    };

    struct InputNode
    {
        string extNodeID;
        double nodeSupply;
    };

    struct ColumnMap
    {
        string arcIDColName;
        string fromColName;
        string toColName;
        string costColName;
        string capColName;
        string onewayColName;
        string nodeIDColName;
        string supplyColName;
    };

    typedef unsigned int IntNodeID;

    typedef unordered_map<InternalArc, ArcData> Arcs;
    //typedef boost::bimap< FTNode, ArcData > Arcs;
    typedef unordered_map<IntNodeID, AddedPoint> AddedPoints;
    typedef unordered_map<IntNodeID, NodeSupply> NodeSupplies;

    //TODO find structure of NewArcs / OldArcs
    //Dictionary<Tuple<uint, uint>, Tuple<IGeometry, AddedNodeType, double, double>> newEdges;
    typedef unordered_map<InternalArc, NewArc> NewArcs; //container for the new parts of arcs that where splitted
    //Dictionary<Tuple<uint, uint>, Tuple<string, double, double>> oldEdges;
    //Query in both directions necessary --> bimap?

    typedef std::vector<NewNode> NewNodes;

    typedef unordered_map<string, SwappedOldArc> SwappedOldArcs; //container for the original arcs that where splitted with original key

    //typedef map<string,string> ColumnMap;
    typedef vector<InputArc> InputArcs;
    typedef vector<InputNode> InputNodes;

    struct NetworkBuilderArc
    {
        ExtArcID extArcID;
        IntNodeID fromNode;
        IntNodeID toNode;
        double cost;
        double capacity;
        std::string oneway;
        shared_ptr<geos::geom::LineString> geom;
    };
    typedef vector<NetworkBuilderArc> NetworkBuilderArcs;

    typedef unordered_map< shared_ptr<geos::geom::Point>, IntNodeID> NetworkBuilderNodes;
}

namespace std
{

    /**
    * \Extension for custom key type InternalArc specifying how to hash; serves as key in
    * \unordered map.
    **/
    template <>
    class hash<netxpert::InternalArc>
    {
      public:
        long operator()(const netxpert::InternalArc& x) const
        {
            hash<string> z;
            return z(to_string(x.fromNode) + to_string(x.toNode));
        }
    };

    /**
    * \Equal_to operator for custom key type InternalArc in unordered map.
    **/
    template <>
    class equal_to<netxpert::InternalArc>
    {
      public:
         bool operator()(const netxpert::InternalArc& a, const netxpert::InternalArc& b) const
         {
            return a.fromNode == b.fromNode && a.toNode == b.toNode;
         }
    };

    /**
    * \Extension for custom key type ODPair specifying how to hash; serves as key in
    * \unordered map.
    **/
    template <>
    class hash<netxpert::ODPair>
    {
      public:
        long operator()(const netxpert::ODPair& x) const
        {
            hash<string> z;
            return z(to_string(x.origin) + to_string(x.dest));
        }
    };

    /**
    * \Equal_to operator for custom key type ODPair in unordered map.
    **/
    template <>
    class equal_to<netxpert::ODPair>
    {
      public:
         bool operator()(const netxpert::ODPair& a, const netxpert::ODPair& b) const
         {
            return a.origin == b.origin && a.dest == b.dest;
         }
    };
}
#endif // DATA_H
