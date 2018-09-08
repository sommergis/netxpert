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

#include "lemon-net.hpp"

using namespace netxpert;
using namespace netxpert::data;
using namespace netxpert::utils;
using namespace netxpert::io;

 InternalNet::InternalNet(const netxpert::data::InputArcs& arcsTbl,
                          const netxpert::data::ColumnMap& _map,
                          const netxpert::cnfg::Config& cnfg,
                          const netxpert::data::InputNodes& nodesTbl,
                          const bool autoClean,
                          const std::map<std::string, netxpert::data::IntNodeID>& extIntNodeMap) {

    try {
        //populate lemon maps
        this->g = std::unique_ptr<graph_t> (new graph_t());
        /*this->extArcIDMap = std::unique_ptr<graph_t::ArcMap<uint32_t>> (
                    new graph_t::ArcMap<uint32_t>(*g) );*/
        this->extArcIDMap = std::unique_ptr<graph_t::ArcMap<extarcid_t>> (
                    new graph_t::ArcMap<extarcid_t>(*g) );
        this->costMap = std::unique_ptr<graph_t::ArcMap<cost_t>> (
                    new graph_t::ArcMap<cost_t>(*g) );
        this->capMap = std::unique_ptr<graph_t::ArcMap<cost_t>> (
                    new graph_t::ArcMap<cost_t>(*g) );
        this->nodeMap = std::unique_ptr<graph_t::NodeMap<std::string>> (
                    new graph_t::NodeMap<std::string>(*g) );
        this->nodeSupplyMap = std::unique_ptr<graph_t::NodeMap<supply_t>> (
                    new graph_t::NodeMap<supply_t>(*g) );
        this->arcChangesMap = std::unique_ptr<graph_t::ArcMap<ArcState>> (
                    new graph_t::ArcMap<ArcState>(*g, ArcState::original) );
        this->arcFilterMap = std::unique_ptr<graph_t::ArcMap<bool>> (
                    new graph_t::ArcMap<bool> (*g, true) );

//        this->arcDataMap = std::unique_ptr<netxpert::data::ArcDataMap<netxpert::data::graph_t,
//                                           netxpert::data::arc_t,
//                                           netxpert::data::ArcData2> > (
//                    new netxpert::data::ArcDataMap<netxpert::data::graph_t,
//                                        netxpert::data::arc_t,
//                                        netxpert::data::ArcData2> (*g) );

        /*this->splittedArcsMap = std::unique_ptr<lemon::SmartDigraph::ArcMap<bool>> (
                    new lemon::SmartDigraph::ArcMap<bool>(*g, false));*

        /*this->sourceMap = std::unique_ptr<lemon::SourceMap<lemon::SmartDigraph> > (
                    new lemon::SourceMap<lemon::SmartDigraph>(*g) );
        this->targetMap = std::unique_ptr<lemon::TargetMap<lemon::SmartDigraph> > (
                    new lemon::TargetMap<lemon::SmartDigraph>(*g) );*/

        arcIDColName = _map.arcIDColName;
        fromColName  = _map.fromColName;
        toColName    = _map.toColName;
        costColName  = _map.costColName;

        if (!_map.capColName.empty())
            capColName = _map.capColName;
        else
            capColName = "";
        if (!_map.onewayColName.empty())
            onewayColName = _map.onewayColName;
        else
            onewayColName = "";

        nodeIDColName = _map.nodeIDColName;
        supplyColName = _map.supplyColName;

        NETXPERT_CNFG = cnfg;

        bool isDirected = cnfg.IsDirected;

        readNodes(arcsTbl, nodesTbl, extIntNodeMap);

        processBarriers();

        readNetwork(arcsTbl, autoClean, isDirected);

        //LOGGER::LogDebug("Count of internalArcData: " + std::to_string(this->arcIDMap.size()));
        LOGGER::LogDebug("Count of arcsTbl: " + std::to_string(arcsTbl.size()));
        LOGGER::LogDebug("Count of internalDistinctNodes: " + std::to_string(this->nodeIDMap.size()));
        LOGGER::LogDebug("Count of eliminatedArcs: " + std::to_string(this->eliminatedArcs.size()));
//        LOGGER::LogDebug("Count of nodeSupplies: " + to_string(nodeSupplies.size()));
        LOGGER::LogDebug("Count of nodesTbl: " + std::to_string(nodesTbl.size()));

    }
    catch (std::exception& ex) {
        LOGGER::LogFatal("InternalNet ctor: Error!");
        LOGGER::LogFatal(ex.what());
    }
}

void
 InternalNet::PrintGraph() {
     using namespace lemon;

    for (graph_t::ArcIt it(*this->g); it != INVALID; ++it)
        std::cout << this->g->id(this->g->source(it)) << "->" << this->g->id(this->g->target(it)) << " co: " << (*this->costMap)[it] << " ca: " << (*this->capMap)[it] << std::endl ;

    for (graph_t::NodeIt it(*this->g); it != INVALID; ++it)
        std::cout << this->g->id(it) << " s:" << (*this->nodeSupplyMap)[it] << std::endl ;

//    std::cout << std::endl;
}

void
 InternalNet::ExportToDIMACS(const std::string& path)
{
    /*
    *  p min 4 4
    n 1 3
    n 4 -3
    a 1 2 0 2 3
    a 2 3 0 2 1
    a 3 4 0 2 1
    a 1 4 0 2 2
    */
    std::ofstream outfile;
    outfile.open(path, std::ios::out );

    using namespace lemon;

    outfile << "p min " << this->g->nodeNum() << " " << this->g->arcNum() << std::endl;

    netxpert::data::graph_t::NodeMap<int> nodes(*this->g);
    int i = 1;
    for(netxpert::data::graph_t::NodeIt v(*this->g); v != INVALID; ++v) {
      nodes.set(v, i);
      //only write out sources and sinks - no transshipment nodes
      if (this->GetNodeSupply(v) != 0) {
        outfile << "n " << i << " " << this->GetNodeSupply(v) << std::endl;
      }
      ++i;
    }
    cost_t cost;
    capacity_t cap;

    for(netxpert::data::graph_t::ArcIt e(*this->g); e != INVALID; ++e) {
      // check for active and filtered arcs first
      // and mark them "not traversible" also in DIMACS with high cost (999999) and low capacity (-1)
      if ((*this->arcFilterMap)[e] == false) {
        cost = DOUBLE_INFINITY;
        cap = DOUBLE_NULL;
      }
      else {
        cost = (*this->costMap)[e];
        cap = (*this->capMap)[e];
      }

      outfile << "a " << nodes[this->g->source(e)] << " " << nodes[this->g->target(e)]
        << " 0 " << cap << " " << cost
         << std::endl;
    }

    outfile.close();

}


//--> Region Contraction Hierarchies
#if (defined NETX_ENABLE_CONTRACTION_HIERARCHIES)
void
 InternalNet::ComputeContraction(float contractionPercent) {

  using namespace netxpert::data;

  this->hasContractionHierarchies = false;

  this->chg           = std::unique_ptr<graph_ch_t>( new graph_ch_t() );
  //copy graph from smart graph to listgraph
  this->chCostMap     = std::unique_ptr<graph_ch_t::ArcMap<cost_t>> ( new graph_ch_t::ArcMap<cost_t> (*this->chg));
  this->chNodeRefMap  = std::unique_ptr<graph_t::NodeMap<graph_ch_t::Node>> (new graph_t::NodeMap<graph_ch_t::Node> (*this->g));
//  this->chArcRefMap   = std::unique_ptr<graph_ch_t::ArcMap<graph_t::Arc>> (new graph_ch_t::ArcMap<graph_t::Arc> (*this->chg));
  this->chNodeCrossRefMap = std::unique_ptr<graph_ch_t::NodeMap<graph_t::Node>> (new graph_ch_t::NodeMap<graph_t::Node> (*this->chg));

  DigraphCopy<graph_t, graph_ch_t> cg(*this->g, *this->chg);
      cg.nodeRef(*this->chNodeRefMap);
      cg.arcMap(*this->costMap, *this->chCostMap);
//      cg.arcCrossRef(*this->chArcRefMap);
      cg.nodeCrossRef(*this->chNodeCrossRefMap);
      cg.run();

  //create CH Interface
  this->chManager = std::unique_ptr<CHInterface<DefaultPriority>> (new CHInterface<DefaultPriority> (*this->chg, *this->chCostMap));

  //contract nodes
  this->chManager->createCH(contractionPercent);

  this->hasContractionHierarchies = true;
}

void
 InternalNet::fillNodeIDMap() {

  using namespace netxpert::data;
  //Fill id map
  graph_t::NodeIt iter(*this->g);
  for (; iter != INVALID; ++iter) {
    auto chNode     = (*this->chNodeRefMap)[iter];
    auto origNodeID = (*this->nodeMap)[iter];
    this->chNodeIDRefMap.insert( std::make_pair(origNodeID, this->chg->id(chNode) ));
  }
}

void
 InternalNet::ExportContractedNetwork(const std::string graphName){

  std::ofstream gs  (graphName + ".gr", std::ofstream::out);
  std::ofstream nos (graphName + "-nodeorder.gr", std::ofstream::out);
  std::ofstream nms (graphName + "-nodemap.gr", std::ofstream::out);

  fillNodeIDMap();
  //export contracted network
  this->chManager->exportDimacs(gs, nos, nms, this->chNodeIDRefMap);

  /*LOGGER::LogDebug("exporting arc ref map..");
  //export arc map
  std::ofstream ams (graphName + "-arcmap.gr", std::ofstream::out);

	ams << "c CH arc ref map " << std::endl;
	ams << "c Orig arc(from,to) CH arc(from,to) " << std::endl;

	size_t s = lemon::countArcs(*this->chg);

	ams << "p " <<  s  << std::endl;

	int origArcID, chArcID = -1;

	for (graph_ch_t::ArcIt e(*this->chg); e != INVALID; ++e) {
    auto origArc = (*this->chArcRefMap)[e];

    if (origArc == INVALID || !this->g->valid(origArc))
      origArcID = -1;
    else
      origArcID = this->g->id(origArc);

    chArcID  = this->chg->id(e) +1;

		ams << "a " << origArcID << " "  << chArcID << std::endl;
	}*/

}

void
 InternalNet::ImportContractedNetwork(const std::string graphName){

  LOGGER::LogDebug("Importing Contraction Hierarchies of "+ graphName + "..");
  using namespace netxpert::data;

  this->hasContractionHierarchies = false;

  this->chg       = std::unique_ptr<graph_ch_t> (new graph_ch_t());
  this->chCostMap = std::unique_ptr<graph_ch_t::ArcMap<cost_t>> (new graph_ch_t::ArcMap<cost_t>(*this->chg));
  this->chManager = std::unique_ptr<CHInterface<DefaultPriority>> (new CHInterface<DefaultPriority> (*this->chg, *this->chCostMap));
  //TEST
  this->chNodeCrossRefMap = std::unique_ptr<graph_ch_t::NodeMap<graph_t::Node>> (new graph_ch_t::NodeMap<graph_t::Node> ((*this->chg), INVALID));

  // chNodeRefMap<origNode,chNode>
  this->chNodeIDRefMap = this->chManager->importDimacs(graphName);

  for (const auto& kv : this->chNodeIDRefMap) {
    auto chNode = this->chg->nodeFromId(kv.second);
    auto node = this->GetNodeFromOrigID(kv.first);

    (*this->chNodeCrossRefMap)[chNode] = node;
  }

  /*LOGGER::LogDebug("importing arc ref map..");
  //import arc map
  std::ifstream is (graphName + "-arcmap.gr", std::ifstream::in);

  char c;
  int origArcID, chArcID = -1;
  std::string str;

  while (is >> c) {
    if (c == 'c') {
      getline(is, str);
      continue;
    }
    if (c == 'a') {
      is >> origArcID >> chArcID;
      getline(is, str);
//      std::cout << origArcID << " " << chArcID << std::endl;
//      std::cout << this->chg->valid(this->chg->arcFromId(chArcID-1)) << std::endl;
//      std::cout << this->g->valid(this->g->arcFromId(origArcID-1)) << std::endl;
      if (origArcID > 0 && chArcID > 0)
        (*this->chArcRefMap)[this->chg->arcFromId(chArcID-1)] = this->g->arcFromId(origArcID-1);
      continue;
    }
  }*/

  fillNodeMap();

  this->hasContractionHierarchies = true;

  LOGGER::LogDebug("CH - countNodes of graph: " + to_string( lemon::countNodes(*this->chg) ));
  LOGGER::LogDebug("CH - countArcs of graph: " + to_string( lemon::countArcs(*this->chg)  ));

  LOGGER::LogDebug("Done!");
}

void
 InternalNet::fillNodeMap() {

  using namespace netxpert::data;
  this->chNodeRefMap = std::unique_ptr<graph_t::NodeMap<graph_ch_t::Node>>(new graph_t::NodeMap<graph_ch_t::Node> (*this->g));
  //populate chNodeRefMap
  for(auto& kv: this->chNodeIDRefMap) {
    auto origNode = this->nodeIDMap.at(kv.first);
    auto chNode   = this->chg->nodeFromId(kv.second);
    (*this->chNodeRefMap)[origNode] = chNode;
  }
}
//|--> Endregion Contraction Hierarchies
#endif

const arc_t
 InternalNet::GetArcFromID(const netxpert::data::intarcid_t arcID) {
    //auto arc = this->arcIDMap.at(arcID);
    auto arc = this->g->arcFromId(arcID);
    return arc;
}

const arc_t
 InternalNet::GetArcFromOrigID(const netxpert::data::extarcid_t arcID) {
    //Ŝauto arc = (*this->extArcIDMap)[arcID];
    auto arc = lemon::mapFind(*this->g, (*this->extArcIDMap), arcID);
    return arc;
}

const netxpert::data::intarcid_t
 InternalNet::GetArcID(const arc_t& arc) {
    //auto result = (*this->extArcIDMap)[arc];
    auto result = this->g->id(arc);
    return result;
}


const cost_t
 InternalNet::GetArcCapacity(const arc_t& arc) {
    auto result = (*this->capMap)[arc];
    //auto result = ((*this->arcDataMap)[arc]).cost;
    return result;
}


const cost_t
 InternalNet::GetArcCost(const arc_t& arc) {
    auto result = (*this->costMap)[arc];
    //auto result = ((*this->arcDataMap)[arc]).capacity;
    return result;
}


const netxpert::data::ArcData
//netxpert::data::ArcData2
 InternalNet::GetArcData(const arc_t& arc) {
    netxpert::data::ArcData result;
//    netxpert::data::ArcData2 result;
    result.extArcID     = (*this->extArcIDMap)[arc];
    result.cost         = (*this->costMap)[arc];
    result.capacity     = (*this->capMap)[arc];
//    result = (*this->arcDataMap)[arc];

    return result;
}

void
 InternalNet::SetArcData(const arc_t& arc, const ArcData& arcData) {

    (*this->extArcIDMap)[arc]   = arcData.extArcID; //can be 'dummy' in case of MCF
    (*this->costMap)[arc]       = arcData.cost;
    (*this->capMap)[arc]        = arcData.capacity;
}

void
 InternalNet::RegisterNodeID(const std::string& nodeID, const netxpert::data::node_t& node) {

    this->nodeIDMap.insert( std::make_pair(nodeID, node) );
    (*this->nodeMap)[node] = nodeID;
}

const std::unordered_set<std::string>
 InternalNet::GetOrigArcIDs(const std::vector<netxpert::data::arc_t>& path){

    using namespace std;
    using namespace lemon;

    unordered_set<string> resultIDs;

    for (auto arc : path) {
        auto extArcID = (*this->extArcIDMap)[arc];
        if (extArcID.size() > 0)
            resultIDs.insert(extArcID);
    }

    return resultIDs;
}


const netxpert::data::arc_t
 InternalNet::GetArcFromNodes(const netxpert::data::node_t& source,
                              const netxpert::data::node_t& target) {
    using namespace lemon;
    return lemon::findArc(*this->g, source, target, INVALID);
}


