#include "mstree.h"

using namespace netxpert;

MinimumSpanningTree::MinimumSpanningTree(Config& cnfg)
{
    //ctor
    LOGGER::LogInfo("MinimumSpanningTree Solver instantiated");
    algorithm = cnfg.MstAlgorithm;
    this->NETXPERT_CNFG = cnfg;
}

MSTAlgorithm MinimumSpanningTree::GetAlgorithm()
{
    return algorithm;
}
void MinimumSpanningTree::SetAlgorithm(MSTAlgorithm mstAlgorithm)
{
    algorithm = mstAlgorithm;
}
double MinimumSpanningTree::GetOptimum() const
{
    return mst->MSTGetF0();
}
void MinimumSpanningTree::SaveResults(const std::string& resultTableName, const netxpert::ColumnMap& cmap) const
{
    try
    {
        Config cnfg = this->NETXPERT_CNFG;

        unique_ptr<DBWriter> writer;
        unique_ptr<SQLite::Statement> qry;
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
                writer->CloseConnection();
            }
                break;
        }

        LOGGER::LogDebug("Writing Geometries..");

        //Processing and Saving Results are handled within net.ProcessResultArcs()

        string arcIDs;
        unordered_set<string> arcIDlist = this->net->GetOriginalArcIDs(this->GetMinimumSpanningTree(), cnfg.IsDirected);
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

void MinimumSpanningTree::Solve(string net)
{

}

void MinimumSpanningTree::Solve(Network& net)
{
    this->net = std::unique_ptr<Network>( move(&net));
    //TODO CHECK
    solve(*(this->net));
}

vector<InternalArc> MinimumSpanningTree::GetMinimumSpanningTree() const
{
    return minimumSpanTree;
}

void MinimumSpanningTree::solve (Network& net)
{
    //vector<long> ign = { 0, -1, (long)4294967295, (long)4261281277 };
    // 0 is a valid ignore value for linux!
    //TODO check in windows
    vector<long> ign = { -1, (long)4294967295, (long)4261281277 };

    vector<InternalArc> result;

    vector<unsigned int> sNds;
    vector<unsigned int> eNds;
    vector<double> supply;
    vector<double> costs;
    vector<double> caps;

    try
    {
        switch (algorithm)
        {
            case MSTAlgorithm::Kruskal_LEMON:
                mst = shared_ptr<IMinSpanTree>(new MST_LEMON());
                break;
            default:
                mst = shared_ptr<IMinSpanTree>(new MST_LEMON());
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

    LOGGER::LogDebug("Arcs: " + to_string(net.GetMaxArcCount() ));
    LOGGER::LogDebug("Nodes: "+ to_string(net.GetMaxNodeCount() ));
    LOGGER::LogDebug("Solving..");

    //Read the network
    try
    {
        convertInternalNetworkToSolverData(net, sNds, eNds, supply, caps, costs);

        //vector::data() returns direct pointer to data
        mst->LoadNet( static_cast<unsigned int>( net.GetMaxNodeCount() ), static_cast<unsigned int>( net.GetMaxArcCount()),
                        static_cast<unsigned int>( net.GetMaxNodeCount() ),static_cast<unsigned int>( net.GetMaxArcCount() ),
                        caps.data(), costs.data(), supply.data(), sNds.data(), eNds.data());
    }
    catch (exception& ex)
    {
        LOGGER::LogFatal( ex.what() );
        result.clear();
    }

    try
    {
        mst->SolveMST();
    }
    catch (exception& ex)
    {
        //throw new ApplicationException(ex.Message, ex.InnerException);
    }

    unsigned int* mstSNds = new unsigned int [net.GetMaxArcCount()];
    unsigned int* mstENds = new unsigned int [net.GetMaxArcCount()];

    mst->GetMST(mstSNds, mstENds);

    for (int i = 0; i < mst->MCFmmax(); i++)
    {
        try
        {
            unsigned int source = mstSNds[i];
            unsigned int target = mstENds[i];
            if (source != ign[0] && source != ign[1] && source != ign[2] &&
                target != ign[0] && target != ign[1] && target != ign[2])
            {
                result.push_back( {source, target} );
            }
        }
        catch (exception& ex)
        {
            LOGGER::LogDebug("mmax: " +to_string( mst->MCFmmax() ) +" i: "+ to_string(i));
        }
    }
    delete[] mstSNds;
    delete[] mstENds;

    this->minimumSpanTree = result;

}
void MinimumSpanningTree::convertInternalNetworkToSolverData(Network& net, vector<unsigned int>& sNds,
            vector<unsigned int>& eNds, vector<double>& supply, vector<double>& caps, vector<double>& costs)
{
    // Die Größe der Arrays müssen passen (ob 0 drin steht ist egal, sonst gibts später bei Dispose
    // das böse Erwachen in Form eines Heap Corruption errors bzw. einer System Access Violation

    Arcs arcs = net.GetInternalArcData();
    vector<InternalArc> keys;
    for(Arcs::iterator it = arcs.begin(); it != arcs.end(); ++it) {
      keys.push_back(it->first);
    }

    // Für Min Span Tree ist eine Richtung ausreichend, weil eh nur die Kante an sich betrachtet wird
    // --> kein doppelter Input der Kanten notwendig
    sNds.resize(keys.size());
    eNds.resize(keys.size());
    //cout << "size of arcs: " << keys.size() << endl;
    for (int i = 0; i < keys.size(); i++)
    {
        sNds[i] = keys[i].fromNode;
        eNds[i] = keys[i].toNode;
    }

    costs.resize(keys.size()); //Größe muss passen!
    caps.resize(keys.size());  //Größe muss passen!
    for (int i = 0; i < keys.size(); i++)
    {
        ArcData oldArcData;
        if (arcs.count(keys[i]) > 0)
            oldArcData = arcs.at(keys[i]);
        costs[i] = oldArcData.cost;
        //Min span tree does not care about capacity
        caps[i] = 0; //oldArcData.capacity;
    }

    //Min span tree does not care about capacity
    /*
    supply.resize( net.GetMaxNodeCount() ); //Größe muss passen!
    for (auto item : net.GetNodeSupplies() )
    {
        unsigned int key = item.first;
        NodeSupply value = item.second;
        // key is 1-based thus -1 for index
        // only care for real supply and demand values
        // transshipment nodes (=0) are already present in the array (because of new array)
        supply[key - 1] = value.supply;
    }*/
}

bool MinimumSpanningTree::validateNetworkData(Network& net)
{
    bool valid = false;


    valid = true;
    return valid;
}
