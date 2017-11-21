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

#include "sptree.h"

using namespace std;
using namespace netxpert;
using namespace netxpert::cnfg;
using namespace netxpert::data;
using namespace netxpert::io;
using namespace netxpert::core;
using namespace netxpert::utils;

ShortestPathTree::ShortestPathTree(Config& cnfg)
{
    //ctor
    LOGGER::LogInfo("ShortestPathTree Solver instantiated");
    algorithm = cnfg.SptAlgorithm;
    isDirected = cnfg.IsDirected;
    sptHeapCard = cnfg.SPTHeapCard;
    geometryHandling = cnfg.GeometryHandling;
    this->NETXPERT_CNFG = cnfg;
}

void
 ShortestPathTree::Solve(std::string net) {
    throw;
}

void
 ShortestPathTree::Solve(netxpert::data::InternalNet& net) {

    LOGGER::LogInfo("Using # " + to_string(LOCAL_NUM_THREADS) + " threads.");

    this->net = &net;

    uint32_t arcCount = net.GetArcCount();
    uint32_t nodeCount = net.GetNodeCount();

    //TODO: check for node supply in network
    /*NodeSupplies ns = net.GetNodeSupplies();
    for (auto kv : ns)
    {
        IntNodeID id = kv.first;
        NodeSupply s = kv.second;
        if (s.supply < 0)
            this->originNode = id;
        if (s.supply > 0)
            this->destinationNodes.push_back(id);
    }*/

    if (destinationNodes.size() == 0)
    {
        //set the size for the heap to m/n (arcs/nodes) if -1
        checkSPTHeapCard(arcCount, nodeCount);
        solve(net, originNode, isDirected);
    }
    else if (destinationNodes.size() == 1)
    {
        //set the size for the heap to m/n (arcs/nodes) if -1
        checkSPTHeapCard(arcCount, nodeCount);
        auto dest = destinationNodes[0];
        solve(net, originNode, dest, isDirected);
    }
    else if (destinationNodes.size() > 1)
    {
        //set the size for the heap to m/n (arcs/nodes) if -1
        checkSPTHeapCard(arcCount, nodeCount);
        solve(net, originNode, destinationNodes, isDirected);
    }
}
void
 ShortestPathTree::checkSPTHeapCard(uint32_t arcCount, uint32_t nodeCount) {

    if (sptHeapCard == -1)
    {
        if (isDirected) {
            sptHeapCard = (int) (arcCount / nodeCount) ;
        }
        else {
            sptHeapCard = (int) (arcCount * 2 / nodeCount );
        }
    }
}