const node_t
 InternalNet::GetNodeFromOrigID(const std::string nodeID) {

//    LOGGER::LogDebug("entering GetNodeFromOrigID()..");

    netxpert::data::node_t node = lemon::INVALID;

    try {
        node = this->nodeIDMap.at(nodeID);
    }
    catch (std::exception& ex) {
        try {
            this->nodeIDMap.at(nodeID + "@netXpert");
        }
        catch (std::exception& ex) {
            LOGGER::LogWarning("Could not get node from orig node id: "+ nodeID);

            std::cout << "Nodes are: " << std::endl << "ext : int" << std::endl;
            for (auto& kv : this->nodeIDMap) {
                std::cout << kv.first << " : " << this->GetNodeID(kv.second) << std::endl;
            }
        }
    }

    return node;
}

const uint32_t
 InternalNet::GetNodeID(const netxpert::data::node_t& node){
    return this->g->id(node);
}

const netxpert::data::node_t
 InternalNet::GetNodeFromID(const uint32_t nodeID) {
    return this->g->nodeFromId(nodeID);
}
const netxpert::data::node_t
 InternalNet::GetSourceNode(const netxpert::data::arc_t& arc) {
    if (this->g->valid(arc))
      return this->g->source(arc);
    else
      return lemon::INVALID;
}

const netxpert::data::node_t
 InternalNet::GetTargetNode(const netxpert::data::arc_t& arc) {
    if (this->g->valid(arc))
      return this->g->target(arc);
    else
      return lemon::INVALID;
}


const std::string
 InternalNet::GetOrigNodeID(const node_t& node) {

//    LOGGER::LogDebug("entering GetOrigNodeID()");
    std::string nodeID;

    nodeID = (*this->nodeMap)[node];
//    std::cout <<"1. Orig node id from nodeMap is: " << nodeID << std::endl;

    if (nodeID.size() > 0) {
        if ( this->addedStartPoints.count(node) != 0 ) {
            auto a = this->addedStartPoints.at(node);
            nodeID = a.extNodeID;
//            std::cout <<"2. Orig node id from nodeMap is: " << nodeID << std::endl;
        }
        else {
            if (this->addedEndPoints.count(node) != 0) {
                auto a = this->addedEndPoints.at(node);
                nodeID = a.extNodeID;
//                std::cout <<"3. Orig node id from nodeMap is: " << nodeID << std::endl;
            }
        }
    }
    else {
        throw std::runtime_error("Orig node id of internal node id " + std::to_string( this->GetNodeID(node) ) + " could not be looked up!");
    }

    return nodeID;
}


const netxpert::data::supply_t
 InternalNet::GetNodeSupply(const node_t& node) {

    auto nodeSupply = (*this->nodeSupplyMap)[node];
    return nodeSupply;
}

const netxpert::data::graph_t::NodeIt
 InternalNet::GetNodesIter() {
    netxpert::data::graph_t::NodeIt iter(*this->g);
    return iter;
}

const netxpert::data::graph_t::ArcIt
 InternalNet::GetArcsIter() {
    netxpert::data::graph_t::ArcIt iter(*this->g);
    return iter;
}

const geos::geom::Coordinate
 InternalNet::GetStartOrEndNodeGeometry(const netxpert::data::ExtNodeID nodeID) {

    geos::geom::Coordinate result;

    auto node = this->GetNodeFromOrigID(nodeID);
//    std::cout << "GetStartOrEndNodeGeometry()" << std::endl;
//    std::cout << "ext node id: " << nodeID << " int node id: " << this->g->id(node) << std::endl;
//    std::cout << "point " << addedStartPoints.at(node).coord.toString() << std::endl;
    try
    {
        if ( this->addedStartPoints.count(node) != 0 ) {
            auto externalNode = this->addedStartPoints.at(node);
            result = externalNode.coord;
//            std::cout << result.toString() << std::endl;
        }
        else {
            if (this->addedEndPoints.count(node) != 0) {
                auto externalNode = this->addedEndPoints.at(node);
                result = externalNode.coord;
//                std::cout << result.toString() << std::endl;
            }
        }

    }
    catch (std::exception& ex)
    {
        LOGGER::LogWarning("Original start/end node " +nodeID +" could not be looked up!");
        throw ex;
    }
    return result;
}

const uint32_t
 InternalNet::GetNodeCount() {
    return static_cast<uint32_t>(lemon::countNodes(*this->g));
}

const uint32_t
 InternalNet::GetArcCount() {
    return static_cast<uint32_t>(lemon::countArcs(*this->g));
}

//Vorgehen:
//1. Hol die nächste Kante (per Spatialite) zum Knoten newNode: ArcID + Geometrie
//2. Wurde die Kante schon aufgebrochen oder nicht?
//3. Hole Geometrie neu wenn nötig (aus den newArcs)
//4. isPointOnLine, position of closestPoint (Start / End ) alles in Memory (egal ob aufgebrochene Kante oder vorhandene Kante)
//5. Splitte Kante
const uint32_t
 InternalNet::AddNode(const netxpert::data::NewNode& newNode, const int threshold,
                      SQLite::Statement& closestArcQry, const bool withCapacity,
                      const AddedNodeType startOrEnd) {

    #ifdef DEBUG
    std::cout << "entering AddNode().. "<< std::endl;
    #endif
    using namespace std;
    using namespace geos::geom;

    const string extNodeID      = newNode.extNodeID;
    const Coordinate point      = newNode.coord;
    const auto nodeSupply       = newNode.supply;

    //1. Suche die nächste Line und den nächsten Punkt auf der Linie zum temporaeren Punkt
    ExtClosestArcAndPoint closestArcAndPoint = DBHELPER::GetClosestArcFromPoint(point,
            threshold, closestArcQry, withCapacity);

    // kann noch auf leeren string gesetzt werden, wenn Kante bereits aufgebrochen wurde
    string extArcID          = closestArcAndPoint.extArcID;
    Geometry& closestArc     = *closestArcAndPoint.arcGeom;
    const string extFromNode = closestArcAndPoint.extFromNode;
    const string extToNode   = closestArcAndPoint.extToNode;
    const cost_t cost        = closestArcAndPoint.cost;
    const cost_t capacity    = closestArcAndPoint.capacity;

    //2. Wenn die originale Kante bereits aufgebrochen wurde hol die nächste Linie zum Punkt aus den aufgebrochenen Kanten
//    auto origArc = GetArcFromOrigID(std::stoul(extArcID));
    auto origArc = GetArcFromOrigID(extArcID);
    //bool isArcSplitAlready = (*this->splittedArcsMap)[origArc];
    bool isArcUnchanged = ( (*this->arcChangesMap)[origArc] == ArcState::original ) ? true : false;

    graph_t::Node resultNode = lemon::INVALID;

    if ( isArcUnchanged ) {
        // Prüfe, ob der nächste Punkt gleichzeitig der Start - oder der Endpunkt einer Linie ist
        auto locationOfPoint = getLocationOfPointOnLine(point, closestArc);

        switch ( locationOfPoint  )
        {
            case StartOrEndLocationOfLine::Start: {
                LOGGER::LogDebug("Closest Point for start node "+extNodeID
                    +" is identical to a start node of the network!");
                //fromNode und toNode von oben nehmen und keinen Split durchführen
//                std::cout << "extFromNode " << extFromNode << " | " << " extNodeID " << extNodeID<< std::endl;
                if (this->nodeIDMap.count(extFromNode) > 0)
                {
                    resultNode = this->GetNodeFromOrigID(extFromNode);
                    //save closestPoint geom for lookup later on straight lines for ODMatrix
                    // no insertion of new node necessary (as with insertNewNode()
                    AddedPoint pVal  {extNodeID, point};

                    switch ( startOrEnd )
                    {
                        case AddedNodeType::StartArc:
                            addedStartPoints.insert(make_pair(resultNode, pVal));
                            break;
                        case AddedNodeType::EndArc:
                            addedEndPoints.insert(make_pair(resultNode, pVal));
                            break;
                    }

                    //add nodeSupply
                    (*this->nodeSupplyMap)[resultNode] = nodeSupply;
                    //set extNodeID really?
                    this->RegisterNodeID(extNodeID, resultNode);
                }
                else
                    std::cout << "Node From ID " << extFromNode << " not found in nodeIDMap!"<< std::endl;
                break;
            }
            case StartOrEndLocationOfLine::End: {
                LOGGER::LogDebug("Closest Point for start node "+extNodeID
                    +" is identical to a end node of the network!");
                //fromNode und toNode von oben nehmen und keinen Split durchführen
//                std::cout << "extToNode " << extToNode << " | " << " extNodeID " << extNodeID<< std::endl;
                if (this->nodeIDMap.count(extToNode) > 0)
                {
                    resultNode = this->GetNodeFromOrigID(extToNode);
                    //save closestPoint geom for lookup later on straight lines for ODMatrix
                    // no insertion of new node necessary (as with insertNewNode()
                    AddedPoint pVal  {extNodeID, point};

                    switch ( startOrEnd )
                    {
                        case AddedNodeType::StartArc:
                            addedStartPoints.insert(make_pair(resultNode, pVal));
                            break;
                        case AddedNodeType::EndArc:
                            addedEndPoints.insert(make_pair(resultNode, pVal));
                            break;
//                        std::cout << "point " << point.toString() << std::endl;
                    }

                    //add nodeSupply
                    (*this->nodeSupplyMap)[resultNode] = nodeSupply;
                    //set extNodeID really?
                    this->RegisterNodeID(extNodeID, resultNode);
                }
//                else
//                    std::cout << "Node To ID " << extToNode << " not found in nodeIDMap!"<< std::endl;
                break;
            }
            case StartOrEndLocationOfLine::Intermediate: {
                LOGGER::LogDebug("Closest point lies somewhere between start and end coordinate of the line..");
                // --> split original line

                //unused
                ExternalArc ftPair  {extFromNode, extToNode};
//                ArcData val  {std::stoul(extArcID), cost, capacity};
                ArcData val  {extArcID, cost, capacity};
                pair<ExternalArc,ArcData> arcData = make_pair(ftPair, val);

                auto splittedLine = getSplittedClosestOrigArcToPoint(point, threshold, arcData, closestArc);

                //insertNewNode will be called always in undirected mode
                resultNode = insertNewNode(true, splittedLine, extArcID, extNodeID, point, startOrEnd);

                //add nodeSupply
//                std::cout << "adding node supply of "<< this->g->id(resultNode) << ": "<<nodeSupply << std::endl;
                (*this->nodeSupplyMap)[resultNode] = nodeSupply;
//                std::cout << "extNodeID: " << extNodeID << std::endl;
//                this->nodeIDMap.insert( make_pair(extNodeID, resultNode)  );
                this->RegisterNodeID(extNodeID, resultNode);

                //arc changes
                (*this->arcChangesMap)[splittedLine.arc] = ArcState::originalAndSplit;

                break;
            }
        }
        //future: return id of closest toNode (if start), fromNode (if end)
        return this->g->id(resultNode);
    }
    else {
        LOGGER::LogDebug("searching in newly added arcs..");
        //search in newly added arcs
        auto splittedLine = getSplittedClosestNewArcToPoint(point, threshold);

        //insertNewNode will be called always in undirected mode!
        resultNode = insertNewNode(true, splittedLine, "", extNodeID, point, startOrEnd);

        //add nodeSupply
        (*this->nodeSupplyMap)[resultNode] = nodeSupply;
//        this->nodeIDMap.insert( make_pair(extNodeID, resultNode)  );
        this->RegisterNodeID(extNodeID, resultNode);

        //arc changes
        (*this->arcChangesMap)[splittedLine.arc] = ArcState::addedAndSplit;

        return this->g->id(resultNode);
    }
}

