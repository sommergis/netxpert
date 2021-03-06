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

#include "mcflow.hpp"

using namespace std;
using namespace netxpert;
using namespace netxpert::cnfg;
using namespace netxpert::data;
using namespace netxpert::io;
using namespace netxpert::core;
using namespace netxpert::utils;

MinCostFlow::MinCostFlow(Config& cnfg)
{
    //ctor
    LOGGER::LogInfo("MinimumCostFlow Solver instantiated");
    NETXPERT_CNFG = cnfg;
    algorithm = cnfg.McfAlgorithm;
    solverStatus = MCFSolverStatus::MCFUnSolved;
    IsDirected = true; //TODO CHECK always true?
}

void
 MinCostFlow::Solve(std::string net){}

void
 MinCostFlow::Solve(netxpert::data::InternalNet& net) {
    //TODO: Check for leaks
    this->net = &net;

    //Check Balancing of Min Cost Flow Instance
    auto type = net.GetMinCostFlowInstanceType();
    LOGGER::LogInfo("Type of Min Cost Flow Instance: "+ to_string(type));

    //And transform the instance if needed
    net.TransformUnbalancedMCF(type);
    LOGGER::LogInfo("Transformed Min Cost Flow Problem done.");

    this->net->ExportToDIMACS("./mcfp.dmx");

    try {
        solve(net);
    }
    catch (exception& ex)
    {
        LOGGER::LogError("Exception solving MCF Problem.");
    }
}

const std::string
 MinCostFlow::GetResultsAsJSON() {

  LOGGER::LogDebug("Entering GetResultsAsJSON()..");
  std::ostringstream outStream;
  //header for json
  outStream << "{ \"result\" : [ " << endl;

  //Processing and Saving Results are handled within net.ProcessResultArcs()
  std::vector<netxpert::data::FlowCost>::const_iterator it; //const_iterator wegen Zugriff auf this->flowCost

  if (NETXPERT_CNFG.GeometryHandling == GEOMETRY_HANDLING::RealGeometry)
  {
//    //check if already loaded to mem through SaveResults()
//    if (DBHELPER::KV_Network.size() < 1) {
       std::string arcIDs = processTotalArcIDs();

      LOGGER::LogDebug("Preloading relevant geometries into Memory..");

      ColumnMap cmap { NETXPERT_CNFG.ArcIDColumnName, NETXPERT_CNFG.FromNodeColumnName, NETXPERT_CNFG.ToNodeColumnName,
                      NETXPERT_CNFG.CostColumnName, NETXPERT_CNFG.CapColumnName, NETXPERT_CNFG.OnewayColumnName};

      if (arcIDs.size() > 0) {
        DBHELPER::LoadGeometryToMem(NETXPERT_CNFG.ArcsTableName, cmap, NETXPERT_CNFG.ArcsGeomColumnName, arcIDs);
      }
      LOGGER::LogDebug("Done!");
//    }
  }

  int counter = 0;

  #pragma omp parallel shared(counter) private(it) num_threads(LOCAL_NUM_THREADS)
  {
  for (it = this->flowCost.begin(); it != this->flowCost.end(); ++it)
  {
    #pragma omp single nowait
    {
    auto arcFlow = *it;

    counter += 1;
    if (counter % 2500 == 0)
        LOGGER::LogInfo("Processed #" + to_string(counter) + " geometries.");

    std::string arcIDs  = "";
    auto arc            = arcFlow.intArc;
    cost_t cost         = arcFlow.cost;
    flow_t flow         = arcFlow.flow;
    capacity_t cap      = -1;

    //works only on non-splitted arcs - the rest of the route parts will
    //be added through InternalNet::addRouteGeomParts()
    const netxpert::data::ArcData arcData = this->net->GetArcData(arc);

    arcIDs  = arcData.extArcID;
    cap     = arcData.capacity;

    string orig = this->net->GetOrigNodeID(this->net->GetSourceNode(arc));
    string dest = this->net->GetOrigNodeID(this->net->GetTargetNode(arc));

    std::vector<netxpert::data::arc_t> arcs {arc};

    if (orig != "dummy" && dest != "dummy")
      this->net->ProcessMCFResultArcsMemS(orig, dest, cost, cap, flow, arcIDs,
                                                arcs, outStream);
    else
      LOGGER::LogInfo("Dummy! orig: "+orig+", dest: "+ dest+", cost: "+to_string(cost)+ ", cap: "+
                                        to_string(cap) + ", flow: " +to_string(flow));

    if (counter < this->flowCost.size())
      outStream << ",";

    } //omp single nowait
  }
  } //omp parallel

  outStream << " ] }" << endl;

  return outStream.str();
}