/**
* 1 - all
* The whole shortest path tree is computed from the root node <orig>.
*/
void
 ShortestPathTree::solve (InternalNet& net, netxpert::data::node_t orig, bool isDirected) {

    try
    {
        switch (algorithm)
        {
            /*case SPTAlgorithm::Dijkstra_2Heap_LEMON:
                spt = unique_ptr<ISPTree>(new SPT_LEM(net.GetNodeCount(), net.GetArcCount(),
                                            isDirected));
                break;
            case SPTAlgorithm::Bijkstra_2Heap_LEMON:
                spt = unique_ptr<ISPTree>(new SPT_LEM_Bijkstra_2Heap(net.GetNodeCount(), net.GetArcCount(),
                                        isDirected));
                break;
            case SPTAlgorithm::Dijkstra_dheap_BOOST:
                spt = unique_ptr<ISPTree>(new SPT_BGL_Dijkstra(net.GetNodeCount(), net.GetArcCount(),
                                        isDirected));
                break;*/
            default:
                spt = unique_ptr<ISPTree>(new SPT_LEM(isDirected));
                break;
        }
    }
    catch (exception& ex)
    {

    }

    if (!validateNetworkData( net, orig ))
        throw;

    LOGGER::LogDebug("Arcs: " + to_string(net.GetArcCount() ));
    LOGGER::LogDebug("Nodes: "+ to_string(net.GetNodeCount() ));
    LOGGER::LogDebug("Dests: "+ to_string(net.GetNodeCount() ));
    LOGGER::LogDebug("Solving..");

    //Read the network
    auto sg = convertInternalNetworkToSolverData(net);
    spt->LoadNet(net.GetNodeCount(), net.GetArcCount(), &sg, net.GetCostMap());

    netxpert::data::cost_t totalCost = 0;
    spt->SetOrigin(orig);

    LOGGER::LogDebug("Calculating routes from " + net.GetOrigNodeID(orig) + " to " +
                                "all nodes in the network..");

    spt->SetDest( lemon::INVALID );
    LOGGER::LogDebug("Starting to solve SPT..");
    spt->SolveSPT();
    LOGGER::LogDebug("SPT solved!");

    // Direction is irrelevant - the solver deals with the direction.
    // Thus it's not necessary to pay attention at this when building the route
    // out of the predecessors.
    this->reachedDests.clear();
    this->shortestPaths.clear();

    #pragma omp critical
    {
    // Get all routes from orig to dest in nodes-List
    auto nodesIter = this->net->GetNodesIter();
    //auto* g = this->net->GetGraph();
    //netxpert::data::graph_t::NodeIt nodesIter(*g);
    for ( ; nodesIter != lemon::INVALID; ++nodesIter)
    {
        auto dest = nodesIter;
        //LOGGER::LogDebug(this->net->GetOrigNodeID(dest) + ".." );
        if (orig != dest && spt->Reached(dest))
        {
            const auto costPerRoute = spt->GetDist(dest);
            const auto path = spt->GetPath(dest);
            totalCost = totalCost + costPerRoute;

            //Neuer vector muss sein, wegen clear() Methode weiter unten - sonst werden
            // bei sps auch die Vektoren geleert.
            this->shortestPaths.insert( make_pair( ODPair {orig, dest},
                                  make_pair( path, costPerRoute)) );
            this->reachedDests.push_back(dest);
        }
        if (!spt->Reached(dest)) {
//            LOGGER::LogError("Destination "+ net.GetOrigNodeID(dest) +" unreachable!");
        }
    }
    }//omp critical
    this->optimum = totalCost;
}