const node_t
 InternalNet::insertNewNode(bool isDirected,
                                 netxpert::data::IntNetSplittedArc<arc_t>& splittedLine,
                                 const std::string& extArcID,
                                 const std::string& extNodeID,
                                 const geos::geom::Coordinate& point,
                                 const AddedNodeType startOrEnd) {

    //std::cout << "entering insertNewNode().. "<< std::endl;

    using namespace std;
    using namespace geos::geom;

//    cout << "insertNewNode() .." << endl;
//    cout << "insert node on extarcid " << extArcID << endl;
    auto origArc = splittedLine.arc;

    //Get relative cost to geometry length
    auto segments = splittedLine.segments;
    auto ptr1 = segments.first;
    auto ptr2 = segments.second;
    const Geometry& g1 = *ptr1;
    const Geometry& g2 = *ptr2;
    const LineString& segment1 = dynamic_cast<const LineString&>(g1);
    const LineString& segment2 = dynamic_cast<const LineString&>(g2);

    const MultiLineString& completeLine = *(DBHELPER::GEO_FACTORY->createMultiLineString(
                                          vector<Geometry*>{ ptr1.get(), ptr2.get() } ));

    auto newArc1Cost = getRelativeValueFromGeomLength<netxpert::data::cost_t>(splittedLine.cost, completeLine, segment1);
    auto newArc2Cost = getRelativeValueFromGeomLength<netxpert::data::cost_t>(splittedLine.cost, completeLine, segment2);

    //Capacity is not relative!

    auto newNode = this->g->addNode();

    auto origFromNode   = this->g->source(origArc);
    auto origToNode     = this->g->target(origArc);

    //detect if arc is reverse also in the graph
    //TODO Check for performance on large graphs
    //TODO Check for duplicate arcs!
    auto revOrigArc = GetArcFromNodes(this->g->target(origArc),this->g->source(origArc));
    if (revOrigArc != lemon::INVALID)
        isDirected = false;
    else
        isDirected = true;

    if (isDirected) {

        //Save newEdges and set Geometry
        LineString* ptr1 = dynamic_cast<LineString*>(segment1.clone());
        LineString* ptr2 = dynamic_cast<LineString*>(segment2.clone());

        //add new arcs
        auto newArc1 = this->g->addArc(origFromNode, newNode);
        auto newArc2 = this->g->addArc(newNode, origToNode);

        NewArc n1 { *ptr1 , startOrEnd, newArc1Cost, splittedLine.capacity};
        NewArc n2 { *ptr2 , startOrEnd, newArc2Cost, splittedLine.capacity};

        //store new arcs
        this->newArcsMap.insert( make_pair(newArc1, n1) );
        this->newArcsMap.insert( make_pair(newArc2, n2) );

        //set in cost map
        (*this->costMap)[newArc1] = newArc1Cost;
        (*this->costMap)[newArc2] = newArc2Cost;

        //cap map - not relative to segment length!
        (*this->capMap)[newArc1] = splittedLine.capacity;
        (*this->capMap)[newArc2] = splittedLine.capacity;

        //disable orig Arc in arcFilterMap
        //-->Remove orig arc from internal graph
        (*this->arcFilterMap)[origArc] = false;

        //enable the new arcs
        (*this->arcFilterMap)[newArc1] = true;
        (*this->arcFilterMap)[newArc2] = true;

#if (defined NETX_ENABLE_CONTRACTION_HIERARCHIES)
        if (this->hasContractionHierarchies) {

          std::cout << "directed - hasContractionHierarchies.. "<< std::endl;
          //add changes also to CH graph & maps
          auto newNodeCH      = this->chg->addNode();

          //add new regular graph node to refMap
          (*this->chNodeRefMap)[newNode] = newNodeCH;
          (*this->chNodeCrossRefMap)[newNodeCH] = newNode;

          //get the correct nodes from the refNodesMap of the contracted graph
          auto origFromNodeCH = (*this->chNodeRefMap)[origFromNode];
          auto origToNodeCH   = (*this->chNodeRefMap)[origToNode];

          //add new arcs to CH graph also
          auto newArc1CH = this->chg->addArc(origFromNodeCH, newNodeCH);
          auto newArc2CH = this->chg->addArc(newNodeCH, origToNodeCH);

          //add arcs also to chArcRefMap for lookup on CH paths
//          (*this->chArcRefMap)[newArc1CH] = newArc1;
//          (*this->chArcRefMap)[newArc2CH] = newArc2;

          //no saving of new arcs in this->newArcsMap

          (*this->chCostMap)[newArc1CH] = newArc1Cost;
          (*this->chCostMap)[newArc2CH] = newArc2Cost;

          //no cap maps - spt respects no caps

          //find ch arc to remove
          auto chArc = lemon::findArc(*this->chg, origFromNodeCH, origToNodeCH);

          //insert the new node into CH structures
          auto r1 = INVALID;
          auto r2 = INVALID;
          auto r3 = INVALID;
          this->chManager->addNodeToCH(newNodeCH, newArc1CH, newArc2CH, newArc1Cost, newArc2Cost, chArc, r1, r2, r3);
        }
#endif
    }
    else {

        //Save newEdges and set Geometry
        LineString* ptr1 = dynamic_cast<LineString*>(segment1.clone());
        LineString* ptr2 = dynamic_cast<LineString*>(segment2.clone());

        //would be more logical
        /*auto newArc1 = this->g->addArc(origFromNode, newNode, );
        auto newArc2 = this->g->addArc(newNode, origToNode);
        auto revNewArc1 = this->g->addArc(newNode, origFromNode );
        auto revNewArc2 = this->g->addArc(origToNode, newNode);*/

        //but is: sic! swap from/toNode in undirected case
        auto newArc1 = this->g->addArc(newNode, origToNode);
//        std::cout << "newArc1 "<<this->g->id(newArc1)<< ", " << g->id(g->source(newArc1)) << "->" << g->id(g->target(newArc1)) <<
//                   //", orig: " << GetOrigNodeID(g->source(newArc1)) << "->" << GetOrigNodeID(g->target(newArc1))<<
//                   std::endl;
        //std::cout << "seg1 ID# " << this->g->id(newArc1) << std::endl;
        auto revNewArc1 = this->g->addArc(origToNode, newNode );
//        std::cout << "revNewArc1 "<<this->g->id(revNewArc1)<< ", " << g->id(g->source(revNewArc1)) << "->" << g->id(g->target(revNewArc1)) <<
//                   //", orig: " << GetOrigNodeID(g->source(revNewArc1)) << "->" << GetOrigNodeID(g->target(revNewArc1))<<
//                   std::endl;
        //std::cout << "revseg1 ID# " << this->g->id(revNewArc1) << std::endl;
        auto newArc2 = this->g->addArc(origFromNode, newNode);
//        std::cout << "newArc2 "<<this->g->id(newArc2)<< ", " << g->id(g->source(newArc2)) << "->" << g->id(g->target(newArc2)) <<
//                   //", orig: " << GetOrigNodeID(g->source(newArc2)) << "->" << GetOrigNodeID(g->target(newArc2))<<
//                   std::endl;
        //std::cout << "seg2 ID# " << this->g->id(newArc2) << std::endl;
        auto revNewArc2 = this->g->addArc(newNode, origFromNode);
//        std::cout << "revNewArc2 "<<this->g->id(revNewArc2)<< ", " << g->id(g->source(revNewArc2)) << "->" << g->id(g->target(revNewArc2)) <<
//                   //", orig: " << GetOrigNodeID(g->source(revNewArc2)) << "->" << GetOrigNodeID(g->target(revNewArc2))<<
//                   std::endl;
        //std::cout << "revseg2 ID# " << this->g->id(revNewArc2) << std::endl;

        NewArc n1 { *ptr1 , startOrEnd, newArc1Cost, splittedLine.capacity};
//        std::cout << "newArc1: " << n1.arcGeom->toString() << std::endl;
        NewArc n2 { *ptr2 , startOrEnd, newArc2Cost, splittedLine.capacity};
//        std::cout << "newArc2: " << n2.arcGeom->toString() << std::endl;
        NewArc rn1 { *ptr1 , startOrEnd, newArc1Cost, splittedLine.capacity};
//        std::cout << "revNewArc1: " << rn1.arcGeom->toString() << std::endl;
        NewArc rn2 { *ptr2 , startOrEnd, newArc2Cost, splittedLine.capacity};
//        std::cout << "revNewArc2: " << rn2.arcGeom->toString() << std::endl;

        //store new arcs
        this->newArcsMap.insert( make_pair(newArc1, n1) );
        this->newArcsMap.insert( make_pair(revNewArc1, rn1) );
        this->newArcsMap.insert( make_pair(newArc2, n2) );
        this->newArcsMap.insert( make_pair(revNewArc2, rn2) );

        //set in cost map
        (*this->costMap)[newArc1] = newArc1Cost;
        (*this->costMap)[newArc2] = newArc2Cost;
        (*this->costMap)[revNewArc1] = newArc1Cost;
        (*this->costMap)[revNewArc2] = newArc2Cost;

        //cap map - not relative to segment length!
        (*this->capMap)[newArc1] = splittedLine.capacity;
        (*this->capMap)[newArc2] = splittedLine.capacity;
        (*this->capMap)[revNewArc1] = splittedLine.capacity;
        (*this->capMap)[revNewArc2] = splittedLine.capacity;

        //disable orig Arc in arcFilterMap
        //-->Remove orig arc from internal graph
        (*this->arcFilterMap)[origArc] = false;

        //-->Remove reversed orig arc also from internal graphs
        if (revOrigArc != lemon::INVALID)
            (*this->arcFilterMap)[revOrigArc] = false;

        //enable the new arcs
        (*this->arcFilterMap)[newArc1] = true;
        (*this->arcFilterMap)[newArc2] = true;
        (*this->arcFilterMap)[revNewArc1] = true;
        (*this->arcFilterMap)[revNewArc2] = true;

#if (defined NETX_ENABLE_CONTRACTION_HIERARCHIES)
        if (this->hasContractionHierarchies) {
          std::cout << "undirected - hasContractionHierarchies.. "<< std::endl;
          //add changes also to CH graph & maps
          auto newNodeCH      = this->chg->addNode();

          //add new regular graph node to refMap
          (*this->chNodeRefMap)[newNode] = newNodeCH;
          (*this->chNodeCrossRefMap)[newNodeCH] = newNode;

          //get the correct nodes from the refNodesMap of the contracted graph
          auto origFromNodeCH = (*this->chNodeRefMap)[origFromNode];
          auto origToNodeCH   = (*this->chNodeRefMap)[origToNode];

          ///TEST
//          auto testArc1CH = lemon::findArc(*this->chg, newNodeCH, origToNodeCH);
//          if (testArc1CH != INVALID)
//            this->chManager->printShortCuts(testArc1CH);

          //add new arcs to CH graph also
          auto newArc1CH    = this->chg->addArc(newNodeCH, origToNodeCH);
          auto revNewArc1CH = this->chg->addArc(origToNodeCH, newNodeCH);
          auto newArc2CH    = this->chg->addArc(origFromNodeCH, newNodeCH);
          auto revNewArc2CH = this->chg->addArc(newNodeCH, origFromNodeCH);

//          auto newArc1CH    = this->chg->addArc(origToNodeCH, newNodeCH);
//          auto revNewArc1CH = this->chg->addArc(newNodeCH, origToNodeCH);
//          auto newArc2CH    = this->chg->addArc(newNodeCH, origFromNodeCH);
//          auto revNewArc2CH = this->chg->addArc(origFromNodeCH, newNodeCH);

//          std::cout << "newArc1CH" << std::endl;
//          std::cout << this->chg->id(this->chg->source(newArc1CH)) << "->"
//          << this->chg->id(this->chg->target(newArc1CH)) <<std::endl;

          //no saving of new arcs in this->newArcsMap
          (*this->chCostMap)[newArc1CH]     = newArc1Cost;
          (*this->chCostMap)[newArc2CH]     = newArc2Cost;
          (*this->chCostMap)[revNewArc1CH]  = newArc1Cost;
          (*this->chCostMap)[revNewArc2CH]  = newArc2Cost;

          //no cap maps - spt respects no caps

          //find ch arc to remove
          auto chArc = lemon::findArc(*this->chg, origFromNodeCH, origToNodeCH);
          auto revChArc = lemon::findArc(*this->chg, origToNodeCH, origFromNodeCH);

          //insert the new node into CH structures
          this->chManager->addNodeToCH(newNodeCH, newArc1CH, newArc2CH, newArc1Cost, newArc2Cost, chArc, revNewArc1CH, revNewArc2CH, revChArc);

        }
#endif
    }
    switch (startOrEnd)
    {
        case AddedNodeType::StartArc:
            addedStartPoints.insert( make_pair(newNode, AddedPoint {extNodeID, point}) );
            break;
        case AddedNodeType::EndArc:
            addedEndPoints.insert( make_pair(newNode, AddedPoint {extNodeID, point}) );
            break;
    }
    //std::cout << "point " << point.toString() << std::endl;

    return newNode;
}

void
 InternalNet::insertNewBarrierNodes(bool isDirected,
                                    netxpert::data::IntNetSplittedArc2<netxpert::data::arc_t>& clippedLine) {

//    using namespace std;
//    using namespace geos::geom;
//
//    auto origArc = clippedLine.arc;
//
//    //Get relative cost to geometry length
//    //Capacity is not relative!
//    auto segments = clippedLine.segments;
//
//    auto ptr1 = segments.first;
//    auto ptr2 = segments.second;
//    const Geometry& g1 = *ptr1;
//    const Geometry& g2 = *ptr2;
//    const LineString& segment1 = dynamic_cast<const LineString&>(g1);
//    const LineString& segment2 = dynamic_cast<const LineString&>(g2);
//
//    const MultiLineString& completeLine = *(DBHELPER::GEO_FACTORY->createMultiLineString(
//                                          vector<Geometry*>{ ptr1.get(), ptr2.get() } ));
//
//    auto newArc1Cost = getRelativeValueFromGeomLength<netxpert::data::cost_t>(splittedLine.cost, completeLine, segment1);
//    auto newArc2Cost = getRelativeValueFromGeomLength<netxpert::data::cost_t>(splittedLine.cost, completeLine, segment2);
//
//    //count of new nodes: (2*n segments) -2)
//    int newNodesCount = (2*segments.size() ) - 2;
//    vector<netxpert::data::node_t> newNodes;
//    for (int j = 0; j < newNodesCount; j++) {
//        auto newNode = this->g->addNode();
//        newNodes.push_back(newNode);
//    }
//
//    auto origFromNode   = this->g->source(origArc);
//    auto origToNode     = this->g->target(origArc);
//
//    //detect if arc is reverse also in the graph
//    auto revOrigArc = lemon::findArc(*this->g, this->g->target(origArc),this->g->source(origArc), lemon::INVALID);
//    if (revOrigArc != lemon::INVALID)
//        isDirected = false;
//    else
//        isDirected = true;
//
//    if (isDirected) {
//
//        //Save newEdges and set Geometry
//        LineString* ptr1 = dynamic_cast<LineString*>(segment1.clone());
//        LineString* ptr2 = dynamic_cast<LineString*>(segment2.clone());
//
//        //add new arcs
//        auto newArc1 = this->g->addArc(origFromNode, newNode);
//        auto newArc2 = this->g->addArc(newNode, origToNode);
//
//        NewArc n1 { *ptr1 , startOrEnd, newArc1Cost, splittedLine.capacity};
//        NewArc n2 { *ptr2 , startOrEnd, newArc2Cost, splittedLine.capacity};
//
//        //store new arcs
//        this->newArcsMap.insert( make_pair(newArc1, n1) );
//        this->newArcsMap.insert( make_pair(newArc2, n2) );
//
//        //set in cost map
//        (*this->costMap)[newArc1] = newArc1Cost;
//        (*this->costMap)[newArc2] = newArc2Cost;
//
//        //disable orig Arc in arcFilterMap
//        //-->Remove orig arc from internal graph
//        (*this->arcFilterMap)[origArc] = false;
//
//        //enable the new arcs
//        (*this->arcFilterMap)[newArc1] = true;
//        (*this->arcFilterMap)[newArc2] = true;
//    }
//

}

const uint32_t
 InternalNet::AddStartNode(std::string extNodeID,
                            double x, double y, netxpert::data::supply_t supply,
                            int threshold, const ColumnMap& cmap, bool withCapacity) {

    using namespace std;
    using namespace geos::geom;

    try {

      //check extNodeID if already present in nodeIDMap
      if (this->nodeIDMap.count(extNodeID) != 0) {
          LOGGER::LogWarning("External Node ID "+ extNodeID + " already present! Renaming..");
          extNodeID = extNodeID + "@netXpert";
      }

      NewNode n { extNodeID, Coordinate {x, y}, supply};

      const string arcsTableName  = NETXPERT_CNFG.ArcsTableName;
      const string geomColumnName = NETXPERT_CNFG.ArcsGeomColumnName;

      auto qry = DBHELPER::PrepareGetClosestArcQuery(arcsTableName, geomColumnName,
                                              cmap, ArcIDColumnDataType::Number, withCapacity);
      return AddNode(n, threshold, *qry, withCapacity, AddedNodeType::StartArc);
    }
    catch (const std::runtime_error& ex) {
      LOGGER::LogError("RuntimeError in AddStartNode()!");
      LOGGER::LogError(ex.what() );
    }
    catch (const std::exception& ex) {
      LOGGER::LogError("Exception in AddStartNode()!");
      LOGGER::LogError(ex.what() );
    }
    catch (...) {
      LOGGER::LogError("Unknown Error in AddStartNode() - maybe a memory error!");
    }
}

const uint32_t
 InternalNet::AddEndNode(std::string extNodeID,
                          double x, double y, netxpert::data::supply_t supply,
                          int threshold, const ColumnMap& cmap, bool withCapacity) {

    using namespace std;
    using namespace geos::geom;

    try  {
      //check extNodeID if already present in nodeIDMap
      if (this->nodeIDMap.count(extNodeID) != 0) {
          LOGGER::LogWarning("External Node ID "+ extNodeID + " already present! Renaming..");
          extNodeID = extNodeID + "@netXpert";
      }

      NewNode n { extNodeID, Coordinate {x, y}, supply};

      const string arcsTableName  = NETXPERT_CNFG.ArcsTableName;
      const string geomColumnName = NETXPERT_CNFG.ArcsGeomColumnName;

      auto qry = DBHELPER::PrepareGetClosestArcQuery(arcsTableName, geomColumnName,
                                              cmap, ArcIDColumnDataType::Number, withCapacity);
      return AddNode(n, threshold, *qry, withCapacity, AddedNodeType::EndArc);
    }
    catch (const std::runtime_error& ex) {
      LOGGER::LogError("RuntimeError in AddEndNode()!");
      LOGGER::LogError(ex.what() );
    }
    catch (const std::exception& ex) {
      LOGGER::LogError("Exception in AddEndNode()!");
      LOGGER::LogError(ex.what() );
    }
    catch (...) {
      LOGGER::LogError("Unknown Error in AddEndNode() - maybe a memory error!");
    }
}

std::vector< std::pair<uint32_t, std::string> >
 InternalNet::LoadStartNodes(std::vector<NewNode> newNodes, const int threshold,
                             const std::string arcsTableName, const std::string geomColumnName,
                             const ColumnMap& cmap, const bool withCapacity) {

    using namespace std;

    auto qry = DBHELPER::PrepareGetClosestArcQuery(arcsTableName, geomColumnName,
                                            cmap, ArcIDColumnDataType::Number, withCapacity);
    vector<pair<uint32_t, string>> startNodes;

    for (auto& startNode : newNodes)
    {
        if (startNode.supply > 0)
        {
            LOGGER::LogDebug("Loading Node "+ startNode.extNodeID + "..");

            //check extNodeID if already present in nodeIDMap
            if (this->nodeIDMap.count(startNode.extNodeID) != 0) {
                LOGGER::LogWarning("External Node ID "+ startNode.extNodeID + " already present! Renaming..");
                startNode.extNodeID = startNode.extNodeID + "@netXpert";
            }

            try
            {
                auto newStartNodeID = AddNode(startNode, threshold, *qry, withCapacity, AddedNodeType::StartArc);
                startNodes.push_back( make_pair(newStartNodeID, startNode.extNodeID ) );
                LOGGER::LogDebug("New Start Node ID " + to_string(newStartNodeID)  + " - " + startNode.extNodeID);
            }
            catch (exception& ex)
            {
                LOGGER::LogError("Error in LoadStartNodes()!");
                LOGGER::LogError(ex.what());
            }
        }
    }
    return startNodes;
}

