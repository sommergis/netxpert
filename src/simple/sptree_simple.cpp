#include "sptree_simple.h"

netxpert::simple::ShortestPathTree::ShortestPathTree(std::string jsonCnfg)
{
    //Convert JSON Config to real Config Object
    NETXPERT_CNFG = netxpert::UTILS::DeserializeJSONtoObject<netxpert::Config>(jsonCnfg);
}

int netxpert::simple::ShortestPathTree::Solve()
{
    using namespace netxpert; //local scope!

    try
    {
        Config cnfg = NETXPERT_CNFG;

        //1. Config
        if (!DBHELPER::IsInitialized)
        {
            DBHELPER::Initialize(cnfg);
        }

        try
        {
            if (!LOGGER::IsInitialized)
            {
                LOGGER::Initialize(cnfg);
            }
        }
        catch (exception& ex)
        {
            cout << "Error creating log file: " + cnfg.LogFileFullPath << endl;
            cout << ex.what() << endl;
        }

        InputArcs arcsTable;
        vector<NewNode> nodesTable;
        string arcsGeomColumnName = cnfg.ArcsGeomColumnName;

        string pathToSpatiaLiteDB = cnfg.NetXDBPath;
        string arcsTableName = cnfg.ArcsTableName;

        string nodesTableName = cnfg.NodesTableName;
        string nodesGeomColName = cnfg.NodesGeomColumnName;

        string resultTableName = cnfg.ResultTableName.empty() ? cnfg.ArcsTableName + "_spt" : cnfg.ResultTableName;
        bool autoCleanNetwork = cnfg.CleanNetwork;

        ColumnMap cmap { cnfg.ArcIDColumnName, cnfg.FromNodeColumnName, cnfg.ToNodeColumnName,
                        cnfg.CostColumnName, cnfg.CapColumnName, cnfg.OnewayColumnName,
                        cnfg.NodeIDColumnName, cnfg.NodeSupplyColumnName };

        bool withCapacity = false;
        if (!cnfg.CapColumnName.empty())
            withCapacity = true;

		LOGGER::LogInfo("Using # " + to_string(LOCAL_NUM_THREADS) + " threads.");

        //2. Load Network
        DBHELPER::OpenNewTransaction();
        LOGGER::LogInfo("Loading Data from DB..!");
        arcsTable = DBHELPER::LoadNetworkFromDB(arcsTableName, cmap);
        nodesTable = DBHELPER::LoadNodesFromDB(nodesTableName, cnfg.NodesGeomColumnName, cmap);

        LOGGER::LogInfo("Done!");
        Network net (arcsTable, cmap, cnfg);

        LOGGER::LogInfo("Converting Data into internal network..");
        net.ConvertInputNetwork(autoCleanNetwork);
        LOGGER::LogInfo("Done!");

        LOGGER::LogInfo("Loading Start nodes..");
        vector<pair<unsigned int, string>> startNodes = net.LoadStartNodes(nodesTable, cnfg.Treshold, arcsTableName,
                                                                        cnfg.ArcsGeomColumnName, cmap, withCapacity);
        vector<pair<unsigned int, string>> endNodes;
        if (!cnfg.SPTAllDests) {
            LOGGER::LogInfo("Loading End nodes..");
            endNodes = net.LoadEndNodes(nodesTable, cnfg.Treshold, arcsTableName,
                                                                        cnfg.ArcsGeomColumnName, cmap, withCapacity);
        }

        DBHELPER::CommitCurrentTransaction();
        DBHELPER::CloseConnection();

        // Solver
        solver = unique_ptr<netxpert::ShortestPathTree> (new netxpert::ShortestPathTree(cnfg));
        auto& spt = *solver;

        spt.SetOrigin(startNodes.at(0).first);
        vector<unsigned int> dests = {};// newEndNodeID, newEndNodeID2}; //newEndNodeID}; // {}

        if (!cnfg.SPTAllDests)
        {
            for (auto d : endNodes)
                dests.push_back(d.first);
        }
        spt.SetDestinations( dests );

        spt.Solve(net);

        auto kvSPS = spt.GetShortestPaths();

        LOGGER::LogInfo("Optimum: " + to_string(spt.GetOptimum()) );
        LOGGER::LogInfo("Count of SPT: " +to_string( kvSPS.size() ) );

        unique_ptr<DBWriter> writer;
		unique_ptr<SQLite::Statement> qry; //is null in case of ESRI FileGDB
        switch (cnfg.ResultDBType)
        {
            case RESULT_DB_TYPE::SpatiaLiteDB:
            {
                if (NETXPERT_CNFG.ResultDBPath == NETXPERT_CNFG.NetXDBPath)
                {
                    //Override result DB Path with original netXpert DB path
                    writer = unique_ptr<DBWriter>(new SpatiaLiteWriter(cnfg, NETXPERT_CNFG.NetXDBPath));
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
                qry = unique_ptr<SQLite::Statement> (sldbWriter.PrepareSaveResultArc(resultTableName));
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

        std::string arcIDs = "";
        std::unordered_set<string> totalArcIDs;
        std::unordered_map<ODPair, CompressedPath>::iterator it;

		if (NETXPERT_CNFG.GeometryHandling == GEOMETRY_HANDLING::RealGeometry)
		{
			LOGGER::LogDebug("Preloading relevant geometries into Memory..");

			#pragma omp parallel default(shared) private(it) num_threads(LOCAL_NUM_THREADS)
			{
				//populate arcIDs
				for (it = kvSPS.begin(); it != kvSPS.end(); ++it)
				{
					#pragma omp single nowait
					{
						auto kv = *it;
						ODPair key = kv.first;
						CompressedPath value = kv.second;
						std::vector<unsigned int> ends = value.first;
						std::vector<InternalArc> route;
						std::unordered_set<std::string> arcIDlist;

						route = spt.UncompressRoute(key.origin, ends);
						arcIDlist = net.GetOriginalArcIDs(route, cnfg.IsDirected);

						if (arcIDlist.size() > 0)
						{
							#pragma omp critical
							{
								for (std::string id : arcIDlist)
								totalArcIDs.insert(id);
							}
						}
					}//omp single
				}
			}//omp parallel

			for (string id : totalArcIDs)
				arcIDs += id += ",";

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

        for (it = kvSPS.begin(); it != kvSPS.end(); ++it)
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
            vector<unsigned int> ends = value.first;
            double costPerPath = value.second;
            vector<InternalArc> route;
            unordered_set<string> arcIDlist;

            route = spt.UncompressRoute(key.origin, ends);
            arcIDlist = net.GetOriginalArcIDs(route, cnfg.IsDirected);

            if (arcIDlist.size() > 0)
            {
                for (string id : arcIDlist)
                    arcIDs += id += ",";
                arcIDs.pop_back(); //trim last comma
            }

            string orig;
            string dest;
            try{
                orig = net.GetOriginalStartOrEndNodeID(key.origin);
            }
            catch (exception& ex) {
                orig = net.GetOriginalNodeID(key.origin);
            }
            try{
                dest = net.GetOriginalStartOrEndNodeID(key.dest);
            }
            catch (exception& ex) {
                dest = net.GetOriginalNodeID(key.dest);
            }
            net.ProcessResultArcsMem(orig, dest, costPerPath, -1, -1, arcIDs, route, resultTableName, *writer, *qry);
            }//omp single
        }
        }//omp paralell

        writer->CommitCurrentTransaction();
        writer->CloseConnection();
        LOGGER::LogDebug("Done!");
        return 0; // OK
    }
    catch (exception& ex)
    {
        LOGGER::LogError("ShortestPathTree_Simple::Solve() - Unexpected Error!");
        LOGGER::LogError(ex.what());
        return 1;
    }
}

double netxpert::simple::ShortestPathTree::GetOptimum()
{
    double result = 0;
    if (this->solver)
        result = this->solver->GetOptimum();
    return result;
}
std::string netxpert::simple::ShortestPathTree::GetShortestPathsAsJSON()
{
    string result;
    /*if (this->solver)
        result = this->solver->GetShortestPathsAsJSON();*/
    return result;
}
std::vector<netxpert::ExtSPTreeArc> netxpert::simple::ShortestPathTree::GetShortestPaths()
{
    std::vector<netxpert::ExtSPTreeArc> result;
    /*if (this->solver)
        result = this->solver->GetShortestPaths();*/
    return result;
}