/**
* 1 - 1
* The s-t shortest path is computed from the root node <orig> to the destination node <dest>.
*/
void
 ShortestPathTree::solve (InternalNet& net, netxpert::data::node_t orig,
                                netxpert::data::node_t dest, bool isDirected) {
    try
    {
        switch (algorithm)
        {
            /*case SPTAlgorithm::Dijkstra_2Heap_LEMON:
                spt = unique_ptr<ISPTree>(new SPT_LEM(net.GetNodeCount(), net.GetArcCount(),
                                        isDirected));
                break;
            case SPTAlgorithm::Bijkstra_2Heap_LEMON:
                spt = unique_ptr<ISPTree>(new SPT_LEM_Bijkstra_2Heap(net.GetNodeCount(), net.GetArcCount(),
                                        isDirected));
                break;
            case SPTAlgorithm::Dijkstra_dheap_BOOST:
                spt = unique_ptr<ISPTree>(new SPT_BGL_Dijkstra(net.GetNodeCount(), net.GetArcCount(),
                                        isDirected));
                break;*/
            default:
                spt = unique_ptr<ISPTree>(new SPT_LEM(isDirected));
                break;
        }
    }
    catch (exception& ex)
    {

    }

    if (!validateNetworkData( net, orig, dest ))
        //throw new InvalidValueException(string.Format("Problem data does not fit the {0} Solver!", this.ToString()));
        throw;

    LOGGER::LogDebug("Arcs: " + to_string(net.GetArcCount() ));
    LOGGER::LogDebug("Nodes: "+ to_string(net.GetNodeCount() ));
    LOGGER::LogDebug("Solving..");

    netxpert::data::cost_t totalCost = 0;

    //Read the network
    #if (defined ENABLE_CONTRACTION_HIERARCHIES)
    bool chSearch = net.GetHasContractionHierarchies();
    #endif
    if (!chSearch) {
      auto sg = convertInternalNetworkToSolverData(net);

      //check arc filter
      /*assert( lemon::countArcs(sg) > 0);
      for (lemon::FilterArcs<netxpert::data::graph_t, netxpert::data::graph_t::ArcMap<bool>>::ArcIt it( sg ); it != lemon::INVALID; ++it) {
          cout << (*af)[it] << endl;
      }*/

      spt->LoadNet(net.GetNodeCount(), net.GetArcCount(), &sg, net.GetCostMap());

      orig = net.GetNodeFromID(net.GetNodeID(orig));
      dest = net.GetNodeFromID(net.GetNodeID(dest));

      spt->SetOrigin(orig);

      LOGGER::LogDebug("Calculating routes from " + net.GetOrigNodeID(orig) + " to " +
                              net.GetOrigNodeID(dest) );

      spt->SetDest(dest);
      LOGGER::LogDebug("Starting to solve SPT..");

      //germany s-t
      //1,66 sec Bijkstra -O3
      //2,1 sec Dijkstra  -O3
      //1,65 sec Bijkstra -O2
      //2,2 sec Dijkstra  -O2
      bool bidirectional = false;
      if (this->NETXPERT_CNFG.SptAlgorithm == netxpert::cnfg::SPTAlgorithm::Bijkstra_2Heap_LEMON)
          bidirectional = true;

      spt->SolveSPT(-1, bidirectional);
      LOGGER::LogDebug("SPT solved!");
      auto sptPath = spt->GetPath(dest);

      totalCost = spt->GetDist(dest);

//      for (auto& a : sptPath) {
//        std::cout << net.GetNodeID(net.GetSourceNode(a)) << "->" << net.GetNodeID(net.GetTargetNode(a))  << " at "<< net.GetArcCost(a)<< std::endl;
//      }

      shortestPaths.insert( make_pair( ODPair {orig, dest},
                            make_pair( sptPath, totalCost)) );
      reachedDests.push_back(dest);

      optimum = totalCost;

    }
//    else { //CH search

    #if (defined ENABLE_CONTRACTION_HIERARCHIES)
    if (chSearch) {

      auto sg = convertInternalNetworkToSolverData(net);

      spt->LoadNet(net.GetNodeCount(), net.GetArcCount(), &sg, net.GetCostMap());

      auto* spt_ch = dynamic_cast<SPT_LEM*>(spt.get());

  //      spt_ch->LoadNet_CH(net.GetCHManager(), net.GetCostMap_CH(), net.GetNodeMap_CH(), net.GetArcMap_CH());
      spt_ch->LoadNet_CH(net.GetCHManager(), net.GetCostMap_CH(), net.GetNodeMap_CH(), net.GetNodeCrossRefMap_CH());

      spt_ch->SetOrigin(orig);

      LOGGER::LogDebug("Calculating routes from " + net.GetOrigNodeID(orig) + " to " +
                              net.GetOrigNodeID(dest) );
      spt_ch->SetDest(dest);


      LOGGER::LogDebug("Starting to solve SPT with CH..");

      spt_ch->SolveSPT_CH();

      LOGGER::LogDebug("SPT CH solved!");

      const auto chPath = spt_ch->GetPath_CH();

    //      for (auto& b : chPath) {
    //        std::cout << net.GetNodeID(net.GetSourceNode(b)) << "->" << net.GetNodeID(net.GetTargetNode(b)) << " at "<< net.GetArcCost(b)<< std::endl;
    //      }

      auto chTotalCost = spt_ch->GetDist_CH();

      //clear first
      shortestPaths.clear();

      shortestPaths.insert( make_pair( ODPair {orig, dest},
                            make_pair( chPath, chTotalCost)) );
      reachedDests.clear();

      reachedDests.push_back(dest);

      optimum = chTotalCost;
    }
    #endif

    //experimental CH
//    LOGGER::LogDebug("Contraction of network..");
//    auto* ch = dynamic_cast<SPT_LEM*>(spt.get());
//    std::string gName = "test";
//    float contractionPercent = 97;
//    ch->ComputeContraction(contractionPercent);
//    LOGGER::LogDebug("Done!");
//
//    LOGGER::LogDebug("Export of contracted network..");
//    ch->ExportContractedNetwork(gName, this->net->GetNodeMap());
//    LOGGER::LogDebug("Done!");
//
//    LOGGER::LogDebug("Import of contracted network..");
//    ch->ImportContractedNetwork(gName, this->net->GetNodeIDMap());
//    LOGGER::LogDebug("Done!");
//
//    LOGGER::LogDebug("Starting to solve SPT with CH..");
//    ch->SolveSPT_CH();
//    LOGGER::LogDebug("SPT CH solved!");
//    auto chPath = ch->GetPath_CH();

    // Direction is irrelevant - the solver deals with the direction.
    // Thus it's not necessary to pay attention at this when building the route
    // out of the predecessors.


//    reachedDests.clear();
//    shortestPaths.clear();
//
//    if (orig != dest && spt->Reached(dest)) {
//
//        totalCost = spt->GetDist(dest);
//        const auto path = spt->GetPath(dest);
//
//        //Neuer vector muss sein, wegen clear() Methode weiter unten - sonst werden
//        // bei sps auch die Vektoren geleert.
//        shortestPaths.insert( make_pair( ODPair {orig, dest},
//                              make_pair( path, totalCost)) );
//        reachedDests.push_back(dest);
//    }
//	if (!spt->Reached(dest)) {
//	    LOGGER::LogError("Destination "+ net.GetOrigNodeID(dest) +" unreachable!");
//	}
//    optimum = totalCost;
}