std::vector< std::pair<uint32_t, std::string> >
 InternalNet::LoadEndNodes(std::vector<NewNode> newNodes, const int threshold,
                           const std::string arcsTableName, const std::string geomColumnName,
                           const ColumnMap& cmap, const bool withCapacity) {

    using namespace std;

    auto qry = DBHELPER::PrepareGetClosestArcQuery(arcsTableName, geomColumnName,
                                            cmap, ArcIDColumnDataType::Number, withCapacity);
    vector<pair<uint32_t, string>> endNodes;

    for (auto& endNode : newNodes)
    {
        if (endNode.supply < 0)
        {
            LOGGER::LogDebug("Loading Node "+ endNode.extNodeID + "..");

            //check extNodeID if already present in nodeIDMap
            if (this->nodeIDMap.count(endNode.extNodeID) != 0) {
                LOGGER::LogWarning("External Node ID "+ endNode.extNodeID + " already present! Renaming..");
                endNode.extNodeID = endNode.extNodeID + "@netXpert";
            }

            try
            {
                auto newEndNodeID = AddNode(endNode, threshold, *qry, withCapacity, AddedNodeType::EndArc);
                endNodes.push_back( make_pair(newEndNodeID, endNode.extNodeID ) );
                LOGGER::LogDebug("New End Node ID " + to_string(newEndNodeID) + " - " + endNode.extNodeID);
            }
            catch (exception& ex)
            {
                LOGGER::LogError("Error in LoadEndNodes()!");
                LOGGER::LogError(ex.what());
            }
            //break;
        }
        //}//omp single
    }
    //}//omp parallel
    return endNodes;
}

void
 InternalNet::Reset() {

    using namespace lemon;
    using namespace std;

    LOGGER::LogInfo("Resetting network..");

    //Reset network by resetting the filtered orig arcs (and their reversed ones)

    //reset new arcs, arcChanges, remove addedNodes
//    for (auto kv : (*this->arcChangesMap) ) {
    auto iter = GetArcsIter();
    int arcChangesCnt = 0;
    int arcFilterOrigCnt = 0;

    // Iterate over all present arcs in the graph
    // note: iterator gets also the reversed arcs if present!
    for (; iter != INVALID; ++iter) {
        auto arc = iter;

        // first the splitted arcs
        // set them back to the original state
        if ( (*this->arcChangesMap)[arc] != netxpert::data::ArcState::original) {
          (*this->arcChangesMap)[arc] = netxpert::data::ArcState::original;
          arcChangesCnt += 1;
        }

        // then set all original arcs to active again
        if ( (*this->arcChangesMap)[arc] == netxpert::data::ArcState::original) {
          (*this->arcFilterMap)[arc] = true; //set the original arcs (unsplitted ones) again to active (true)
          arcFilterOrigCnt += 1;
        }

    }

    //"delete" new arcs (=splitted) through arc filter
    int counter = 0;
    // map has also the reversed arcs if present!
    for (auto& kv : this->newArcsMap) {
      auto arc = kv.first;
      (*this->arcFilterMap)[arc] = false;
      counter += 1;

      //clear new nodes in nodeIDmap
//      auto srcNodeID = (*this->nodeMap)[this->g->source(arc)];
//      auto tgtNodeID = (*this->nodeMap)[this->g->target(arc)];
      //this->nodeIDMap.erase(srcNodeID);
      //this->nodeIDMap.erase(tgtNodeID);
    }

    LOGGER::LogDebug("arcFilterOrigCnt reset # "+ to_string(arcFilterOrigCnt));
    LOGGER::LogDebug("arcChangesCnt reset # "+ to_string(arcChangesCnt));
    LOGGER::LogDebug("newArcs reset # "+ to_string(counter));

    // don't clear the newArcsMap!
    //this->newArcsMap.clear();
    this->newNodesMap.clear();

    this->addedStartPoints.clear();
    this->addedEndPoints.clear();
    this->eliminatedArcs.clear();

    //TODO CH
    #if (defined NETX_ENABLE_CONTRACTION_HIERARCHIES)
    #endif


    LOGGER::LogInfo("Done!");
 }
//--|Region Add Nodes

//-->Region Save Results

std::unique_ptr<geos::geom::LineString>
 InternalNet::getStraightLine(const std::string& orig, const std::string& dest) {

    //get geometry as straight lines from orig to dest
    auto origGeom = GetStartOrEndNodeGeometry(orig);
    auto destGeom = GetStartOrEndNodeGeometry(dest);
    auto coordFactory = DBHELPER::GEO_FACTORY->getCoordinateSequenceFactory();
    //cast to size_t with zero coordinates, otherwise there are 0.0 0.0 coords in sequence factory!
    auto coordSeq = coordFactory->create(static_cast<size_t>(0), static_cast<size_t>(2)) ; //0 coordinates, 2 dimensions
    coordSeq->add(origGeom);
    coordSeq->add(destGeom);
    std::unique_ptr<geos::geom::LineString> line ( DBHELPER::GEO_FACTORY->createLineString(coordSeq));

    return line;
 }

void
 InternalNet::ProcessSPTResultArcsMem(const std::string& orig, const std::string& dest, const netxpert::data::cost_t cost,
                                      const std::string& arcIDs, const std::vector<netxpert::data::arc_t>& routeNodeArcRep,
                                      const std::string& resultTableName, netxpert::io::DBWriter& writer,
                                      SQLite::Statement& qry //can be null in case of ESRI FileGDB
                                     ){
    using namespace netxpert::cnfg;
    using namespace geos::geom;
    std::vector<geos::geom::Geometry*> routeParts;
    //for straight lines or no geometry
    std::unique_ptr<MultiLineString> mLine (DBHELPER::GEO_FACTORY->createMultiLineString());

    switch (NETXPERT_CNFG.GeometryHandling) {
        case GEOMETRY_HANDLING::RealGeometry: {
          routeParts = addRoutePartGeoms(routeNodeArcRep);
//            std::cout << "Route from "<< orig << " to " << dest << std::endl;
//            for (auto a : routeNodeArcRep)
////                std::cout << this->g->id(this->g->source(a)) << "->" << this->g->id(this->g->target(a)) << " , ";
//                std::cout << GetOrigNodeID(this->g->source(a)) << "->" << GetOrigNodeID(this->g->target(a)) << " , ";
//
//            std::cout << std::endl;
          saveSPTResultsMem(orig, dest, cost, arcIDs, routeParts, resultTableName, writer, qry);
        }
        break;

        case GEOMETRY_HANDLING::StraightLines: {

          std::unique_ptr<LineString> line = getStraightLine(orig, dest);
          Geometry* gLine  = dynamic_cast<Geometry*> (line.get());
          mLine = std::unique_ptr<MultiLineString> ( DBHELPER::GEO_FACTORY->createMultiLineString( std::vector<Geometry*> {gLine} ));

          switch (NETXPERT_CNFG.ResultDBType) {
            case RESULT_DB_TYPE::SpatiaLiteDB: {
              #pragma omp critical
              {
              auto& sldb = dynamic_cast<SpatiaLiteWriter&>(writer);
              sldb.SaveResultArc(orig, dest, cost, *mLine, resultTableName, qry);
              }
            }
            break;
            case RESULT_DB_TYPE::ESRI_FileGDB: {
              #pragma omp critical
              {
              auto& fgdb = dynamic_cast<FGDBWriter&>(writer);
              fgdb.SaveResultArc(orig, dest, cost, *mLine, resultTableName);
              }
            }
            break;
          }
        }
        break;

        case GEOMETRY_HANDLING::NoGeometry: {
          switch (NETXPERT_CNFG.ResultDBType) {
            case RESULT_DB_TYPE::SpatiaLiteDB: {
              #pragma omp critical
              {
              auto& sldb = dynamic_cast<SpatiaLiteWriter&>(writer);
              sldb.SaveResultArc(orig, dest, cost, *mLine, resultTableName, qry);
              }
            }
            break;
            case RESULT_DB_TYPE::ESRI_FileGDB: {
              #pragma omp critical
              {
              auto& fgdb = dynamic_cast<FGDBWriter&>(writer);
              fgdb.SaveResultArc(orig, dest, cost, *mLine, resultTableName);
              }
            }
            break;
          }
        }
        break;
    }
}


void
 InternalNet::ProcessSPTResultArcsMemS(const std::string& orig, const std::string& dest, const netxpert::data::cost_t cost,
                                       const std::string& arcIDs, const std::vector<netxpert::data::arc_t>& routeNodeArcRep,
                                       std::ostringstream& output) {
    using namespace netxpert::cnfg;
    using namespace geos::geom;

    std::vector<geos::geom::Geometry*> routeParts;
    //for straight lines or no geometry
    std::unique_ptr<MultiLineString> mLine (DBHELPER::GEO_FACTORY->createMultiLineString());

    switch (NETXPERT_CNFG.GeometryHandling) {
      case GEOMETRY_HANDLING::RealGeometry: {
        routeParts = addRoutePartGeoms(routeNodeArcRep);

        saveSPTResultsMemS(orig, dest, cost, arcIDs, routeParts, output);
      }
      break;

      case GEOMETRY_HANDLING::StraightLines: {

        std::unique_ptr<LineString> line = getStraightLine(orig, dest);
        Geometry* gLine  = dynamic_cast<Geometry*> (line.get());
        mLine = std::unique_ptr<MultiLineString> ( DBHELPER::GEO_FACTORY->createMultiLineString( std::vector<Geometry*> {gLine} ));

        std::string coordStr = convertRouteToCoordList(mLine);
        /**
        {"orig" : orig, "dest" : dest, "cost" : cost, "route" : coordinateList }
        **/
        std::stringstream row;
        row << "{ \"orig\" : \""
            << orig << "\", \"dest\" : \""<< dest << "\", \"cost\" : " << cost
            << ", \"route\" : " << coordStr << "} " << std::endl;

        #pragma omp critical
        {
        //save data to stream
        output << row.str() << std::endl;
        }
      }
      break;

      case GEOMETRY_HANDLING::NoGeometry: {

        //will be empty!
        std::string coordStr = convertRouteToCoordList(mLine);
        /**
        {"orig" : orig, "dest" : dest, "cost" : cost }
        **/
        std::stringstream row;
        row << "{ \"orig\" : \""
            << orig << "\", \"dest\" : \""<< dest << "\", \"cost\" : " << cost
            << ", \"route\" : " << coordStr << "} " << std::endl;

        #pragma omp critical
        {
        //save data to stream
        output << row.str() << std::endl;
        }
      }
      break;
    }
}

// Only for unbroken, original network (e.g. MST)
// handles also case, where result DB = netxpertDB
// NO Geometry Handling --> creates always a subset of the original geometries
void
 InternalNet::ProcessResultArcs(/*const std::string& orig, const std::string& dest,
                                const double cost, const double capacity, const double flow,*/
                                const std::string& arcIDs, const std::string& resultTableName) {
    saveResults(arcIDs, resultTableName);
}

//With Geometry Handling and prepared insert query
//TODO: Geometry_Handling - not really!
//Isolines
void
 InternalNet::ProcessIsoResultArcsMem(const std::string& orig, const netxpert::data::cost_t cost,
                                      const std::string& arcIDs, const std::vector<netxpert::data::arc_t>& routeNodeArcRep,
                                      const std::string& resultTableName, netxpert::io::DBWriter& writer,
                                      SQLite::Statement& qry,
                                      const std::unordered_map<ExtNodeID, std::vector<double> > cutOffs)
{
    using namespace netxpert::cnfg;
    std::vector<geos::geom::Geometry*> routeParts;

    switch (NETXPERT_CNFG.GeometryHandling)
    {
        case GEOMETRY_HANDLING::RealGeometry:
        {
            routeParts = addRoutePartGeoms(routeNodeArcRep);
            saveIsoResultsMem(orig, cost, arcIDs, routeParts, resultTableName, writer, qry,
                            cutOffs);
        }
        break;
    }
}


void
 InternalNet::ProcessMCFResultArcsMem(const std::string& orig, const std::string& dest, const netxpert::data::cost_t cost,
                                      const netxpert::data::capacity_t capacity, const netxpert::data::flow_t flow,
                                      const std::string& arcIDs, std::vector<netxpert::data::arc_t>& routeNodeArcRep,
                                      const std::string& resultTableName, netxpert::io::DBWriter& writer,
                                      SQLite::Statement& qry //can be null in case of ESRI FileGDB
                                  )
{

    using namespace netxpert::cnfg;
    using namespace geos::geom;

    std::vector<geos::geom::Geometry*> routeParts;
    //for straight lines or no geometry
    std::unique_ptr<MultiLineString> mLine (DBHELPER::GEO_FACTORY->createMultiLineString());

    switch (NETXPERT_CNFG.GeometryHandling)
    {
        case GEOMETRY_HANDLING::RealGeometry: {
          routeParts = addRoutePartGeoms(routeNodeArcRep);
          saveMCFResultsMem(orig, dest, cost, capacity, flow, arcIDs, routeParts, resultTableName, writer, qry);
        }
        break;

        case GEOMETRY_HANDLING::StraightLines: {

          std::unique_ptr<LineString> line = getStraightLine(orig, dest);
          Geometry* gLine  = dynamic_cast<Geometry*> (line.get());
          mLine = std::unique_ptr<MultiLineString> ( DBHELPER::GEO_FACTORY->createMultiLineString( std::vector<Geometry*> {gLine} ));

          switch (NETXPERT_CNFG.ResultDBType) {
            case RESULT_DB_TYPE::SpatiaLiteDB: {
              #pragma omp critical
              {
              auto& sldb = dynamic_cast<SpatiaLiteWriter&>(writer);
              sldb.SaveResultArc(orig, dest, cost, capacity, flow, *mLine, resultTableName, qry);
              }
            }
            break;
            case RESULT_DB_TYPE::ESRI_FileGDB: {
              #pragma omp critical
              {
              auto& fgdb = dynamic_cast<FGDBWriter&>(writer);
              fgdb.SaveResultArc(orig, dest, cost, capacity, flow, *mLine, resultTableName);
              }
            }
            break;
          }
        }
        break;

        case GEOMETRY_HANDLING::NoGeometry: {
          switch (NETXPERT_CNFG.ResultDBType) {
            case RESULT_DB_TYPE::SpatiaLiteDB: {
              #pragma omp critical
              {
              auto& sldb = dynamic_cast<SpatiaLiteWriter&>(writer);
              sldb.SaveResultArc(orig, dest, cost, capacity, flow, *mLine, resultTableName, qry);
              }
            }
            break;
            case RESULT_DB_TYPE::ESRI_FileGDB: {
              #pragma omp critical
              {
              auto& fgdb = dynamic_cast<FGDBWriter&>(writer);
              fgdb.SaveResultArc(orig, dest, cost, capacity, flow, *mLine, resultTableName);
              }
            }
            break;
          }
        }
        break;
    }
}