std::string
 MinCostFlow::processTotalArcIDs() {

  std::string arcIDs = "";
  std::unordered_set<string> totalArcIDs;
  std::vector<netxpert::data::FlowCost>::const_iterator it; //const_iterator wegen Zugriff auf this->flowCost

  #pragma omp parallel default(shared) private(it) num_threads(LOCAL_NUM_THREADS)
  {
    //populate arcIDs
    for (it = this->flowCost.begin(); it != this->flowCost.end(); ++it)
    {
      #pragma omp single nowait
      {
        auto arcFlow = *it;
        auto arc = arcFlow.intArc;
  //    double cost = arcFlow.cost;
  //    double flow = arcFlow.flow;
        //only one arc
        auto arcIDlist = this->net->GetOrigArcIDs(std::vector<arc_t>{arc});
        std::unordered_set<string>::const_iterator it;
        it = arcIDlist.begin();

        if (arcIDlist.size() > 0)
        {
          std::string id = *it;

          #pragma omp critical
          {
            if (id != "dummy")
              totalArcIDs.insert(id);
          }
        }
      }//omp single
    }//for
  }//omp parallel

  for (std::string id : totalArcIDs) {
    arcIDs += id += ",";
  }
  if (arcIDs.size() > 0)
    arcIDs.pop_back();

  return arcIDs;
}