/**
* 1 - n
* The whole shortest path tree is computed from the root node <orig>. Then the path to all
* destination nodes <dests> is being retrieved. This is normally cheaper than computing each s-t pair
* (even with bidirectional search).
*/
void
 ShortestPathTree::solve (InternalNet& net, netxpert::data::node_t orig,
                                std::vector<netxpert::data::node_t> dests, bool isDirected) {
    try
    {
        switch (algorithm)
        {
            /*case SPTAlgorithm::Dijkstra_2Heap_LEMON:
                spt = unique_ptr<ISPTree>(new SPT_LEM(net.GetNodeCount(), net.GetArcCount(),
                                            isDirected));
                break;
            case SPTAlgorithm::Bijkstra_2Heap_LEMON:
                spt = unique_ptr<ISPTree>(new SPT_LEM_Bijkstra_2Heap(net.GetNodeCount(), net.GetArcCount(),
                                        isDirected));
                break;
            case SPTAlgorithm::Dijkstra_dheap_BOOST:
                spt = unique_ptr<ISPTree>(new SPT_BGL_Dijkstra(net.GetNodeCount(), net.GetArcCount(),
                                        isDirected));
                break;*/
            default:
                spt = unique_ptr<ISPTree>(new SPT_LEM(isDirected));
                break;
        }
    }
    catch (exception& ex)
    {

    }

    if (!validateNetworkData( net, orig, dests ))
        //throw new InvalidValueException(string.Format("Problem data does not fit the {0} Solver!", this.ToString()));
        throw;

    LOGGER::LogDebug("Arcs: " + to_string(net.GetArcCount() ));
    LOGGER::LogDebug("Nodes: "+ to_string(net.GetNodeCount() ));
    LOGGER::LogDebug("Dests: "+ to_string(dests.size()));
    LOGGER::LogDebug("Solving..");

    //Filter arcs out and read the network
    auto sg = convertInternalNetworkToSolverData(net);
    spt->LoadNet(net.GetNodeCount(), net.GetArcCount(), &sg, net.GetCostMap());

    netxpert::data::cost_t totalCost = 0;
    spt->SetOrigin(orig);

    LOGGER::LogDebug("Calculating routes from " + net.GetOrigNodeID(orig) + " to " +
                                "all nodes in the network..");

    spt->SetDest( lemon::INVALID );
    LOGGER::LogDebug("Starting to solve SPT..");
    spt->SolveSPT();
    LOGGER::LogDebug("SPT solved!");

    // Direction is irrelevant - the solver deals with the direction.
    // Thus it's not necessary to pay attention at this when building the route
    // out of the predecessors.
    reachedDests.clear();
    shortestPaths.clear();

    #pragma omp critical
    {
    // Get all routes from orig to dest in nodes-List
    for (auto dest : dests)
    {
        //LOGGER::LogDebug(to_string(dest) + " of " + to_string(nodes.size()) );
        if (orig != dest && spt->Reached(dest)) {
            const auto costPerRoute = spt->GetDist(dest);
            const auto path = spt->GetPath(dest);
            totalCost = totalCost + costPerRoute;

            //Neuer vector muss sein, wegen clear() Methode weiter unten - sonst werden
            // bei sps auch die Vektoren geleert.
            this->shortestPaths.insert( make_pair( ODPair {orig, dest},
                                   make_pair( path, costPerRoute)) );
            this->reachedDests.push_back(dest);
        }
        if (!spt->Reached(dest)) {
            LOGGER::LogError("Destination "+ net.GetOrigNodeID(dest) +" unreachable!");
        }
    }
    }//omp critical
    optimum = totalCost;
}