void
 InternalNet::ProcessMCFResultArcsMemS(const std::string& orig, const std::string& dest, const netxpert::data::cost_t cost,
                                      const netxpert::data::capacity_t capacity, const netxpert::data::flow_t flow,
                                      const std::string& arcIDs, std::vector<netxpert::data::arc_t>& routeNodeArcRep,
                                      std::ostringstream& output)
{
  using namespace netxpert::cnfg;
  using namespace geos::geom;

  std::vector<geos::geom::Geometry*> routeParts;
  //for straight lines or no geometry
  std::unique_ptr<MultiLineString> mLine (DBHELPER::GEO_FACTORY->createMultiLineString());

  switch (NETXPERT_CNFG.GeometryHandling)
  {
    case GEOMETRY_HANDLING::RealGeometry: {
      routeParts = addRoutePartGeoms(routeNodeArcRep);

      saveMCFResultsMemS(orig, dest, cost, capacity, flow, arcIDs, routeParts, output);
    }
    break;

    case GEOMETRY_HANDLING::StraightLines: {

      std::unique_ptr<LineString> line = getStraightLine(orig, dest);
      Geometry* gLine  = dynamic_cast<Geometry*> (line.get());
      mLine = std::unique_ptr<MultiLineString> ( DBHELPER::GEO_FACTORY->createMultiLineString( std::vector<Geometry*> {gLine} ));

      std::string coordStr = convertRouteToCoordList(mLine);
      /**
      {"orig" : orig, "dest" : dest, "cost" : cost, "capacity": capacity, "flow" : flow, "geom" : coordinateList }
      **/
      std::stringstream row;
      row << "{ \"orig\" : \""
          << orig << "\", \"dest\" : \""<< dest << "\", \"cost\" : " << cost
          << "\", \"capacity\" : " << capacity
          << "\", \"flow\" : " << flow
          << ", \"geom\" : " << coordStr << "} " << std::endl;

      #pragma omp critical
      {
      //save data to stream
      output << row.str() << std::endl;
      }
    }
    break;

    case GEOMETRY_HANDLING::NoGeometry: {
      //will be empty!
      std::string coordStr = convertRouteToCoordList(mLine);
      /**
      {"orig" : orig, "dest" : dest, "cost" : cost, "capacity": capacity, "flow" : flow, "geom" : coordinateList }
      **/
      std::stringstream row;
      row << "{ \"orig\" : \""
          << orig << "\", \"dest\" : \""<< dest << "\", \"cost\" : " << cost
          << "\", \"capacity\" : " << capacity
          << "\", \"flow\" : " << flow
          << ", \"geom\" : " << coordStr << "} " << std::endl;

      #pragma omp critical
      {
      //save data to stream
      output << row.str() << std::endl;
      }
    }
    break;
  }
}

// Loops through all new arcs and searches in the spt path for them for getting start or end route parts
std::vector<geos::geom::Geometry*>
 InternalNet::addRoutePartGeoms(const std::vector<netxpert::data::arc_t>& routeNodeArcRep) {

    using namespace lemon;

    //LOGGER::LogDebug("Entering processRouteParts()..");

    //order of start or end segments do not matter
    std::vector<geos::geom::Geometry*> routeParts;

    if (addedStartPoints.size() > 0) {
        // 1. Add relevant start edges, that are part of the route
        for (const auto& arc : this->newArcsMap) {
            auto key = arc.first;
            auto val = arc.second;
//            bool found = false;

            if (val.nodeType == AddedNodeType::StartArc) {
                if ( std::find(routeNodeArcRep.begin(), routeNodeArcRep.end(), key) != routeNodeArcRep.end() ) {
//                    found = true;
                    auto clonedGeom = val.arcGeom;//val.arcGeom.clone();
                    routeParts.push_back(clonedGeom.get());
//                    std::cout << clonedGeom->getLength() << std::endl;
//                    std::cout << clonedGeom->getStartPoint()->toString() << " , " << clonedGeom->getEndPoint()->toString() << std::endl;
//                    std::cout << "ID# "<< g->id(key) << std::endl;
//                    std::cout << "start part arc found!" << std::endl;
//                    std::cout << "orig: " << GetOrigNodeID(this->g->source(key)) << "->" << GetOrigNodeID(this->g->target(key)) << std::endl;
//                    std::cout << "int: " << this->g->id(this->g->source(key)) << "->" << this->g->id(this->g->target(key)) << std::endl;
                    //do not break, because there can be several parts that belong to the route
                    //break;
                }
            }

        }
    }
    if (addedEndPoints.size() > 0) {
        // 1. Add relevant end edges, that are part of the route
        for (const auto& arc : this->newArcsMap) {
            auto key = arc.first;
            auto val = arc.second;
//            bool found = false;

            if (val.nodeType == AddedNodeType::EndArc) {

                if ( std::find(routeNodeArcRep.begin(), routeNodeArcRep.end(), key) != routeNodeArcRep.end() ) {
//                    found = true;
                    auto clonedGeom = val.arcGeom;//val.arcGeom.clone();
                    routeParts.push_back(clonedGeom.get());
//                    std::cout << clonedGeom->getLength() << std::endl;
//                    std::cout << clonedGeom->getStartPoint()->toString() << " , " << clonedGeom->getEndPoint()->toString() << std::endl;
//                    std::cout << "end arc found!" << std::endl;
//                    std::cout << "ID# "<< g->id(key) << std::endl;
//                    std::cout << "orig: "<<GetOrigNodeID(this->g->source(key)) << "->" << GetOrigNodeID(this->g->target(key)) << std::endl;
//                    std::cout << "int: " << this->g->id(this->g->source(key)) << "->" << this->g->id(this->g->target(key)) << std::endl;
                    //do not break, because there can be several parts that belong to the route
                    //break;
                }
            }
        }
    }
    //LOGGER::LogDebug("# "+ to_string(omp_get_thread_num()) +" : end - processRouteParts()");
    return routeParts;
}


void
 InternalNet::saveSPTResultsMem(const std::string orig, const std::string dest, const netxpert::data::cost_t cost,
                                const std::string& arcIDs, std::vector<geos::geom::Geometry*> routeParts,
                                const std::string& resultTableName, netxpert::io::DBWriter& writer,
                                SQLite::Statement& qry ) {
   //LOGGER::LogDebug("# "+ to_string(omp_get_thread_num()) +" : saveResults() - enter");

    using namespace geos::geom;
    using namespace geos::operation::linemerge;
    using namespace netxpert::cnfg;
    using namespace netxpert::io;
    using namespace netxpert::utils;

//    LOGGER::LogDebug("Entering saveSPTResultsMem()..");

    switch (NETXPERT_CNFG.ResultDBType) {
        case RESULT_DB_TYPE::SpatiaLiteDB: {
            //put all geometries in routeParts into one (perhaps disconnected) Multilinestring
            //MultilineString could also contain only one Linestring
            std::unique_ptr<MultiLineString> mLine ( DBHELPER::GEO_FACTORY->createMultiLineString( routeParts ));
            std::unique_ptr<MultiLineString> mLineDB;
            //LOGGER::LogDebug("# "+ to_string(omp_get_thread_num()) +" : saveResults() - load from DB");

            //load geometry from db
            //TODO: 0,5 bis 1 sec pro Ladevorgang
            //Stopwatch<> sw;
            //sw.start();
            if (arcIDs.size() > 0) {
                mLineDB = std::move( DBHELPER::GetArcGeometriesFromMem(arcIDs) );
            }
            //sw.stop();
            //LOGGER::LogDebug("DBHELPER::TEST_GetArcGeometriesFromRAM() took " + to_string(sw.elapsed()/1000)+" ms");
            //LOGGER::LogDebug("# "+ to_string(omp_get_thread_num()) +" : saveResults() - merge");
            //merge routeParts with original arcs
            //sw.start();
            LineMerger lm;
            if (! (mLine->isEmpty()) )
                lm.add(mLine.get());

            if (mLineDB)
                lm.add(mLineDB.get());

            std::unique_ptr< std::vector<LineString *> > mls ( lm.getMergedLineStrings() );
            std::vector<Geometry*> mls2;
            for (auto& l : *mls) {
                //if (!l->isEmpty())
                mls2.push_back(dynamic_cast<Geometry*>(l));
            }

            std::unique_ptr<MultiLineString> route (DBHELPER::GEO_FACTORY->createMultiLineString( mls2 ) );

            //sw.stop();
            //LOGGER::LogDebug("Merging Geometry with Geos took " + to_string(sw.elapsed())+" mcs");
            //LOGGER::LogDebug("# "+ to_string(omp_get_thread_num()) +" : saveResults() - save to DB");
            //sw.start();
            #pragma omp critical
            {
            auto& sldb = dynamic_cast<SpatiaLiteWriter&>(writer);
            //save merged route geometry to db
            sldb.SaveResultArc(orig, dest, cost, *route, resultTableName, qry);
            //sw.stop();
            //LOGGER::LogDebug("SaveResultArc() took " + to_string(sw.elapsed())+" mcs");
            }
        }
        break;

        case RESULT_DB_TYPE::ESRI_FileGDB: {
            /*
                Get all geometries from Spatialite DB (per arcIDs) of one route and merge them with
                all the segments of routeParts;
                Save the geometry with SaveSolveQueryToDB()
            */
            std::unique_ptr<MultiLineString> mLine ( DBHELPER::GEO_FACTORY->createMultiLineString( routeParts ));
            std::unique_ptr<MultiLineString> mLineDB;
            //Stopwatch<> sw;
            //sw.start();
            if (arcIDs.size() > 0) {
                mLineDB = std::move( DBHELPER::GetArcGeometriesFromMem(arcIDs) );
            }
            //sw.stop();
            //LOGGER::LogDebug("DBHELPER::TEST_GetArcGeometriesFromRAM() took " + to_string(sw.elapsed()/1000)+" ms");

            //merge
            //sw.start();
            LineMerger lm;
            if (! (mLine->isEmpty()) )
                lm.add(mLine.get());

            if (mLineDB)
                lm.add(mLineDB.get());

            std::unique_ptr< std::vector<LineString *> > mls ( lm.getMergedLineStrings() );
            std::vector<Geometry*> mls2;
            for (auto& l : *mls) {
                //if (!l->isEmpty())
                mls2.push_back(dynamic_cast<Geometry*>(l));
            }

            std::unique_ptr<MultiLineString> route (DBHELPER::GEO_FACTORY->createMultiLineString( mls2 ) );
            //sw.stop();
            //LOGGER::LogDebug("Merging Geometry with Geos took " + to_string(sw.elapsed())+" mcs");
            auto& fgdb = dynamic_cast<FGDBWriter&>(writer);
            //LOGGER::LogDebug("# "+ to_string(omp_get_thread_num()) +" : saveResults() - save to DB");
            #pragma omp critical
            {
            //sw.start();
            fgdb.SaveResultArc(orig, dest, cost, *route, resultTableName);
            //sw.stop();
            //LOGGER::LogDebug("SaveResultArc() took " + to_string(sw.elapsed())+" mcs");
            }
        }
            break;
    }
    //cout << "nexpert::Network - saved" <<endl;
}

std::string
 InternalNet::convertRouteToCoordList(std::unique_ptr<geos::geom::MultiLineString>& route) {

    //data to JSON
    const std::vector<geos::geom::Coordinate>* coords = route->getCoordinates()->toVector();

    std::stringstream coordStr;
    int counter = 0;
    for (auto& c : *coords) {
      counter += 1;
      coordStr << "["
      << std::fixed << std::setprecision(0) << c.x << ","
      << std::fixed << std::setprecision(0) << c.y << "]";

      if (counter < coords->size())
        coordStr << ",";
    }

    return "[" + coordStr.str() += "]";
}

void
 InternalNet::saveSPTResultsMemS(const std::string orig, const std::string dest, const netxpert::data::cost_t cost,
                                const std::string& arcIDs, std::vector<geos::geom::Geometry*> routeParts,
                                std::ostringstream& outStream) {

    using namespace geos::geom;
    using namespace geos::operation::linemerge;
    using namespace netxpert::cnfg;
    using namespace netxpert::io;
    using namespace netxpert::utils;

    #if DEBUG
    LOGGER::LogDebug("entering saveSPTResultsMemS()..");
    #endif

    //put all geometries in routeParts into one (perhaps disconnected) Multilinestring
    //MultilineString could also contain only one Linestring
    std::unique_ptr<MultiLineString> mLine ( DBHELPER::GEO_FACTORY->createMultiLineString( routeParts ));
    std::unique_ptr<MultiLineString> mLineDB;
    //LOGGER::LogDebug("# "+ to_string(omp_get_thread_num()) +" : saveResults() - load from DB");

    //load geometry from db
    //TODO: 0,5 bis 1 sec pro Ladevorgang
    //Stopwatch<> sw;
    //sw.start();
    if (arcIDs.size() > 0) {
        mLineDB = std::move( DBHELPER::GetArcGeometriesFromMem(arcIDs) );
    }
    //sw.stop();
    //LOGGER::LogDebug("DBHELPER::TEST_GetArcGeometriesFromRAM() took " + to_string(sw.elapsed()/1000)+" ms");
    //LOGGER::LogDebug("# "+ to_string(omp_get_thread_num()) +" : saveResults() - merge");
    //merge routeParts with original arcs
    //sw.start();
    LineMerger lm;
    if (! (mLine->isEmpty()) )
        lm.add(mLine.get());

    if (mLineDB)
        lm.add(mLineDB.get());

    std::unique_ptr< std::vector<LineString *> > mls ( lm.getMergedLineStrings() );
    std::vector<Geometry*> mls2;
    for (auto& l : *mls) {
        //if (!l->isEmpty())
        mls2.push_back(dynamic_cast<Geometry*>(l));
    }

    std::unique_ptr<MultiLineString> route (DBHELPER::GEO_FACTORY->createMultiLineString( mls2 ) );

    std::string coordStr = convertRouteToCoordList(route);

    /**
    {"orig" : orig, "dest" : dest, "cost" : cost, "route" : coordinateList }
    **/
    std::stringstream row;
    row << "{ \"orig\" : \""
        << orig << "\", \"dest\" : \""<< dest << "\", \"cost\" : " << cost
        << ", \"route\" : " << coordStr << "} " << std::endl;

    //sw.stop();
    //LOGGER::LogDebug("Merging Geometry with Geos took " + to_string(sw.elapsed())+" mcs");
    //LOGGER::LogDebug("# "+ to_string(omp_get_thread_num()) +" : saveResults() - save to DB");
    //sw.start();
    #pragma omp critical
    {
    //save merged route geometry to stream
    outStream << row.str() << std::endl;
    }
}

