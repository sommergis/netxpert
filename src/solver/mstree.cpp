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

#include "mstree.h"

using namespace std;
using namespace netxpert;
using namespace netxpert::cnfg;
using namespace netxpert::data;
using namespace netxpert::io;
using namespace netxpert::core;
using namespace netxpert::utils;

MinimumSpanningTree::MinimumSpanningTree(Config& cnfg)
{
    //ctor
    LOGGER::LogInfo("MinimumSpanningTree Solver instantiated");
    algorithm = cnfg.MstAlgorithm;
    this->NETXPERT_CNFG = cnfg;
}

const MSTAlgorithm
 MinimumSpanningTree::GetAlgorithm() const {
    return algorithm;
}

void
 MinimumSpanningTree::SetAlgorithm(MSTAlgorithm mstAlgorithm) {
    algorithm = mstAlgorithm;
}

const double
 MinimumSpanningTree::GetOptimum() const {
    return mst->GetOptimum();
}

void
 MinimumSpanningTree::SaveResults(const std::string& resultTableName,
                                  const ColumnMap& cmap) const {
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
                writer->CreateSolverResultTable(resultTableName, NetXpertSolver::MinSpanningTreeSolver, true);
                writer->CommitCurrentTransaction();
                /*if (cnfg.GeometryHandling != GEOMETRY_HANDLING::RealGeometry)
                {*/
                auto& sldbWriter = dynamic_cast<SpatiaLiteWriter&>(*writer);
                qry = unique_ptr<SQLite::Statement> (sldbWriter.PrepareSaveResultArc(resultTableName, NetXpertSolver::MinSpanningTreeSolver));
                //}
            }
                break;
            case RESULT_DB_TYPE::ESRI_FileGDB:
            {
                writer = unique_ptr<DBWriter> (new FGDBWriter(cnfg)) ;
                writer->CreateNetXpertDB();
                writer->OpenNewTransaction();
                writer->CreateSolverResultTable(resultTableName, NetXpertSolver::MinSpanningTreeSolver, true);
                writer->CommitCurrentTransaction();
            }
                break;
        }

        LOGGER::LogDebug("Writing Geometries..");
        //Processing and Saving Results are handled within net.ProcessResultArcs()
        std::string arcIDs = "";
        std::unordered_set<string> arcIDlist = this->net->GetOrigArcIDs(this->GetMinimumSpanningTree());

        for (string id : arcIDlist)
        {
            // 13-14 min on 840000 arcs
            //arcIDs = arcIDs + id + ","; //+= is c++ concat operator!
            arcIDs += id += ","; //optimized! 0.2 seconds on 840000 arcs
        }
        arcIDs.pop_back(); //trim last comma

        this->net->ProcessResultArcs(arcIDs, resultTableName);

        LOGGER::LogDebug("Done!");
    }
    catch (exception& ex)
    {
        LOGGER::LogError("MinimumSpanningTree::SaveResults() - Unexpected Error!");
        LOGGER::LogError(ex.what());
    }
}

void
 MinimumSpanningTree::Solve(string net) {
  throw;
}

void
 MinimumSpanningTree::Solve(netxpert::data::InternalNet& net) {
    this->net = &net;
    solve(net);
}

std::vector<netxpert::data::arc_t>
 MinimumSpanningTree::GetMinimumSpanningTree() const {
    return minimumSpanTree;
}

void
 MinimumSpanningTree::solve (netxpert::data::InternalNet& net) {

    vector<netxpert::data::arc_t> result;

    try {
        switch (algorithm)
        {
            case MSTAlgorithm::Kruskal_LEMON:
                mst = unique_ptr<IMinSpanTree>(new MST_LEM());
                break;
            default:
                mst = unique_ptr<IMinSpanTree>(new MST_LEM());
                break;
        }
    }
    catch (exception& ex)
    {
        /*if (ex.GetInnermostException() is DllNotFoundException)
        {
            DllNotFoundException dllEx = (DllNotFoundException)ex.GetInnermostException();
            Logger.WriteLog(string.Format("Fehler beim Instanziieren des MST-Solvers (Kern): {0}", dllEx.Message), LogLevel.Fatal);
            throw dllEx;
        }
        else
        {
            Logger.WriteLog(string.Format("Fehler beim Instanziieren des MST-Solvers (Kern): {0}", ex.InnerException), LogLevel.Fatal);
            Logger.WriteLog(string.Format("StackTrace: {0}", ex.StackTrace), LogLevel.Fatal);
            throw ex;
        }*/
    }

    if (!validateNetworkData( net ))
        //throw new InvalidValueException(string.Format("Problem data does not fit the {0} Solver!", this.ToString()));
        throw;

    LOGGER::LogDebug("Arcs: " + to_string(net.GetArcCount() ));
    LOGGER::LogDebug("Nodes: "+ to_string(net.GetNodeCount() ));
    LOGGER::LogDebug("Solving..");

    //Read the network
    auto sg = convertInternalNetworkToSolverData(net);
    mst->LoadNet(net.GetNodeCount(), net.GetArcCount(), &sg, net.GetCostMap());

    mst->SolveMST();

    result = mst->GetMST();

    this->minimumSpanTree = result;

    LOGGER::LogDebug("Number of MST Arcs: "+ to_string(result.size()));
}

lemon::FilterArcs<netxpert::data::graph_t, netxpert::data::graph_t::ArcMap<bool>>
 MinimumSpanningTree::convertInternalNetworkToSolverData(InternalNet& net
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
 MinimumSpanningTree::validateNetworkData(netxpert::data::InternalNet& net) {
    bool valid = false;


    valid = true;
    return valid;
}