SPTAlgorithm
 ShortestPathTree::GetAlgorithm() const
{
    return this->algorithm;
}

void
 ShortestPathTree::SetAlgorithm(SPTAlgorithm sptAlgorithm) {
    this->algorithm = sptAlgorithm;
}

GEOMETRY_HANDLING
 ShortestPathTree::GetGeometryHandling() const {
    return this->geometryHandling;
}
void
 ShortestPathTree::SetGeometryHandling(GEOMETRY_HANDLING geomHandling) {
    this->geometryHandling = geomHandling;
}

int
 ShortestPathTree::GetSPTHeapCard() const {
    return this->sptHeapCard;
}

void
 ShortestPathTree::SetSPTHeapCard(int heapCard) {
    this->sptHeapCard = heapCard;
}

const netxpert::data::node_t
 ShortestPathTree::GetOrigin() const {
    return this->originNode;
}

void
 ShortestPathTree::SetOrigin(const netxpert::data::node_t  orig) {
    this->originNode = orig;
}

vector<netxpert::data::node_t>
 ShortestPathTree::GetDestinations() const {
    return this->destinationNodes;
}

vector<uint32_t>
 ShortestPathTree::GetDestinationIDs() const {
    vector<uint32_t> result;
    for (auto n : this->destinationNodes) {
        result.push_back(this->net->GetNodeID(n));
    }
    return result;
}

void
 ShortestPathTree::SetDestinations(const vector<netxpert::data::node_t >& dests) {
    this->destinationNodes = dests;
}

void
 ShortestPathTree::SetDestinations(const vector<uint32_t >& dests) {
    vector<netxpert::data::node_t> tmp;
    for (auto n : dests) {
        tmp.push_back(this->net->GetNodeFromID(n));
    }
    this->destinationNodes = tmp;
}

vector<netxpert::data::node_t>
 ShortestPathTree::GetReachedDests() const {
    return this->reachedDests;
}

vector<uint32_t>
 ShortestPathTree::GetReachedDestIDs() const {
    vector<uint32_t> result;
    for (auto n : this->reachedDests) {
        result.push_back(this->net->GetNodeID(n));
    }
    return result;
}

map<ODPair, CompressedPath>
 ShortestPathTree::GetShortestPaths() const {
  return this->shortestPaths;
}

inline const double
 ShortestPathTree::GetOptimum() const {
    return this->optimum;
}

