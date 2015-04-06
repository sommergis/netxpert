#ifndef DATA_H
#define DATA_H

#include <geos/geom/Coordinate.h>
#include <geos/geom/Geometry.h>
#include <geos/geom/MultiLineString.h>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <vector>
#include <boost/bimap.hpp>
//#include <pair>

//Dictionary<Tuple<unsigned int, unsigned int>, Tuple<string, double, double>> internalArcData;
//TODO: data structure: LEMON? structs? maps?
using namespace std;
using namespace geos::geom;

namespace NetXpert {

    /**
    * \Maximum Value that shall be used instead of Infinity for arc values (e.g. capacity).
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
        StartEdge = 1,
        EndEdge = 2
    };

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

    enum StartOrEndLocationOfLine
    {
        Intermediate = 0,
        Start = 1,
        End = 2
    };

    /**
    * \Custom data type for storing external nodes tuple <fromNode,toNode>
    **/
    struct ExtFTNode
    {
        string extFromNode;
        string extToNode;
    };
    /**
    * \Custom data type for storing nodes tuple <fromNode,toNode>
    **/
    struct FTNode
    {
        unsigned int fromNode;
        unsigned int toNode;
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
        Geometry& arcGeom;
        AddedNodeType nodeType;
        double cost;
        double capacity;
    };

    /**
    * \Custom data type for storing tuple <fromNode,toNode,arcGeom,cost,capacity>
    **/
    struct SplittedArc
    {
        unsigned int fromNode;
        unsigned int toNode;
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
    typedef string ExtNodeID;
    typedef string ExtArcID;

    typedef unordered_map<FTNode, ArcData> Arcs;
    //typedef boost::bimap< FTNode, ArcData > Arcs;
    typedef unordered_map<IntNodeID, AddedPoint> AddedPoints;
    typedef unordered_map<IntNodeID, NodeSupply> NodeSupplies;
    typedef unordered_map<FTNode, NewArc> NewArcs;

    //typedef map<string,string> ColumnMap;
    typedef list<InputArc> InputArcs;
    typedef list<InputNode> InputNodes;
}

namespace std
{
    /**
    * \Extension for custom key type FTNode specifying how to hash; serves as key in
    * \unordered map.
    **/
    template <>
    class hash<NetXpert::FTNode>
    {
      public:
        long operator()(const NetXpert::FTNode& x) const
        {
            hash<string> z;
            return z(to_string(x.fromNode) + to_string(x.toNode));
        }
    };

    /**
    * \Equal_to operator for custom key type FTNode in unordered map.
    **/
    template <>
    class equal_to<NetXpert::FTNode>
    {
      public:
         bool operator()(const NetXpert::FTNode& a, const NetXpert::FTNode& b) const
         {
            return a.fromNode == b.fromNode && a.toNode == b.toNode;
         }
    };
}

#endif // DATA_H