//Isolines
void
 InternalNet::saveIsoResultsMem(const std::string orig, const netxpert::data::cost_t cost,
                               const std::string& arcIDs, std::vector<geos::geom::Geometry*> routeParts,
                               const std::string& resultTableName, netxpert::io::DBWriter& writer,
                               SQLite::Statement& qry,
                               const std::unordered_map<ExtNodeID, std::vector<double> >& cutOffs )
{
   //LOGGER::LogDebug("# "+ to_string(omp_get_thread_num()) +" : saveResults() - enter");
    using namespace std;
    using namespace geos::geom;
    using namespace geos::operation::linemerge;
    using namespace netxpert::cnfg;
    using namespace netxpert::io;
    using namespace netxpert::utils;

    switch (NETXPERT_CNFG.ResultDBType)
    {
    case RESULT_DB_TYPE::SpatiaLiteDB:
    {
        //put all geometries in routeParts into one (perhaps disconnected) Multilinestring
        //MultilineString could also contain only one Linestring
        unique_ptr<MultiLineString> mLine ( DBHELPER::GEO_FACTORY->createMultiLineString( routeParts ));

        unique_ptr<MultiLineString> mLineDB;
        //LOGGER::LogDebug("# "+ to_string(omp_get_thread_num()) +" : saveResults() - load from DB");

        //Stopwatch<> sw;
        //sw.start();

        if (arcIDs.size() > 0)
            mLineDB = move( DBHELPER::GetArcGeometriesFromMem(arcIDs) );

        //sw.stop();
        //LOGGER::LogDebug("DBHELPER::TEST_GetArcGeometriesFromRAM() took " + to_string(sw.elapsed()/1000)+" ms");
        //LOGGER::LogDebug("# "+ to_string(omp_get_thread_num()) +" : saveResults() - merge");
        //merge routeParts with original arcs
        //sw.start();

        LineMerger lm;
        if (mLineDB)
            lm.add(mLineDB.get());

        if (! mLine->isEmpty() )
            lm.add(mLine.get());

        std::unique_ptr< vector<LineString *> > mls ( lm.getMergedLineStrings() );
        vector<Geometry*> mls2;
        for (auto& l : *mls)
        {
            //if (!l->isEmpty())
            mls2.push_back(dynamic_cast<Geometry*>(l));
        }

        unique_ptr<MultiLineString> route (DBHELPER::GEO_FACTORY->createMultiLineString( mls2 ) );

        // for each cut off value, if matched the origin key create a route cut down to the value
        std::vector<double>::const_iterator cutOffIt;
        #pragma omp critical
        {
        auto& sldb = dynamic_cast<SpatiaLiteWriter&>(writer);
        std::vector<double> cutOffValues;

        if (cutOffs.count(orig) > 0)
            cutOffValues = cutOffs.at(orig);

        for (cutOffIt = cutOffValues.begin(); cutOffIt != cutOffValues.end(); cutOffIt++)
        {
            double cutOff = *cutOffIt;
            auto startCoord = this->GetStartOrEndNodeGeometry(orig);
            //std::cout << startCoord.toString() << std::endl;
            double relCost = cost;
            //relCost as reference
            auto cuttedLine = cutLine(startCoord, *route, cutOff, relCost);

            //LOGGER::LogDebug("# "+ to_string(omp_get_thread_num()) +" : saveResults() - save to DB");
            //sw.start();
            //save merged route geometry to db
            sldb.SaveResultArc(orig, relCost, cutOff, *cuttedLine, resultTableName, qry);
            //sw.stop();
            //LOGGER::LogDebug("SaveResultArc() took " + to_string(sw.elapsed())+" mcs");
        }
        }
    }
        break;

    case RESULT_DB_TYPE::ESRI_FileGDB:
    {
        /*
            Get all geometries from Spatialite DB (per arcIDs) of one route and merge them with
            all the segments of routeParts;
            Save the geometry with SaveSolveQueryToDB()
        */
        unique_ptr<MultiLineString> mLine ( DBHELPER::GEO_FACTORY->createMultiLineString( routeParts ));

        unique_ptr<MultiLineString> mLineDB;

        if (arcIDs.size() > 0)
            mLineDB = move( DBHELPER::GetArcGeometriesFromMem(arcIDs) );

        LineMerger lm;
        if (mLineDB)
            lm.add(mLineDB.get());

        if (! mLine->isEmpty() )
            lm.add(mLine.get());

        std::unique_ptr< vector<LineString *> > mls ( lm.getMergedLineStrings() );
        vector<Geometry*> mls2;
        for (auto& l : *mls)
        {
            //if (!l->isEmpty())
            mls2.push_back(dynamic_cast<Geometry*>(l));
        }

        unique_ptr<MultiLineString> route (DBHELPER::GEO_FACTORY->createMultiLineString( mls2 ) );

        // for each cut off value, if matched the origin key create a route cut down to the value
        std::vector<double>::const_iterator cutOffIt;
        #pragma omp critical
        {
        auto& fgdb = dynamic_cast<FGDBWriter&>(writer);
        std::vector<double> cutOffValues;

        if (cutOffs.count(orig) > 0)
            cutOffValues = cutOffs.at(orig);

        for (cutOffIt = cutOffValues.begin(); cutOffIt != cutOffValues.end(); cutOffIt++)
        {
            double cutOff = *cutOffIt;
            auto startCoord = this->GetStartOrEndNodeGeometry(orig);
            //std::cout << startCoord.toString() << std::endl;
            double relCost = cost;
            //relCost as reference
            auto cuttedLine = cutLine(startCoord, *route, cutOff, relCost);
            fgdb.SaveResultArc(orig, relCost, cutOff,
                                    *cuttedLine, resultTableName);

        }
        }
    }
        break;
    }

}

//MCF
void InternalNet::saveMCFResultsMem(const std::string orig, const std::string dest, const netxpert::data::cost_t cost,
                                 const netxpert::data::capacity_t capacity, const netxpert::data::flow_t flow,
                                 const std::string& arcIDs, std::vector<geos::geom::Geometry*> routeParts,
                                 const std::string& resultTableName, netxpert::io::DBWriter& writer,
                                 SQLite::Statement& qry ) {

    using namespace std;
    using namespace geos::geom;
    using namespace geos::operation::linemerge;
    using namespace netxpert::cnfg;
    using namespace netxpert::io;
    using namespace netxpert::utils;

    bool isResultDBequalNetXpertDB = false;
    if (NETXPERT_CNFG.ResultDBPath == NETXPERT_CNFG.NetXDBPath)
    {
        isResultDBequalNetXpertDB = true;
    }

    switch (NETXPERT_CNFG.ResultDBType)
    {
        case RESULT_DB_TYPE::SpatiaLiteDB:
        {
            //special case: we can write directly into netxpert db without creating a new db
            if (isResultDBequalNetXpertDB)
            {
                auto& sldb = dynamic_cast<SpatiaLiteWriter&>(writer);
                //put all geometries in routeParts into one (perhaps disconnected) Multilinestring
                //MultilineString could also contain only one Linestring
                unique_ptr<MultiLineString> mLine ( DBHELPER::GEO_FACTORY->createMultiLineString( routeParts ));

                #pragma omp critical
                {
                sldb.MergeAndSaveResultArcs(orig, dest, cost, capacity, flow, NETXPERT_CNFG.ArcsGeomColumnName,
                    NETXPERT_CNFG.ArcIDColumnName, NETXPERT_CNFG.ArcsTableName,
                    arcIDs, *mLine, resultTableName);
                }
            }
            else
            {
                //put all geometries in routeParts into one (perhaps disconnected) Multilinestring
                //MultilineString could also contain only one Linestring
                unique_ptr<MultiLineString> mLine ( DBHELPER::GEO_FACTORY->createMultiLineString( routeParts ));

                unique_ptr<MultiLineString> mLineDB;
                //LOGGER::LogDebug("# "+ to_string(omp_get_thread_num()) +" : saveResults() - load from DB");

                //load geometry from db
                //TODO: 0,5 bis 1 sec pro Ladevorgang
                //Stopwatch<> sw;
                //sw.start();

//                LOGGER::LogDebug("ArcIDS: "+arcIDs);

                if (arcIDs.size() > 0)
                    mLineDB = move( DBHELPER::GetArcGeometriesFromMem(arcIDs) );
                //sw.stop();
                //LOGGER::LogDebug("DBHELPER::TEST_GetArcGeometriesFromRAM() took " + to_string(sw.elapsed()/1000)+" ms");
                //LOGGER::LogDebug("# "+ to_string(omp_get_thread_num()) +" : saveResults() - merge");
                //merge routeParts with original arcs
                //sw.start();
                LineMerger lm;

                if (! (mLine->isEmpty()) )
                    lm.add(mLine.get());

                if (mLineDB)
                    lm.add(mLineDB.get());

                std::unique_ptr< vector<LineString *> > mls ( lm.getMergedLineStrings() );
                vector<Geometry*> mls2;
                for (auto& l : *mls)
                {
                    //if (!l->isEmpty())
                    mls2.push_back(dynamic_cast<Geometry*>(l));
                }

                unique_ptr<MultiLineString> route (DBHELPER::GEO_FACTORY->createMultiLineString( mls2 ) );

                //sw.stop();
                //LOGGER::LogDebug("Merging Geometry with Geos took " + to_string(sw.elapsed())+" mcs");

                //LOGGER::LogDebug("# "+ to_string(omp_get_thread_num()) +" : saveResults() - save to DB");

                //sw.start();
                #pragma omp critical
                {
                auto& sldb = dynamic_cast<SpatiaLiteWriter&>(writer);
                //save merged route geometry to db

                sldb.SaveResultArc(orig, dest, cost, capacity, flow, *route, resultTableName, qry);
                //sw.stop();
                //LOGGER::LogDebug("SaveResultArc() took " + to_string(sw.elapsed())+" mcs");
                }
            }
        }
            break;

        case RESULT_DB_TYPE::ESRI_FileGDB:
        {
            /*
                Get all geometries from Spatialite DB (per arcIDs) of one route and merge them with
                all the segments of routeParts;
                Save the geometry with SaveSolveQueryToDB()
            */
            unique_ptr<MultiLineString> mLine ( DBHELPER::GEO_FACTORY->createMultiLineString( routeParts ));

            unique_ptr<MultiLineString> mLineDB;
            //Stopwatch<> sw;
            //sw.start();
            if (arcIDs.size() > 0)
                mLineDB = move( DBHELPER::GetArcGeometriesFromMem(arcIDs) );
            //sw.stop();
            //LOGGER::LogDebug("DBHELPER::TEST_GetArcGeometriesFromRAM() took " + to_string(sw.elapsed()/1000)+" ms");

            //merge
            //sw.start();
            LineMerger lm;
            if (! (mLine->isEmpty()) )
                lm.add(mLine.get());

            if (mLineDB)
                lm.add(mLineDB.get());

            std::unique_ptr< vector<LineString *> > mls ( lm.getMergedLineStrings() );
            vector<Geometry*> mls2;
            for (auto& l : *mls)
            {
                //if (!l->isEmpty())
                mls2.push_back(dynamic_cast<Geometry*>(l));
            }

            unique_ptr<MultiLineString> route (DBHELPER::GEO_FACTORY->createMultiLineString( mls2 ) );
            //sw.stop();
            //LOGGER::LogDebug("Merging Geometry with Geos took " + to_string(sw.elapsed())+" mcs");
            auto& fgdb = dynamic_cast<FGDBWriter&>(writer);
            //LOGGER::LogDebug("# "+ to_string(omp_get_thread_num()) +" : saveResults() - save to DB");
            #pragma omp critical
            {
            //sw.start();
            fgdb.SaveResultArc(orig, dest, cost, capacity, flow,
                                *route, resultTableName);
            //sw.stop();
            //LOGGER::LogDebug("SaveResultArc() took " + to_string(sw.elapsed())+" mcs");
            }
        }
            break;
    }
    //cout << "nexpert::Network - saved" <<endl;
}

//MCF
void InternalNet::saveMCFResultsMemS(const std::string orig, const std::string dest, const netxpert::data::cost_t cost,
                                 const netxpert::data::capacity_t capacity, const netxpert::data::flow_t flow,
                                 const std::string& arcIDs, std::vector<geos::geom::Geometry*> routeParts,
                                 std::ostringstream& outStream) {

    using namespace std;
    using namespace geos::geom;
    using namespace geos::operation::linemerge;
    using namespace netxpert::cnfg;
    using namespace netxpert::io;
    using namespace netxpert::utils;

    //put all geometries in routeParts into one (perhaps disconnected) Multilinestring
    //MultilineString could also contain only one Linestring
    unique_ptr<MultiLineString> mLine ( DBHELPER::GEO_FACTORY->createMultiLineString( routeParts ));

    unique_ptr<MultiLineString> mLineDB;
    //LOGGER::LogDebug("# "+ to_string(omp_get_thread_num()) +" : saveResults() - load from DB");

    //load geometry from db
    //TODO: 0,5 bis 1 sec pro Ladevorgang
    //Stopwatch<> sw;
    //sw.start();

    //LOGGER::LogDebug("ArcIDS: "+arcIDs);

    if (arcIDs.size() > 0)
        mLineDB = move( DBHELPER::GetArcGeometriesFromMem(arcIDs) );
    //sw.stop();
    //LOGGER::LogDebug("DBHELPER::TEST_GetArcGeometriesFromRAM() took " + to_string(sw.elapsed()/1000)+" ms");
    //LOGGER::LogDebug("# "+ to_string(omp_get_thread_num()) +" : saveResults() - merge");
    //merge routeParts with original arcs
    //sw.start();
    LineMerger lm;

    if (! (mLine->isEmpty()) )
        lm.add(mLine.get());

    if (mLineDB)
        lm.add(mLineDB.get());

    std::unique_ptr< vector<LineString *> > mls ( lm.getMergedLineStrings() );
    vector<Geometry*> mls2;
    for (auto& l : *mls)
    {
        //if (!l->isEmpty())
        mls2.push_back(dynamic_cast<Geometry*>(l));
    }

    unique_ptr<MultiLineString> route (DBHELPER::GEO_FACTORY->createMultiLineString( mls2 ) );

    std::string coordStr = convertRouteToCoordList(route);

    /**
    {"orig" : orig, "dest" : dest, "cost" : cost, "capacity": capacity, "flow" : flow, "geom" : coordinateList }
    **/
    std::stringstream row;
    row << "{ \"orig\" : \""
        << orig << "\", \"dest\" : \""<< dest << "\", \"cost\" : " << cost
        << "\", \"capacity\" : " << capacity
        << "\", \"flow\" : " << flow
        << ", \"geom\" : " << coordStr << "} " << endl;

    //sw.stop();
    //LOGGER::LogDebug("Merging Geometry with Geos took " + to_string(sw.elapsed())+" mcs");
    //LOGGER::LogDebug("# "+ to_string(omp_get_thread_num()) +" : saveResults() - save to DB");
    //sw.start();
    #pragma omp critical
    {
    //save data to stream
    outStream << row.str() << endl;
    }

    //sw.stop();
    //LOGGER::LogDebug("SaveResultArc() took " + to_string(sw.elapsed())+" mcs");
}

/**
* For results of original arcs only
*/
void
 InternalNet::saveResults(const std::string& arcIDs, const std::string& resultTableName) {

    using namespace std;
    using namespace netxpert::data;
    using namespace netxpert::cnfg;
    using namespace netxpert::io;
    using namespace geos::geom;

    bool isResultDBequalNetXpertDB = false;
    if (NETXPERT_CNFG.ResultDBPath == NETXPERT_CNFG.NetXDBPath)
    {
        isResultDBequalNetXpertDB = true;
    }

    switch (NETXPERT_CNFG.ResultDBType)
    {

        case RESULT_DB_TYPE::SpatiaLiteDB:
        {
            //special case: we can write directly into netxpert db without creating a new db
            if (isResultDBequalNetXpertDB)
            {
                //Override result DB Path with original netXpert DB path
                //for using MergeAndSaveResultArcs()
                SpatiaLiteWriter sldb (NETXPERT_CNFG, NETXPERT_CNFG.NetXDBPath);
                sldb.OpenNewTransaction();
                sldb.MergeAndSaveResultArcs(NETXPERT_CNFG.CostColumnName, NETXPERT_CNFG.ArcsGeomColumnName,
                    NETXPERT_CNFG.ArcIDColumnName, NETXPERT_CNFG.ArcsTableName,
                    arcIDs, resultTableName);
                sldb.CommitCurrentTransaction();
                sldb.CloseConnection();
            }
            else{
                //Load result arc from DB
                SpatiaLiteWriter sldb (NETXPERT_CNFG);
                sldb.OpenNewTransaction();
                //Prepare query once!
                auto qry = sldb.PrepareSaveResultArc(resultTableName, NetXpertSolver::MinSpanningTreeSolver);
                //for arc in arcIDs: load geometry from db and save result to resultDB
                unique_ptr<MultiLineString> arc;
                if (arcIDs.size() > 0)
                {
                    auto tokens = UTILS::Split(arcIDs, ',');
                    double cost = 0;
                    for (string& s : tokens)
                    {
                        arc = DBHELPER::GetArcGeometriesFromDB(NETXPERT_CNFG.ArcsTableName, NETXPERT_CNFG.ArcIDColumnName,
                                                     NETXPERT_CNFG.ArcsGeomColumnName, ArcIDColumnDataType::Number,
                                                     s);
                        //TODO: cost
                        sldb.SaveResultArc(s, cost, *arc, resultTableName, *qry);
                    }
                }
                sldb.CommitCurrentTransaction();
                sldb.CloseConnection();
            }
        }
            break;

        case RESULT_DB_TYPE::ESRI_FileGDB:
        {
            FGDBWriter fgdb (NETXPERT_CNFG);
            fgdb.OpenNewTransaction();

            unique_ptr<MultiLineString> arc;
            if (arcIDs.size() > 0)
            {
                auto tokens = UTILS::Split(arcIDs, ',');
                double cost = 0;
                for (string& s : tokens)
                {
                    arc = DBHELPER::GetArcGeometriesFromDB(NETXPERT_CNFG.ArcsTableName,
                                                                NETXPERT_CNFG.ArcIDColumnName,
                                                                NETXPERT_CNFG.ArcsGeomColumnName,
                                                            ArcIDColumnDataType::Number, s);
                    //TODO: cost
                    fgdb.SaveResultArc(s, cost, *arc, resultTableName);
                }
            }
            fgdb.CommitCurrentTransaction();
            fgdb.CloseConnection();
        }
            break;
    }
}
//--|Region Save Results

