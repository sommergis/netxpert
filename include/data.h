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

#ifndef DATA_H
#define DATA_H

#include <stdint.h>
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

#include <lemon/smart_graph.h>
#include <lemon/list_graph.h>
#include <lemon/adaptors.h>

namespace netxpert {

    namespace data {

    const double DOUBLE_INFINITY = 999999;  //!< \brief constant value that shall be used instead of Infinity for arc values (e.g. capacity).
    const double DOUBLE_NULL = -1; //!< \brief constant value that represents NULL for double values

    typedef lemon::SmartDigraph graph_t;  //!< \brief type of the regular internal graph
    typedef lemon::ListDigraph graph_ch_t;  //!< \brief type of the contraction hierarchy graph
    typedef lemon::FilterArcs<netxpert::data::graph_t,
                              netxpert::data::graph_t::ArcMap<bool>> filtered_graph_t;  //!< \brief type of the filtered internal graph
    typedef graph_t::Node node_t;  //!< \brief node type of internal graph
    typedef graph_t::Arc arc_t;  //!< \brief arc type of internal graph
    typedef double cost_t;  //!< \brief cost type of internal graph
    typedef double flow_t;  //!< \brief flow type of internal graph (how much is actually transported over the arc)
    typedef double capacity_t;  //!< \brief capacity type of internal graph (how much can be transported at maximum over the arc)
    typedef double supply_t;  //!< \brief supply type
    typedef uint32_t  intarcid_t;  //!< \brief internal arc ID
    //typedef uint32_t  extarcid_t; // does not work for MCF-Solver ('dummy'!)
    typedef std::string  extarcid_t; //!< \brief original arc ID - string type is necessary for MCF-Solver ('dummy'!)


    /**
    * \brief Custom data type for storing <arc,cost,capacity,segmentGeoms>
    **/
    template<typename arc_t>
    struct IntNetSplittedArc
    {
        arc_t       arc;
        cost_t      cost;
        capacity_t  capacity;
//        std::shared_ptr<geos::geom::MultiLineString> arcGeom;
        std::pair< std::shared_ptr<geos::geom::Geometry>,
                   std::shared_ptr<geos::geom::Geometry> > segments;
    };

    template<typename arc_t>
    struct IntNetSplittedArc2
    {
        arc_t       arc;
        cost_t      cost;
        capacity_t  capacity;
//        std::shared_ptr<geos::geom::MultiLineString> arcGeom;
        std::vector< std::shared_ptr<geos::geom::Geometry>> segments;
    };

    enum ArcState : int32_t {
        original = 0,
        originalAndSplit = 1,
        added = 2,
        addedAndSplit = 3
    };

    /**
    * \brief Enum that reflects the type of the netXpert Solver
    **/
    enum NetXpertSolver : int16_t {
        UndefinedNetXpertSolver = -1, //!< \brief Solver type is undefined
        ShortestPathTreeSolver = 0,   //!< \brief Shortest Path solver type
        ODMatrixSolver = 1,           //!< \brief ODMatrix solver type
        TransportationSolver = 2,     //!< \brief Transportation solver type
        MinCostFlowSolver = 3,        //!< \brief Minimum Cost Flow solver type
        MinSpanningTreeSolver = 4,    //!< \brief Minimum Spanning Tree solver type
        TransshipmentSolver = 5,      //!< \brief Transshipment solver type
        NetworkBuilderResult = 6,     //!< \brief NetworkBuilder solver type
        IsolinesSolver = 7            //!< \brief Isolines solver type \warning experimental!
    };

    /**
    * \brief Enum that reflects the type of the ArcID Column in the netxpert database. Used for building
    * the correct sql statements (e.g. SQL IN Clauses): text or numbers (double or int).
    **/
    enum ArcIDColumnDataType : int16_t {
        Number = 0,     //!< \brief double or integer type
        Std_String = 1  //!< \brief string type
    };
    /**
    * \brief Enum that reflects the type of the node that was added to break an arc of the network.
    * Needed for building the total geometry of the route, if the network has been broken up through
    * additional start or end nodes.
    **/
    enum AddedNodeType : int16_t {
        UndefinedAddedNodeType = 0, //!< \brief undefined added node type
        StartArc = 1,               //!< \brief node was set as start
        EndArc = 2                  //!< \brief node was set as end
    };
    /**
    * \brief Enum that reflects the type of the Minimum Cost Flow instance.
    **/
    enum MinCostFlowInstanceType : int16_t {
        MCFUndefined = 0,     //!< undefined MCF problem type
        MCFBalanced = 1,      //!< balanced MCF problem
        MCFExtrasupply = 2,   //!< MCF problem with more supply than demand
        MCFExtrademand = 3    //!< MCF problem with more demand than supply
    };