void
 MinCostFlow::SaveResults(const std::string& resultTableName,
                          const ColumnMap& cmap) {
  try
  {
    Config cnfg = this->NETXPERT_CNFG;
    unique_ptr<DBWriter> writer;
    unique_ptr<SQLite::Statement> qry; //is null in case of ESRI FileGDB
    std::ofstream outfile; // maybe unused - just for JSON or Google Polyline
    std::ostringstream outStream; // maybe unused - just for JSON or Google Polyline

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
        writer->CreateSolverResultTable(resultTableName, NetXpertSolver::MinCostFlowSolver, true);
        writer->CommitCurrentTransaction();
        /*if (cnfg.GeometryHandling != GEOMETRY_HANDLING::RealGeometry)
        {*/
        auto& sldbWriter = dynamic_cast<SpatiaLiteWriter&>(*writer);
        qry = unique_ptr<SQLite::Statement> (sldbWriter.PrepareSaveResultArc(resultTableName, NetXpertSolver::MinCostFlowSolver));
        //}
      }
      break;
      case RESULT_DB_TYPE::ESRI_FileGDB:
      {
        writer = unique_ptr<DBWriter> (new FGDBWriter(cnfg)) ;
        writer->CreateNetXpertDB();
        writer->OpenNewTransaction();
        writer->CreateSolverResultTable(resultTableName, NetXpertSolver::MinCostFlowSolver, true);
        writer->CommitCurrentTransaction();
      }
      break;
      case RESULT_DB_TYPE::JSON:
      {
        //init file
        LOGGER::LogDebug("Writing Geometries..");
        outfile.open(cnfg.ResultDBPath.c_str(), ios::out | ios::app);
        //header for json
        outfile << "{ \"result\" : [ " << endl;
      }
      break;
    }

    if (cnfg.ResultDBType == RESULT_DB_TYPE::ESRI_FileGDB | cnfg.ResultDBType == RESULT_DB_TYPE::SpatiaLiteDB) {
      LOGGER::LogDebug("Writing Geometries..");
      writer->OpenNewTransaction();
    }

    //Processing and Saving Results are handled within net.ProcessResultArcs()
    std::string arcIDs = "";
    std::vector<FlowCost>::const_iterator it;

    if (cnfg.GeometryHandling == GEOMETRY_HANDLING::RealGeometry)
    {
      arcIDs = processTotalArcIDs();

      LOGGER::LogDebug("Preloading relevant geometries into Memory..");

      if (arcIDs.size() > 0) {
        DBHELPER::LoadGeometryToMem(cnfg.ArcsTableName, cmap, cnfg.ArcsGeomColumnName, arcIDs);
      }
      LOGGER::LogDebug("Done!");
    }

    int counter = 0;

    #pragma omp parallel shared(counter) private(it) num_threads(LOCAL_NUM_THREADS)
    {
    for (it = this->flowCost.begin(); it != this->flowCost.end(); ++it)
    {
      #pragma omp single nowait
      {
      auto arcFlow = *it;

      counter += 1;
      if (counter % 2500 == 0)
          LOGGER::LogInfo("Processed #" + to_string(counter) + " geometries.");

      std::string arcIDs  = "";
      auto arc            = arcFlow.intArc;
      cost_t cost         = arcFlow.cost;
      flow_t flow         = arcFlow.flow;
      capacity_t cap      = -1;

      //works only on non-splitted arcs - the rest of the route parts will
      //be added through InternalNet::addRouteGeomParts()
      const netxpert::data::ArcData arcData = this->net->GetArcData(arc);

      arcIDs  = arcData.extArcID;
      cap     = arcData.capacity;

      string orig = this->net->GetOrigNodeID(this->net->GetSourceNode(arc));
      string dest = this->net->GetOrigNodeID(this->net->GetTargetNode(arc));

      std::vector<netxpert::data::arc_t> arcs {arc};

      if (orig != "dummy" && dest != "dummy")
          this->net->ProcessMCFResultArcsMem(orig, dest, cost, cap, flow, arcIDs,
                                                  arcs,
                                                  resultTableName, *writer, *qry);
      else
          LOGGER::LogInfo("Dummy! orig: "+orig+", dest: "+ dest+", cost: "+to_string(cost)+ ", cap: "+
                                          to_string(cap) + ", flow: " +to_string(flow));
      }//omp single
    } //for
    }//omp paralell

    if (cnfg.ResultDBType == RESULT_DB_TYPE::ESRI_FileGDB | cnfg.ResultDBType == RESULT_DB_TYPE::SpatiaLiteDB) {
      LOGGER::LogDebug("Committing..");
      writer->CommitCurrentTransaction();
      writer->CloseConnection();
      LOGGER::LogDebug("Done!");
    }
    if (cnfg.ResultDBType == RESULT_DB_TYPE::JSON) {
      LOGGER::LogDebug("Writing to disk..");
      outfile << " ] }" << endl;
      outfile.flush();
      outfile.close();
      LOGGER::LogDebug("Done!");
    }
  }//try
  catch (exception& ex)
  {
      LOGGER::LogError("MinCostFlow::SaveResults() - Unexpected Error!");
      LOGGER::LogError(ex.what());
  }
}

const MCFAlgorithm
 MinCostFlow::GetAlgorithm() const {
    return algorithm;
}

void
 MinCostFlow::SetAlgorithm(MCFAlgorithm mcfAlgorithm) {
    algorithm = mcfAlgorithm;
}

inline const double
 MinCostFlow::GetOptimum() const {
    return optimum;
}

std::vector<FlowCost>
 MinCostFlow::GetMinCostFlow() const {
    return this->flowCost;
}