//-->Region Geo Utils

const double
 InternalNet::getPositionOfPointAlongLine(const geos::geom::Coordinate& coord,
                                          const geos::geom::Geometry& arc) {

    using namespace geos::linearref;

    try {
        LengthIndexedLine idxLine ( &arc );

        double pointIdx = idxLine.indexOf(coord);

        if (idxLine.isValidIndex( pointIdx) )
            return pointIdx;
        else
            return idxLine.clampIndex( pointIdx );
    }
    catch (std::exception& ex) {
        LOGGER::LogError( "Error getting position along line!" );
        LOGGER::LogError( ex.what() );
        throw ex;
    }
}

const netxpert::data::StartOrEndLocationOfLine
 InternalNet::getLocationOfPointOnLine(const geos::geom::Coordinate& coord,
                                       const geos::geom::Geometry& arc) {

    //using namespace geos::geom;
    using namespace geos::linearref;
    //using namespace geos::operation::linemerge;

    try {
        LengthIndexedLine idxLine ( &arc );

        double pointIdx = idxLine.indexOf(coord);
        double startIdx = idxLine.getStartIndex();
        double endIdx = idxLine.getEndIndex();

        //cout << "Indexes: "<< endl;
        //cout << pointIdx << endl << startIdx << endl << endIdx << endl;

        //validate Index
        if (!idxLine.isValidIndex( pointIdx) )
            pointIdx = idxLine.clampIndex( pointIdx );

        if (pointIdx == startIdx)
            return StartOrEndLocationOfLine::Start;

        if (pointIdx == endIdx)
            return StartOrEndLocationOfLine::End;

        else
            return StartOrEndLocationOfLine::Intermediate;
    }
    catch (std::exception& ex) {
        LOGGER::LogError( "Error getting position along line!" );
        LOGGER::LogError( ex.what() );
        throw ex;
    }
}

template<typename T>
inline const T
 InternalNet::getRelativeValueFromGeomLength(const T attrValue,
                                             const geos::geom::MultiLineString& completeLine,
                                             const geos::geom::LineString& segment) {

    return ( segment.getLength() / completeLine.getLength() ) * attrValue;
}


const netxpert::data::IntNetSplittedArc<arc_t>
 InternalNet::getSplittedClosestOrigArcToPoint(const geos::geom::Coordinate coord, const int threshold,
                                              const std::pair<ExternalArc,ArcData>& arcData,
                                              const geos::geom::Geometry& arcGeom) {

    using namespace std;
    using namespace geos::geom;

    try {
        ExternalArc nearestArcKey = arcData.first;
        ArcData nearestArc        = arcData.second;

        auto mLine = splitLine(coord, arcGeom);

        auto arc = GetArcFromOrigID( nearestArc.extArcID );
//        auto intArc = GetArcFromID( std::stoul(nearestArc.extArcID) );

        netxpert::data::IntNetSplittedArc<arc_t> result{ arc, nearestArc.cost, nearestArc.capacity, mLine };

        return result;
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error splitting new line!" );
        LOGGER::LogError( ex.what() );
        throw ex;
    }
}

const netxpert::data::IntNetSplittedArc<arc_t>
 InternalNet::getSplittedClosestNewArcToPoint(const geos::geom::Coordinate& coord,
                                              const int threshold) {

    using namespace std;
    using namespace geos::geom;
    using namespace geos::operation::distance;

    try
    {
        //GeometryFactory gFac;
        shared_ptr<const Point> p ( DBHELPER::GEO_FACTORY->createPoint(coord) );
        shared_ptr<const Geometry> gPtr ( dynamic_pointer_cast<const Geometry>(p) );
        shared_ptr<const Geometry> buf ( gPtr->buffer(threshold) );
        //must not defined as smart_pointer!
        const Envelope* bufEnv = buf->getEnvelopeInternal();

        map<double, pair<graph_t::Arc, NewArc> > distanceTbl;

        /*// search in spatial index for relevant geometry
        // with buffer around point (threshold)*/

        //1. Generate Distance Table
        for ( auto nArc : this->newArcsMap )
        {
            graph_t::Arc key = nArc.first;
            NewArc val       = nArc.second;

            //NewArc geometry as shared_ptr
            //Geometry* g2Ptr = static_cast<Geometry*>( val.arcGeom.get() );
            Geometry* g2Ptr = dynamic_cast<Geometry*>( val.arcGeom.get() );

            // "spatial index like":
            // calculate distance table only for those geometries
            // that envelopes do intersect

            //must not defined as smart_pointer!
            const Envelope* g2Env = g2Ptr->getEnvelopeInternal() ;

            if(bufEnv->intersects(g2Env))
            {
                DistanceOp distCalc(*gPtr, *g2Ptr);
                //cout << distCalc.distance() << endl << val.arcGeom.toString() << endl;
                distanceTbl.insert( make_pair(distCalc.distance(),
                                    make_pair(key, val) ) );
            }
        }
        //map is sorted by distance - so take the first
        auto elem = *distanceTbl.begin();
        //double dist = elem.first;
        graph_t::Arc nearestArcKey = elem.second.first;
        NewArc nearestArc          = elem.second.second;

        auto mLine = splitLine(coord, static_cast<Geometry&>( *nearestArc.arcGeom ));

        netxpert::data::IntNetSplittedArc<arc_t> result{ nearestArcKey,
                                                         nearestArc.cost,
                                                         nearestArc.capacity,
                                                         mLine };
        return result;
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error splitting new line!" );
        LOGGER::LogError( ex.what() );
        throw ex;
    }
}

//std::shared_ptr<geos::geom::MultiLineString>
std::pair<std::shared_ptr<geos::geom::Geometry>, std::shared_ptr<geos::geom::Geometry>>
 InternalNet::splitLine(const geos::geom::Coordinate& coord,
                        const geos::geom::Geometry& lineGeom) {

    using namespace std;
    using namespace geos::geom;
    using namespace geos::linearref;

    const Geometry* gPtr = &lineGeom;

    // normalize frist
    //unique_ptr<LineString> l ( dynamic_cast<LineString*>( DBHELPER::GEO_FACTORY->createLineString(gPtr->getCoordinates())) );
    //l->normalize();

    /*geos::io::WKTReader r;
    string wkt = "LINESTRING (703475.8942999998107553 5364706.0032000001519918, 703442.5213000001385808 5364728.6747999992221594, 703419.0372000001370907 5364740.5065999999642372, 703391.1070999996736646 5364743.4655000008642673)";
    shared_ptr<const Geometry> geomPtr1 (r.read(wkt));*/

    //do the split with Linear Referencing
    // Line must be LINESTRING as Linear Ref on MULTILINESTRINGs does not work properly
    //orientation of line should not be important
    LengthIndexedLine idxLine = geos::linearref::LengthIndexedLine( gPtr );//get for raw pointer
    //LengthIndexedLine idxLine = geos::linearref::LengthIndexedLine( l.get() );//get for raw pointer

    double pointIdx = idxLine.indexOf(coord);
    //assumption that orientation of line should be aligned to fromNode and toNode
    double startIdx = idxLine.getStartIndex();
    double endIdx = idxLine.getEndIndex();

//    cout << "Indexes: "<< endl;
//    cout << startIdx << endl << pointIdx << endl << endIdx << endl;

    shared_ptr<Geometry> seg1 ( idxLine.extractLine(startIdx, pointIdx) );
    shared_ptr<Geometry> seg2 ( idxLine.extractLine(pointIdx, endIdx) );

    /*shared_ptr<MultiLineString> mLine ( DBHELPER::GEO_FACTORY->createMultiLineString(
                                          vector<Geometry*>{ seg1.get() , seg2.get() } )
                                        );*/
//    std::cout << "seg1: " << seg1->toString() << std::endl;
//    std::cout << "seg2: " << seg2->toString() << std::endl;

    std::pair<std::shared_ptr<Geometry>, std::shared_ptr<Geometry>> result = std::make_pair(seg1, seg2);
    return result;
}

std::vector<std::shared_ptr<geos::geom::Geometry>>
 InternalNet::clipLine(const geos::geom::Geometry& polyGeom,
                         const geos::geom::Geometry& lineGeom) {

    using namespace std;
    using namespace geos::geom;

    const Geometry* pPtr = &polyGeom;
    const Geometry* lPtr = &lineGeom;

    //do the clip
    shared_ptr<Geometry> diff ( lPtr->difference(pPtr) );

    //diff could be LINESTRING (clip at start or end part of the line)
    // or MULTILINESTRING (normally)
    size_t numParts = diff->getNumGeometries();
    std::vector<std::shared_ptr<Geometry>> result;

    for (int i = 0; i < numParts; i++) {
        result.push_back( shared_ptr<Geometry>( diff->getGeometryN(i)->clone()) );
    }
    return result;
}

std::unique_ptr<geos::geom::MultiLineString>
 InternalNet::cutLine(const geos::geom::Coordinate& startCoord,
                      const geos::geom::Geometry& lineGeom,
                      const netxpert::data::cost_t cutOff,
                      netxpert::data::cost_t& cost) {

    using namespace std;
    using namespace geos::geom;

    unique_ptr<Geometry> gPtr ( lineGeom.clone() );

    //should be linestring - multilinestring does not work!
    unique_ptr<LineString> l ( dynamic_cast<LineString*>( DBHELPER::GEO_FACTORY->createLineString(gPtr->getCoordinates())) );
    //don't normalize!
    //l->normalize();

    //do the split with Linear Referencing
    // Line must be LINESTRING as Linear Ref on MULTILINESTRINGs does not work properly
    geos::linearref::LocationIndexedLine idxLine = geos::linearref::LocationIndexedLine( l.get() );

    auto pointLoc = idxLine.project(startCoord);
    //snap tolerance 10m
    pointLoc.snapToVertex(l.get(), 10);

    auto startLoc = idxLine.getStartIndex();
    auto endLoc = idxLine.getEndIndex();

    //finalRoute could be reversed!
    //Start loc segment fraction must be near 0
    unique_ptr<Geometry> line;

    bool reverseLine = false;
    //cout << startCoord.toString() << endl;
    //cout << l->getStartPoint()->getCoordinate()->toString() << endl;

    if ( !(idxLine.extractPoint(pointLoc).equals2D(*(l->getStartPoint()->getCoordinate()))) )
    {
        //cout << "Start point of line is NOT equal to digitized point" <<endl;
        reverseLine = true;
    }

    //start coord lies not on start point of the line
    if (reverseLine)
    {
        //cout << "cutting reverse linestring"<< endl;
        unique_ptr<LineString> newGptr ( dynamic_cast<LineString*>( l->reverse()) );

        idxLine = geos::linearref::LocationIndexedLine(newGptr.get());

        pointLoc = idxLine.project(startCoord);
        pointLoc.snapToVertex(newGptr.get(), 10);

        startLoc = idxLine.getStartIndex();
        endLoc = idxLine.getEndIndex();

        //static method
        geos::linearref::LinearLocation cutOffLoc =
            geos::linearref::LengthLocationMap::getLocation(newGptr.get(), cutOff);

        line = unique_ptr<Geometry>( idxLine.extractLine(startLoc, cutOffLoc) );

        cost = cutOffLoc.getSegmentFraction() * cost;
        //cout <<  "Factor " << cutOffLoc.getSegmentFraction()<<endl;
    }
    else
    {
        //cout << "cutting normal oriented linestring" << endl;

        //static method
        geos::linearref::LinearLocation cutOffLoc =
            geos::linearref::LengthLocationMap::getLocation(l.get(), cutOff);

       line = unique_ptr<Geometry>( idxLine.extractLine(startLoc, cutOffLoc) );

       cost = cutOffLoc.getSegmentFraction() * cost;
       //cout <<  "Factor " << cutOffLoc.getSegmentFraction()<< endl;
    }

    unique_ptr<MultiLineString> mLine ( DBHELPER::GEO_FACTORY->createMultiLineString(
                                          vector<Geometry*>{ line.get() } )
                                        );
    return mLine;
}
inline const bool
 InternalNet::isPointOnArc(const geos::geom::Coordinate& coords,
                           const geos::geom::Geometry& arc) {

    using namespace geos::geom;

    bool isPointOnLine = false;
    const double tolerance = 0.000001;

    try {
        //GeometryFactory gFac;
        const Geometry* pPtr = dynamic_cast<const Geometry*>( DBHELPER::GEO_FACTORY->createPoint(coords) );

        // Buffer with minimal tolerance
        isPointOnLine = arc.intersects(pPtr->buffer(tolerance));

        return isPointOnLine;
    }
    catch (std::exception& ex) {
        LOGGER::LogError( "Error testing for point on line!" );
        LOGGER::LogError( ex.what() );
        throw ex;
    }
}

//--|Region Geo Utils


//-->Region Network core functions