void ShortestPathTree::SaveResults(const std::string& resultTableName,
                                   const ColumnMap& cmap,
                                   const std::string& format) const {
  try
  {
    Config cnfg = this->NETXPERT_CNFG;
    unique_ptr<DBWriter> writer;
    unique_ptr<SQLite::Statement> qry; //is null in case of ESRI FileGDB

    switch (cnfg.ResultDBType)
    {
      case RESULT_DB_TYPE::SpatiaLiteDB:
      {
        if (cnfg.ResultDBPath == cnfg.NetXDBPath)
        {
            //Override result DB Path with original netXpert DB path
            writer = unique_ptr<DBWriter>(new SpatiaLiteWriter(cnfg, cnfg.NetXDBPath));
        }
        else
        {
          writer = unique_ptr<DBWriter>(new SpatiaLiteWriter(cnfg));
        }
        writer->CreateNetXpertDB(); //create before preparing query
        writer->OpenNewTransaction();
        writer->CreateSolverResultTable(resultTableName, NetXpertSolver::ShortestPathTreeSolver, true);
        writer->CommitCurrentTransaction();
        /*if (cnfg.GeometryHandling != GEOMETRY_HANDLING::RealGeometry)
        {*/
        auto& sldbWriter = dynamic_cast<SpatiaLiteWriter&>(*writer);
        qry = unique_ptr<SQLite::Statement> (sldbWriter.PrepareSaveResultArc(resultTableName, NetXpertSolver::ShortestPathTreeSolver));
        //}
      }
      break;

      case RESULT_DB_TYPE::ESRI_FileGDB:
      {
          writer = unique_ptr<DBWriter> (new FGDBWriter(cnfg)) ;
          writer->CreateNetXpertDB();
          writer->OpenNewTransaction();
          writer->CreateSolverResultTable(resultTableName, NetXpertSolver::ShortestPathTreeSolver, true);
          writer->CommitCurrentTransaction();
      }
      break;
    }

    LOGGER::LogDebug("Writing Geometries..");
    writer->OpenNewTransaction();

    //Processing and Saving Results are handled within net.ProcessResultArcs()
    std::string arcIDs = "";
    std::unordered_set<string> totalArcIDs;
    std::map<ODPair, CompressedPath>::const_iterator it; //const_iterator wegen Zugriff auf this->shortestPath

		if (cnfg.GeometryHandling == GEOMETRY_HANDLING::RealGeometry)
		{
			LOGGER::LogDebug("Preloading relevant geometries into Memory..");

			#pragma omp parallel default(shared) private(it) num_threads(LOCAL_NUM_THREADS)
			{
				//populate arcIDs
				for (it = this->shortestPaths.begin(); it != this->shortestPaths.end(); ++it)
				{
					#pragma omp single nowait
					{
						auto kv = *it;
//						ODPair key = kv.first;
						CompressedPath value = kv.second;
						/* TODO resolve pred path to arcids */
						/* ArcLookup vs AllArcLookup vs saving the path of the route, not only the preds ?*/
						auto path = value.first;

						std::unordered_set<std::string> arcIDlist = this->net->GetOrigArcIDs(path);

						if (arcIDlist.size() > 0)
						{
							#pragma omp critical
							{
								for (std::string id : arcIDlist){
								  totalArcIDs.insert(id);
								}
							}
						}
					}//omp single
				}
			}//omp parallel

			for (string id : totalArcIDs) {
				arcIDs += id += ",";
            		}

			if (arcIDs.size() > 0)
			{
				arcIDs.pop_back(); //trim last comma
				DBHELPER::LoadGeometryToMem(cnfg.ArcsTableName, cmap, cnfg.ArcsGeomColumnName, arcIDs);
			}
			LOGGER::LogDebug("Done!");
		}

    int counter = 0;

		#pragma omp parallel shared(counter) private(it) num_threads(LOCAL_NUM_THREADS)
    {

    for (it = this->shortestPaths.begin(); it != this->shortestPaths.end(); ++it)
    {
      #pragma omp single nowait
      {
      auto kv = *it;

      counter += 1;
      if (counter % 2500 == 0)
          LOGGER::LogInfo("Processed #" + to_string(counter) + " geometries.");

      string arcIDs = "";
      ODPair key = kv.first;
      CompressedPath value = kv.second;
      /* resolve pred path to arcids */
      /* ArcLookup vs AllArcLookup vs saving the path of the route, not only the preds ?*/
      auto path = value.first;
      std::unordered_set<std::string> arcIDlist = this->net->GetOrigArcIDs(path);
      double costPerPath = value.second;

      if (arcIDlist.size() > 0)
      {
        for (string id : arcIDlist) {
            arcIDs += id += ",";
        }
        arcIDs.pop_back(); //trim last comma
      }

      string orig = this->net->GetOrigNodeID(key.origin);
      string dest = this->net->GetOrigNodeID(key.dest);

      this->net->ProcessSPTResultArcsMem(orig, dest, costPerPath, arcIDs, path, resultTableName, *writer, *qry);
      }//omp single

      }
      }//omp paralell
      LOGGER::LogDebug("Committing..");
      writer->CommitCurrentTransaction();
      writer->CloseConnection();
      LOGGER::LogDebug("Done!");
    }
    catch (exception& ex)
    {
      LOGGER::LogError("ShortestPaths::SaveResults() - Unexpected Error!");
      LOGGER::LogError(ex.what());
    }
}