//private
void
 MinCostFlow::solve (netxpert::data::InternalNet& net) {

    vector<FlowCost> result;
    auto* costMap = net.GetCostMap();
    auto* supplyMap = net.GetSupplyMap();

    switch (algorithm)
    {
        case MCFAlgorithm::NetworkSimplex_LEMON:
            mcf = shared_ptr<IMinCostFlow>(new NS_LEM());
            break;
        default:
            mcf = shared_ptr<IMinCostFlow>(new NS_LEM());
            break;
    }

    if (!validateNetworkData( net ))
        throw;

    LOGGER::LogDebug("Arcs: " + to_string(net.GetArcCount() ));
    LOGGER::LogDebug("Nodes: "+ to_string(net.GetNodeCount() ));

    //Read the network
    auto sg = convertInternalNetworkToSolverData(net);
    mcf->LoadNet(net.GetNodeCount(), net.GetArcCount(), &sg, costMap, net.GetCapMap(), supplyMap);

    int srcCount = 0;
    int transshipCount = 0;
    int sinkCount = 0;
    this->getSupplyNodesTypeCount(srcCount, transshipCount, sinkCount);

    LOGGER::LogDebug("Sources: "+ to_string(srcCount));
    LOGGER::LogDebug("Transshipment nodes: "+ to_string(transshipCount));
    LOGGER::LogDebug("Sinks: "+ to_string(sinkCount));
    LOGGER::LogDebug("Solving..");

    mcf->SolveMCF();

    solverStatus = static_cast<netxpert::data::MCFSolverStatus>(mcf->GetMCFStatus());

    LOGGER::LogDebug("MCF Solver Status: " + std::to_string(mcf->GetMCFStatus()));

    if (solverStatus == netxpert::data::MCFSolverStatus::MCFOK) {

        this->optimum = mcf->GetOptimum();

        auto* flowMap = mcf->GetMCFFlow();
//        costMap = mcf->GetMCFCost();

        //loop over all arcs
        auto arcIter = this->net->GetArcsIter();
        for ( ; arcIter != lemon::INVALID; ++arcIter) {
            auto arc = arcIter;
            flow_t flow = (*flowMap)[arc];
            cost_t cost = (*costMap)[arc];
//            auto startNodeId  = this->net->GetNodeID(this->net->GetSourceNode(arc));
//            auto endNodeId    = this->net->GetNodeID(this->net->GetTargetNode(arc));
//            cout << "s: " << startNodeId << " e: " <<endNodeId<<" f: "<<flow << " c: "<<cost << endl;
            if (flow > 0) {
                FlowCost fc{ arc, flow, cost};
                result.push_back(fc);
            }
        }
    }
    else
    {
        string ex = "MCF Solver Status not OK! Solverstatus: " + to_string(solverStatus);
        LOGGER::LogError(ex);
        throw std::runtime_error(ex);
    }
    this->flowCost = result;
}

void
 MinCostFlow::getSupplyNodesTypeCount(int& srcNodeCount, int& transshipNodeCount, int& sinkNodeCount ) {

    auto nodesIter  = this->net->GetNodesIter();
    for (; nodesIter != lemon::INVALID; ++nodesIter) {
        if ( this->net->GetNodeSupply(nodesIter) > 0 )
            srcNodeCount += 1;
        if ( this->net->GetNodeSupply(nodesIter) == 0 )
            transshipNodeCount += 1;
        if ( this->net->GetNodeSupply(nodesIter) < 0 )
            sinkNodeCount += 1;
    }
}


bool
 MinCostFlow::validateNetworkData(netxpert::data::InternalNet& net) {
    bool valid = false;

    /*auto nodesIter = this->net->GetNodesIter();
    auto* supplyMap = this->net->GetSupplyMap();
    for (; nodesIter != lemon::INVALID; ++nodesIter) {
        std::cout << this->net->GetOrigNodeID(nodesIter) << " " << (*supplyMap)[nodesIter] << std::endl;
    }

    auto arcsIter = this->net->GetArcsIter();
    auto* capMap = this->net->GetCapMap();
    auto* costMap = this->net->GetCostMap();
    for (; arcsIter != lemon::INVALID; ++arcsIter) {
        std::cout << this->net->GetNodeID(this->net->GetSourceNode(arcsIter)) << "->"
                  << this->net->GetNodeID(this->net->GetTargetNode(arcsIter)) << ": "
                  << (*costMap)[arcsIter] << ": "
                  << (*capMap)[arcsIter]
                  << std::endl;
    }*/

    valid = true;
    return valid;
}

lemon::FilterArcs<netxpert::data::graph_t, netxpert::data::graph_t::ArcMap<bool>>
 MinCostFlow::convertInternalNetworkToSolverData(netxpert::data::InternalNet& net) {

    using namespace netxpert::data;
    LOGGER::LogInfo("#Arcs internal graph: " +to_string(lemon::countArcs(*net.GetGraph())));

    lemon::FilterArcs<graph_t, graph_t::ArcMap<bool>> sg(*net.GetGraph(), *net.GetArcFilterMap());

    assert(lemon::countArcs(sg) > 0);

    LOGGER::LogInfo("#Arcs filtered graph: " +to_string(lemon::countArcs(sg)));
    return sg;
}