/** @brief processes barrier geometries, that prevent arcs in the network to be considered in routes
*
*/
void
 InternalNet::processBarriers() {

//    if (!this->NETXPERT_CNFG.BarrierPolyTableName.empty())
//    {
//        LOGGER::LogInfo("Processing barrier polygons from "+NETXPERT_CNFG.BarrierPolyTableName + "..");
//
//        std::unordered_set<extarcid_t> arcIDs = DBHELPER::GetIntersectingArcs(NETXPERT_CNFG.BarrierPolyTableName,
//                                      NETXPERT_CNFG.BarrierPolyGeomColumnName,
//                                      NETXPERT_CNFG.ArcsTableName,
//                                      NETXPERT_CNFG.ArcIDColumnName,
//                                      NETXPERT_CNFG.ArcsGeomColumnName);
//        for (auto& id : arcIDs)
//        {
//            LOGGER::LogDebug("Elim arc: " + std::to_string(id) );
//            this->eliminatedArcs.insert(id);
//        }
//        LOGGER::LogDebug("Got "+ std::to_string(arcIDs.size()) +" intersecting barrier polygons from " + NETXPERT_CNFG.BarrierPolyTableName +".");
//    }
//    if (!this->NETXPERT_CNFG.BarrierLineTableName.empty())
//    {
//        LOGGER::LogInfo("Processing barrier lines from "+NETXPERT_CNFG.BarrierLineTableName + "..");
//
//        std::unordered_set<extarcid_t> arcIDs = DBHELPER::GetIntersectingArcs(NETXPERT_CNFG.BarrierLineTableName,
//                                      NETXPERT_CNFG.BarrierLineGeomColumnName,
//                                      NETXPERT_CNFG.ArcsTableName,
//                                      NETXPERT_CNFG.ArcIDColumnName,
//                                      NETXPERT_CNFG.ArcsGeomColumnName);
//        for (auto& id : arcIDs)
//        {
//            this->eliminatedArcs.insert(id);
//        }
//        LOGGER::LogDebug("Got "+ std::to_string(arcIDs.size()) +" intersecting barrier lines from " + NETXPERT_CNFG.BarrierLineTableName +".");
//    }
//    if (!this->NETXPERT_CNFG.BarrierPointTableName.empty())
//    {
//        LOGGER::LogInfo("Processing barrier points from "+NETXPERT_CNFG.BarrierPointTableName + "..");
//
//        std::unordered_set<extarcid_t> arcIDs = DBHELPER::GetIntersectingArcs(NETXPERT_CNFG.BarrierPointTableName,
//                                      NETXPERT_CNFG.BarrierPointGeomColumnName,
//                                      NETXPERT_CNFG.ArcsTableName,
//                                      NETXPERT_CNFG.ArcIDColumnName,
//                                      NETXPERT_CNFG.ArcsGeomColumnName);
//        for (auto& id : arcIDs)
//        {
//            this->eliminatedArcs.insert(id);
//        }
//        LOGGER::LogDebug("Got "+ std::to_string(arcIDs.size()) +" intersecting barrier points from " + NETXPERT_CNFG.BarrierPointTableName +".");
//    }

    if (!this->NETXPERT_CNFG.BarrierPolyTableName.empty())
    {
        LOGGER::LogInfo("Processing barrier polygons from "+NETXPERT_CNFG.BarrierPolyTableName + "..");

        std::unordered_set<extarcid_t> arcIDs = DBHELPER::GetIntersectingArcs(NETXPERT_CNFG.BarrierPolyTableName,
                                      NETXPERT_CNFG.BarrierPolyGeomColumnName,
                                      NETXPERT_CNFG.ArcsTableName,
                                      NETXPERT_CNFG.ArcIDColumnName,
                                      NETXPERT_CNFG.ArcsGeomColumnName);

        for (auto& id : arcIDs)
        {
//            LOGGER::LogDebug("Elim arc: " + std::to_string(id) );
            LOGGER::LogDebug("Elim arc: " + id );
            this->eliminatedArcs.insert(id);
        }
        LOGGER::LogDebug("Got "+ std::to_string(arcIDs.size()) +" intersecting barrier polygons from " + NETXPERT_CNFG.BarrierPolyTableName +".");

        //TODO
        //additional approach:
        //for each barrier geometry:
        // clip network with polygon geometry for each arc geom that intersect with the barrier geometry

        //get geoms from barriers
        auto barrierGeoms = DBHELPER::GetBarrierGeometriesFromDB(NETXPERT_CNFG.BarrierPolyTableName,
                                      NETXPERT_CNFG.BarrierPolyGeomColumnName);

        for (const auto& barrierGeom : barrierGeoms) {

            for (auto& arcID : arcIDs)  {
                //get line geom from network
                auto lineGeom = DBHELPER::GetArcGeometryFromDB(NETXPERT_CNFG.ArcsTableName,
                                          NETXPERT_CNFG.ArcIDColumnName,
                                          NETXPERT_CNFG.ArcsGeomColumnName,
                                          netxpert::data::ArcIDColumnDataType::Number,
                                          arcID);
                //clipLine
                auto clippedLineGeoms = clipLine(*barrierGeom, *lineGeom);
                //insertNewBarrierNodes()
            }
        }
    }
}

/** @brief reads the network arcs from the input arc data and transforms it to the internal network data.
*
*/
void
 InternalNet::readNetwork(const netxpert::data::InputArcs& arcsTbl,
                                              const bool autoClean,
                                              const bool isDirected)
{
    using namespace netxpert::data;
    using namespace std;

    std::string onewayColName = this->NETXPERT_CNFG.OnewayColumnName;
    bool oneWay = this->NETXPERT_CNFG.IsDirected;

    //size_t m = arcsTbl.size();
    this->g->reserveArc(arcsTbl.size());

    for (InputArcs::const_iterator it = arcsTbl.begin(); it != arcsTbl.end(); ++it)
    {
        InputArc arc = *it;
//        extarcid_t externalArcID = std::stoul( arc.extArcID ) ;
        extarcid_t externalArcID = arc.extArcID;
        // String -> so content of nodeID fields could be string, int or double
        string externalStartNode = arc.extFromNode;
        string externalEndNode = arc.extToNode;
//        auto cost = arc.cost;
//        auto capacity = arc.capacity;
        string oneway = arc.oneway;

        node_t internalStartNode;
        node_t internalEndNode;

        try
        {
            internalStartNode   = this->GetNodeFromOrigID(externalStartNode);
            internalEndNode     = this->GetNodeFromOrigID(externalEndNode);
        }
        catch (exception& ex)
        {
            cout << "readNetwork(): Error getting internal Nodes from nodeMap!" << endl;
            cout << ex.what() << endl;
            throw ex;
        }

        bool isArcOneway = false;
        if (!onewayColName.empty())
        {
            if (arc.oneway == "Y")
                isArcOneway = true;
        }

        //CHECKS!
        //if found in eliminatedArcs go to next one
        //could be eliminated from barriers
        //if (std::find(eliminatedArcs.begin(), eliminatedArcs.end(), externalArcID)!=eliminatedArcs.end())
        //    continue;

        if (externalStartNode == externalEndNode)
        {
//            if (autoClean)
//            {
//                LOGGER::LogInfo("Loop at "+ externalStartNode + " - " + externalEndNode+ " ignored");
//
//                /*ExternalArc ftNode {externalStartNode, externalEndNode};
//                arcLoops.push_back(ftNode);
//                eliminatedArcs.insert(externalArcID);*/
//
//                //eliminatedArcsCount = eliminatedArcsCount + 1;
//                continue; // ignorier einfach diese fehlerhafte Kante
//            }
//            else
//            {
                LOGGER::LogWarning("Loop at "+externalStartNode+ " - " + externalEndNode+ "!");
//            }
        }
        if (oneWay) {
            if (isArcOneway == false) //arc level oneway Y or N
            {
                //double the from-to-pair
                processArc(arc, internalStartNode, internalEndNode);
                processArc(arc, internalEndNode, internalStartNode);
            }
            else //regular addition of directed from-to-pair
            {
                processArc(arc, internalStartNode, internalEndNode);
            }
        }
        else {
            //double the from-to-pair
            processArc(arc, internalStartNode, internalEndNode);
            processArc(arc, internalEndNode, internalStartNode);
        }
    }

    DBHELPER::EliminatedArcs = this->eliminatedArcs;
    /*for (auto& s : DBHELPER::EliminatedArcs)
        cout << s << endl;*/
}

void
 InternalNet::readNodes(const InputArcs& arcsTbl, const InputNodes& nodesTbl,
                        const std::map<std::string, netxpert::data::IntNodeID>& extIntNodeMap) {

    using namespace std;

    //get distinct nodes
    auto distinctNodes = getDistinctOrigNodes(arcsTbl);
    //reserve memory
    this->g->reserveNode(distinctNodes.size());
    //add them to the graph and save original node ID
    for (vector<string>::const_iterator it = distinctNodes.begin(); it != distinctNodes.end(); ++it)
    {
        node_t lemNode = this->g->addNode();
//        (*this->nodeMap)[lemNode] = *it;
//        this->nodeIDMap.insert( make_pair(*it, lemNode) );
        this->RegisterNodeID(*it, lemNode);
    }

    // We have to care for the nodes and their supply values also if they are present
    if (nodesTbl.size() > 0)
    {
        for (InputNodes::const_iterator it = nodesTbl.begin(); it != nodesTbl.end(); it++) {
            node_t internalNode;
            auto extNodeID = it->extNodeID;
            auto nodeSupply = it->nodeSupply;

            //Get internal node ID from dictionary:
            //add values to NodeSupply
            //throws an exception if not found!
            try {
                internalNode = this->nodeIDMap.at(extNodeID);
            }
            catch (exception& ex)
            {
                LOGGER::LogError("Original node ID " +extNodeID +" from nodes table not found in arcs!");
                LOGGER::LogError("-->Node ID "+ extNodeID + " will be ignored.");
                continue;
                //throw ex;
            }
            try {
//                std::cout <<"adding "<< extNodeID << ", " << nodeSupply << std::endl;
                (*this->nodeSupplyMap)[internalNode] = nodeSupply;
                // populate mapping for ext/int Nodes
//                extIntNodeMap.insert( std::make_pair(extNodeID, this->GetNodeID(internalNode) ) );

                //filter out transshipment nodes -> they're not important here.
                //if (nodeSupply != 0)
                //{
//                    NodeSupply sVal {extNodeID, nodeSupply};
//                    nodeSupplies.insert( make_pair(internalNodeID, sVal) );
                //}
            }
            catch (exception& ex)
            {
                LOGGER::LogWarning("readNodes(): Error inserting supply values!");
                LOGGER::LogWarning(ex.what());
            }
        }
    }
}

std::vector<std::string>
 InternalNet::getDistinctOrigNodes(const InputArcs& arcsTbl) {

    using namespace std;
    //set is sorted by default
    set<string> sortedDistinctNodesSet;
    for (InputArcs::const_iterator it = arcsTbl.begin(); it != arcsTbl.end(); ++it)
    {
        auto itD = *it;
        //if element is not in set - add it!
        if (sortedDistinctNodesSet.find(itD.extFromNode) == sortedDistinctNodesSet.end() )
        {
            //cout << itD.extFromNode << endl;
            sortedDistinctNodesSet.insert(itD.extFromNode);
        }
        if (sortedDistinctNodesSet.find(itD.extToNode) == sortedDistinctNodesSet.end() )
        {
            //cout << itD.extToNode << endl;
            sortedDistinctNodesSet.insert(itD.extToNode);
        }
    }
    //copy set to vector for index access
    vector<string> sortedDistinctNodes(sortedDistinctNodesSet.begin(), sortedDistinctNodesSet.end());

    return sortedDistinctNodes;
}

void
 InternalNet::processArc(const InputArc& _arc,
                         const node_t internalStartNode,
                         const node_t internalEndNode) {

    auto arc = this->g->addArc(internalStartNode, internalEndNode);

//    this->SetArcData(arc, ArcData { std::stoul(_arc.extArcID),
    this->SetArcData(arc, ArcData { _arc.extArcID,
                                        _arc.cost,
                                        _arc.capacity} );
    (*this->arcFilterMap)[arc] = true;
}
//--|Region Network core functions

//-->Region MinCostFlow functions

double
 InternalNet::calcTotalSupply () {

    supply_t supplyValue = 0;
    auto nodeIter = this->GetNodesIter();
    for ( ; nodeIter != lemon::INVALID; ++nodeIter) {
        auto supply = (*this->nodeSupplyMap)[nodeIter];
        if (supply > 0)
            supplyValue += supply;
    }
//    std::cout << supplyValue << std::endl;
    return supplyValue;
}

double
 InternalNet::calcTotalDemand () {

    supply_t demandValue = 0;
    auto nodeIter = this->GetNodesIter();
    for ( ; nodeIter != lemon::INVALID; ++nodeIter) {
        auto demand = (*this->nodeSupplyMap)[nodeIter];
        if (demand < 0)
            demandValue += demand;
    }
//    std::cout << demandValue << std::endl;
    return abs(demandValue);
}

MinCostFlowInstanceType
 InternalNet::GetMinCostFlowInstanceType() {

    using namespace std;

    supply_t totalSupply = calcTotalSupply();
    supply_t totalDemand = calcTotalDemand();

    LOGGER::LogDebug("Supply - Demand: " + to_string( totalSupply ) + " - " + to_string(totalDemand));

    if (totalSupply > totalDemand)
        return MinCostFlowInstanceType::MCFExtrasupply;
    if (totalSupply < totalDemand)
        return MinCostFlowInstanceType::MCFExtrademand;
    else
        return MinCostFlowInstanceType::MCFBalanced;
}

// Arbeiten mit Dummy Nodes; Ziel ist die Ausgewogene Verteilung von Angebot und Nachfrage
void
 InternalNet::TransformUnbalancedMCF(MinCostFlowInstanceType mcfInstanceType) {
    switch (mcfInstanceType)
    {
        case MinCostFlowInstanceType::MCFExtrasupply:
            transformExtraSupply();
            break;
        case MinCostFlowInstanceType::MCFExtrademand:
            transformExtraDemand();
            break;
        case MinCostFlowInstanceType::MCFBalanced: //Nothing to do
            break;
        default: //MCFUndefined
            break;
    }
}

void
 InternalNet::transformExtraDemand() {
    // Dummy Angebotsknoten mit überschüssiger Nachfrage hinzufügen
    // Dummy-Kosten (0 km) in Netzwerk hinzufügen
    // Reicht es, wenn der neue Dummy-Knoten von allen Nicht-Transshipment-Knoten (=! 0) erreichbar ist?
    // --> In Networkx schon

    //OK - Funktion behandelt Angebotsüberschuss oder Nachfrageüberschuss
    // siehe getSupplyDemandDifference()
    processSupplyOrDemand();
}

void InternalNet::transformExtraSupply() {
    // Dummy Nachfrageknoten mit überschüssigem Angebot hinzufügen
    // Dummy-Kosten (0 km) in Netzwerk hinzufügen
    // Reicht es, wenn der neue Dummy-Knoten von allen Nicht-Transshipment-Knoten (=! 0) erreichbar ist?

    //OK - Funktion behandelt Angebotsüberschuss oder Nachfrageüberschuss
    // siehe getSupplyDemandDifference()
    processSupplyOrDemand();
}

void
 InternalNet::processSupplyOrDemand() {
    // Dummy Knoten
    /*uint32_t newNodeID = GetNodeCount() + 1;
    internalDistinctNodeIDs.insert(make_pair("dummy", newNodeID));
    swappedInternalDistinctNodeIDs.insert( make_pair( newNodeID, "dummy" ));*/

    //New dummy node
    auto newNode = this->g->addNode();
    //std::cout << "Ausgleich: " << getSupplyDemandDifference() << std::endl;
    auto diff = getSupplyDemandDifference();

    //NodeSupply sup {"dummy", diff }; //Differenz ist positiv oder negativ, je nach Überschuss
    //nodeSupplies.insert( make_pair( newNodeID, sup));
    (*this->nodeSupplyMap)[newNode] = diff;

    // Dummy-Kosten (0 km) in Netzwerk hinzufügen
    const cost_t cost = 0;
    const capacity_t capacity = DOUBLE_INFINITY;
    auto nodeIter = this->GetNodesIter();
    for (; nodeIter != lemon::INVALID; ++nodeIter) //Filter dummy und transshipment nodes (=0) raus
    {
        auto curNode = nodeIter;
        if (curNode == newNode) //Filter new dummy
            continue;
        if ( (*this->nodeSupplyMap)[curNode] == 0) //Filter transshipment nodes (=0)
            continue;

        arc_t arc;
        if (diff > 0) {
            arc = this->g->addArc(newNode, curNode);
            // enable arc in filters
            (*this->arcFilterMap)[arc] = true;
            LOGGER::LogDebug("Inserted arc from dummy node " + std::to_string(this->g->id(newNode)) +
                    " to " + std::to_string(this->g->id(curNode)) +
                    " with cost " + std::to_string(cost) +
                    " and cap " + std::to_string(capacity));
        }
        else {
            arc = this->g->addArc(curNode, newNode);
            // enable arc in filters
            (*this->arcFilterMap)[arc] = true;
            LOGGER::LogDebug("Inserted arc from node " + std::to_string(this->g->id(curNode)) +
                    " to dummy node " + std::to_string(this->g->id(newNode)) +
                    " with cost " + std::to_string(cost) +
                    " and cap " + std::to_string(capacity));
        }

        ArcData arcData  {std::to_string(this->g->id(arc)), cost, capacity };
        this->SetArcData(arc, arcData);
    }
    // add dummy node
    this->RegisterNodeID("dummy", newNode);
    (*this->nodeSupplyMap)[newNode] = diff;
    LOGGER::LogDebug("Inserted dummy node (" + std::to_string(this->g->id(newNode)) + ") with supply of "+ std::to_string(diff) );
}

double
 InternalNet::getSupplyDemandDifference() {
    //can be negative or positive
    return (calcTotalDemand() - calcTotalSupply());
}


//--|Region MinCostFlow functions