    /**
    * \brief Enum that reflects the status of the Minimum Cost Flow solver.
    **/
    enum MCFSolverStatus : int16_t {
       MCFUnSolved = -1 ,     //!< no solution available
       MCFOK = 0 ,            //!< optimal solution found
       MCFStopped = 1,        //!< optimization stopped
       MCFUnfeasible = 2,     //!< problem is unfeasible
       MCFUnbounded = 3,      //!< problem is unbounded
       MCFError = 4           //!< error in the solver
    };

    /**
    * \brief Enum that reflects the location of a given point on a line.
    **/
    enum StartOrEndLocationOfLine : int16_t {
        Intermediate = 0,     //!< points location is somewhere between the start and end point of the line
        Start = 1,            //!< point is identical to the start point of the line
        End = 2               //!< point is identical to the end point of the line
    };

    typedef std::string ExtArcID;
    typedef std::string ExtNodeID;

    struct ExtClosestArcAndPoint
    {
        std::string extArcID;
        std::string extFromNode;
        std::string extToNode;
        cost_t      cost;
        capacity_t  capacity;
        geos::geom::Coordinate closestPoint;
        std::shared_ptr<geos::geom::Geometry> arcGeom;
    };

    /**
    * \brief Custom data type for storing external nodes tuple <fromNode,toNode> as simple type variant
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
    *\brief Data type for storing external node supply <extNodeID,supply> as simple type variant
    **/
    struct ExtNodeSupply
    {
        netxpert::data::ExtNodeID extNodeID;
        supply_t                  supply;

        template<class Archive>
        void serialize( Archive & ar )
        {
            ar( cereal::make_nvp("nodeid", extNodeID),
                cereal::make_nvp("supply", supply) );
        }
    };

    /**
    * \brief Custom data type for storing external arc of SPTree/ODMatrix <extFromNode,extToNode,cost> as simple type variant
    **/
    struct ExtSPTreeArc
    {
        netxpert::data::ExtArcID    extArcID;
        netxpert::data::ExternalArc extArc;
        cost_t                      cost;

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
    * \brief Custom data type for storing nodes tuple <fromNode,toNode>
    **/
    struct InternalArc
    {
        uint32_t fromNode;
        uint32_t toNode;

      bool operator==(const InternalArc& p2) const {
        const InternalArc& p1=(*this);
        return p1.fromNode == p2.fromNode && p1.toNode == p2.toNode;
      }
    };

    /*
    /**
    * \brief Custom data type for storing ODPair tuple <origin,dest>
    **/
    /*
    struct ODPair
    {
        uint32_t origin;
        uint32_t dest;

        bool operator==(const ODPair& p2) const {
          const ODPair_& p1=(*this);
          return p1.origin == p2.origin && p1.dest == p2.dest;
        }
        template<class Archive>
        void serialize( Archive & ar )
        {
            ar( cereal::make_nvp("origin",origin),
                cereal::make_nvp("destination",dest) );
        }
    };*/


    /**
    * \brief Custom data type for storing ExtODPair tuple <origin,dest> as simple type variant
    **/
    struct ExtODPair
    {
        uint32_t origin;
        uint32_t dest;