lemon::FilterArcs<netxpert::data::graph_t, netxpert::data::graph_t::ArcMap<bool>>
 ShortestPathTree::convertInternalNetworkToSolverData(InternalNet& net
                                                            /*lemon::FilterArcs<netxpert::data::graph_t,
                                                                              lemon::IterableBoolMap<netxpert::data::graph_t,
                                                                                                     netxpert::data::arc_t>
                                                                              >& arcFilter //OUT*/
                                                            )
{
    using namespace netxpert::data;
    LOGGER::LogInfo("#Arcs internal graph: " +to_string(lemon::countArcs(*net.GetGraph())));

    lemon::FilterArcs<graph_t, graph_t::ArcMap<bool>> sg(*net.GetGraph(), *net.GetArcFilterMap());

    assert(lemon::countArcs(sg) > 0);

    LOGGER::LogInfo("#Arcs filtered graph: " +to_string(lemon::countArcs(sg)));

//    for (filtered_graph_t::ArcIt it(sg); it != lemon::INVALID; ++it)
//        std::cout << sg.id(sg.source(it)) << "->" << sg.id(sg.target(it)) << " , ";
//
//    std::cout << std::endl;
    return sg;
}

bool
 ShortestPathTree::validateNetworkData(InternalNet& net, netxpert::data::node_t orig) {
    bool valid = false;

    //cout << "Orig: "<<orig << endl;
    //IMPORTANT Checks
    /*if (orig == 0){
        LOGGER::LogFatal("Origin Node ID must be greater than zero!");
        throw;
        //throw new InvalidValueException("Origin Node ID must be greater than zero!");
    }

    if (orig > net.GetNodeCount() ) {
        //throw new InvalidValueException("Origin Node ID must not exceed net.MaxNodeCount!");
        LOGGER::LogFatal("Origin Node ID must not exceed maximum node count of network!");
        throw;
    }*/

    valid = true;
    return valid;
}

bool ShortestPathTree::validateNetworkData(InternalNet& net, netxpert::data::node_t orig, netxpert::data::node_t dest)
{
    bool valid = false;

    //IMPORTANT Checks
    /*if (orig == 0){
        LOGGER::LogFatal("Origin Node ID must be greater than zero!");
        throw;
        //throw new InvalidValueException("Origin Node ID must be greater than zero!");
    }

    if (orig > net.GetNodeCount() ) {
        //throw new InvalidValueException("Origin Node ID must not exceed net.MaxNodeCount!");
        LOGGER::LogFatal("Origin Node ID must not exceed maximum node count of network!");
        throw;
    }
    if (dest > net.GetNodeCount() ) {
            //throw new InvalidValueException("Destination Node ID must not exceed net.MaxNodeCount!");
            LOGGER::LogFatal("Destination Node ID must not exceed maximum node count of network!");
            throw;
    }*/

    valid = true;
    return valid;
}
bool ShortestPathTree::validateNetworkData(InternalNet& net, netxpert::data::node_t orig, std::vector<netxpert::data::node_t>& dests)
{
    bool valid = false;

    //IMPORTANT Checks
    /*if (orig == 0){
        LOGGER::LogFatal("Origin Node ID must be greater than zero!");
        throw;
        //throw new InvalidValueException("Origin Node ID must be greater than zero!");
    }
    if (orig > net.GetNodeCount() ) {
        //throw new InvalidValueException("Origin Node ID must not exceed net.MaxNodeCount!");
        LOGGER::LogFatal("Origin Node ID must not exceed maximum node count of network!");
        throw;
    }

    for (auto dest : dests)
    {
        if (dest > net.GetNodeCount() ) {
            //throw new InvalidValueException("Destination Node ID must not exceed net.MaxNodeCount!");
            LOGGER::LogFatal("Destination Node ID must not exceed maximum node count of network!");
            throw;
        }
    }
    */
    valid = true;
    return valid;
}