        bool operator==(const ExtODPair& p2) const {
          const ExtODPair& p1=(*this);
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
    * \brief Custom data type for storing lemon ODPair tuple <origin,dest>
    **/
    struct ODPair
    {
        netxpert::data::node_t origin;
        netxpert::data::node_t dest;

        bool operator==(const ODPair& p2) const {
          bool ret = (this->origin == p2.origin) && (this->dest == p2.dest);
          return ret;
        }
        bool operator<(const ODPair& p2) const {
          //CRITICAL for correct use of ODPair Class in std::map!

          // order first by origin and then by dest!
          if ( this->origin != p2.origin )
            return (this->origin < p2.origin );
          else
            return (this->dest < p2.dest );

        }
        // will not work in archives
        /*template<class Archive>
        void serialize( Archive & ar )
        {
            ar( cereal::make_nvp("origin",origin),
                cereal::make_nvp("destination",dest) );
        }*/
    };

    /**
    * \brief Data type for storing tuple <extArcID,cost,capacity>
    **/
    struct DuplicateArcData
    {
        std::string extArcID;
        cost_t      cost;
    };
    /**
    * \brief Data type for storing tuple <extArcID,cost,capacity>
    **/
    struct ArcData
    {
        extarcid_t  extArcID;
        cost_t      cost;
        capacity_t  capacity;
    };
    struct ArcData2
    {
        uint32_t    extArcID;
        cost_t      cost;
        capacity_t  capacity;
    };
    /**
    * \brief Data type for storing tuple <oldArcID,cost,capacity,flow>
    **/
    struct ArcDataAndFlow
    {
        std::string oldArcID;
        cost_t      cost;
        capacity_t  capacity;
        flow_t      flow;
    };
    /**
    * \brief Data type for storing tuple <fromNode,toNode,flow,cost>
    **/
    struct FlowCost
    {
        arc_t        intArc;
        capacity_t   flow;
        cost_t       cost;
    };

    ///\brief Data type for storing tuple <CompressedPath,cost>
    /*typedef std::pair<std::vector<uint32_t>,double> CompressedPath;*/
    typedef std::pair<std::vector<netxpert::data::arc_t>, netxpert::data::cost_t> CompressedPath;

    ///\brief Data type for storing tuple <CompressedPath,flow>
    struct DistributionArc
    {
        CompressedPath  path;
        flow_t          flow;
    };

    ///\brief Data type for storing tuple <extArcID,extArc,cost,flow>
    struct ExtDistributionArc
    {
        netxpert::data::ExtArcID    arcid;
        netxpert::data::ExternalArc extArc;
        cost_t                      cost;
        flow_t                      flow;

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

    ///\brief Data type for storing the result of the Transpotation Solver in JSON-Format.
    struct TransportationResult
    {
        cost_t optimum;
        std::vector<netxpert::data::ExtDistributionArc> dist;

        template<class Archive>
        void serialize( Archive & ar )
        {
            ar( cereal::make_nvp("optimum", optimum),
                cereal::make_nvp("distribution", dist)   );
        }
    };

    ///\brief Data type for storing the result of the MST Solver in JSON-Format.
    struct MSTResult
    {
        cost_t optimum;
        std::vector<netxpert::data::ExternalArc> mst;

        template<class Archive>
        void serialize( Archive & ar )
        {
            ar( cereal::make_nvp("optimum", optimum),
                cereal::make_nvp("mst", mst)   );
        }
    };

    ///\brief Data type for storing the result of the SPT Solver in JSON-Format.
    struct SPTResult
    {
        cost_t optimum;
        std::vector<netxpert::data::ExtSPTreeArc> spt;

        template<class Archive>
        void serialize( Archive & ar )
        {
            ar( cereal::make_nvp("optimum", optimum),
                cereal::make_nvp("spt", spt)   );
        }
    };

    ///\brief Data type for storing tuple <extNodeID,coord>
    struct AddedPoint
    {
        std::string             extNodeID;
        geos::geom::Coordinate  coord;
    };
    ///\brief Data type for storing tuple <extNodeID,supply>
    struct NodeSupply
    {
        std::string extNodeID;
        supply_t    supply;
    };

    ///\brief Data type for storing new nodes data <extNodeID,coord,supply>
    struct NewNode
    {
        std::string             extNodeID;
        geos::geom::Coordinate  coord;
        supply_t                supply;
    };

//    #include "dbhelper.h"
//    using netxpert::io::DBHELPER::GEO_FACTORY;

    ///\brief Data type for storing tuple <arcGeom,nodeType,cost,capacity>
    struct NewArc
    {
        NewArc(geos::geom::LineString& _arcGeom, netxpert::data::AddedNodeType _nodeType, cost_t _cost, capacity_t _capacity ) {
            arcGeom     = std::shared_ptr<geos::geom::LineString>(dynamic_cast<geos::geom::LineString*>( _arcGeom.clone()) );
            nodeType    = _nodeType;
            cost        = _cost;
            capacity    = _capacity;
        }

        std::shared_ptr<geos::geom::LineString> arcGeom;
        //geos::geom::LineString& arcGeom = ( *gf->createEmptyGeometry() );
        //LineString* arcGeom;
        netxpert::data::AddedNodeType           nodeType;
        cost_t                                  cost;
        capacity_t                              capacity;
    };

    struct SwappedOldArc
    {
        netxpert::data::InternalArc ftNode;
        cost_t                      cost;
        capacity_t                  capacity;
    };

    ///\brief Data type for storing tuple <fromNode,toNode,arcGeom,cost,capacity>
    struct SplittedArc
    {
        netxpert::data::InternalArc                  ftNode;
        cost_t                                       cost;
        capacity_t                                   capacity;
        std::shared_ptr<geos::geom::MultiLineString> arcGeom;
    };

    ///\brief Data type for storing tuple <extArcID,extFromNode,extToNode,cost,capacity,oneway>
    struct InputArc
    {
        std::string extArcID;
        std::string extFromNode;
        std::string extToNode;
        cost_t      cost;
        capacity_t  capacity;
        std::string oneway;
    };

    ///\brief Data type for storing tuple <extNodeID,nodeSupply>
    struct InputNode
    {
        std::string extNodeID;
        supply_t    nodeSupply;
    };

    ///\brief Stores the column names of the input data
    struct ColumnMap
    {
        std::string arcIDColName;   //!< ID column name of the arcs
        std::string fromColName;    //!< From node column Name of the arcs
        std::string toColName;      //!< To node column Name of the arcs
        std::string costColName;    //!< Cost column Name of the arcs
        std::string capColName;     //!< Capacity node column Name of the arcs
        std::string onewayColName;  //!< Oneway column Name of the arcs
        std::string nodeIDColName;  //!< ID column name of the nodes
        std::string supplyColName;  //!< Supply column name of the nodes
    };

    typedef uint32_t IntNodeID;

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
        cost_t                   cost;
        capacity_t               capacity;
        std::string              oneway;
		//Bug in VS2013: can't move for default copy
		//https://connect.microsoft.com/VisualStudio/feedback/details/858243/c-cli-compiler-error-trying-to-std-move-a-std-unique-ptr-to-parameter-taken-by-value
		//--> std::unique_ptr funktioniert nicht als data member
		std::shared_ptr<geos::geom::Geometry> geom;
    };
    typedef std::vector<netxpert::data::NetworkBuilderInputArc> NetworkBuilderInputArcs;

    struct NetworkBuilderResultArc
    {
        netxpert::data::ExtArcID    extArcID;
        netxpert::data::IntNodeID   fromNode;
        netxpert::data::IntNodeID   toNode;
        cost_t                      cost;
        capacity_t                  capacity;
        std::string                 oneway;
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
    typedef std::unordered_map< uint32_t, netxpert::data::NetworkBuilderResultArc> NetworkBuilderResultArcs;

    typedef std::unordered_map< std::unique_ptr<geos::geom::Point>, netxpert::data::IntNodeID> NetworkBuilderResultNodes;

 } //namespace data
} //namespace netxpert

namespace std
{

    /**
    * \class Extension for custom key type InternalArc specifying how to hash; serves as key in
    * unordered map.
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
    * \class equal_to operator for custom key type InternalArc in unordered map.
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
    * \class Extension for custom key type ODPair specifying how to hash; serves as key in
    * unordered map.
    **/
    /*template <>
    class hash<netxpert::data::ODPair>
    {
      public:
        long operator()(const netxpert::data::ODPair& x) const
        {
            hash<std::string> z;
            return z(x.origin) ^ z(x.dest);
        }
    };*/

    /**
    * \class equal_to operator for custom key type ODPair in unordered map.
    **/
    /*template <>
    class equal_to<netxpert::data::ODPair>
    {
      public:
         bool operator()(const netxpert::data::ODPair& a, const netxpert::data::ODPair& b) const
         {
            return (a.origin == b.origin) && (a.dest == b.dest);
         }
    };*/
} //namespace std
#endif // DATA_H
