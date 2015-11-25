#include "netbuilder_simple.h"

netxpert::simple::NetworkBuilder::NetworkBuilder(std::string jsonCnfg)
{
	//Convert JSON Config to real Config Object
	NETXPERT_CNFG = netxpert::UTILS::DeserializeJSONtoObject<netxpert::Config>(jsonCnfg);
}

std::string netxpert::simple::NetworkBuilder::GetBuiltNetworkAsJSON()
{
	std::string result;

	return result;
}

int netxpert::simple::NetworkBuilder::Build()
{
	try
	{
		Config cnfg = this->NETXPERT_CNFG;

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
			std::cout << "Error creating log file: " + cnfg.LogFileFullPath << std::endl;
			std::cout << ex.what() << std::endl;
		}

		string arcsGeomColumnName = cnfg.ArcsGeomColumnName; //"Geometry";

		string pathToSpatiaLiteDB = cnfg.NetXDBPath; //args[0].ToString(); //@"C:\data\TRANSPRT_40.sqlite";
		string arcsTableName = cnfg.ArcsTableName; //args[1].ToString(); //"TRANSPRT_GES_LINE_edges";

		string nodesTableName = cnfg.NodesTableName;
		string nodesGeomColName = cnfg.NodesGeomColumnName;
		string resultTableName = cnfg.ResultTableName.empty() ? cnfg.ArcsTableName + "_net" : cnfg.ResultTableName;

		bool autoCleanNetwork = cnfg.CleanNetwork;

		ColumnMap cmap{ cnfg.ArcIDColumnName, cnfg.FromNodeColumnName, cnfg.ToNodeColumnName,
			cnfg.CostColumnName, cnfg.CapColumnName, cnfg.OnewayColumnName,
			cnfg.NodeIDColumnName, cnfg.NodeSupplyColumnName };

		//2. Load Network to build
		this->builder = unique_ptr<netxpert::NetworkBuilder>(new netxpert::NetworkBuilder(cnfg));
		builder->LoadData();

		LOGGER::LogInfo("Calculating Network..");

		unordered_map< unsigned int, NetworkBuilderResultArc> kvArcs = builder->GetBuiltNetwork();
		LOGGER::LogDebug("Size of built network: " + to_string(kvArcs.size()));

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
				writer->CreateSolverResultTable(resultTableName, NetXpertSolver::NetworkBuilderResult, true);
				writer->CommitCurrentTransaction();
				/*if (cnfg.GeometryHandling != GEOMETRY_HANDLING::RealGeometry)
				{*/
				auto& sldbWriter = dynamic_cast<SpatiaLiteWriter&>(*writer);
				qry = unique_ptr<SQLite::Statement>(sldbWriter.PrepareSaveNetworkBuilderArc(resultTableName));
				//}
				break;
			}
			case RESULT_DB_TYPE::ESRI_FileGDB:
			{
				writer = unique_ptr<DBWriter>(new FGDBWriter(cnfg));
				writer->CreateNetXpertDB();
				writer->OpenNewTransaction();
				writer->CreateSolverResultTable(resultTableName, NetXpertSolver::NetworkBuilderResult, true);
				writer->CommitCurrentTransaction();

				break;
			}
		}

		LOGGER::LogDebug("Writing Geometries..");
		writer->OpenNewTransaction();

		unordered_map< unsigned int, NetworkBuilderResultArc>::iterator it;
		int counter = 0;
		#pragma omp parallel shared(counter) private(it) num_threads(LOCAL_NUM_THREADS)
		{
			for (it = kvArcs.begin(); it != kvArcs.end(); ++it)
			{
			#pragma omp single nowait
				{
					//auto kv = it;

					counter += 1;
					if (counter % 2500 == 0)
						LOGGER::LogInfo("Processed #" + to_string(counter) + " geometries.");

					unsigned int key = it->first;

					NetworkBuilderResultArc value = it->second;

					switch (cnfg.ResultDBType)
					{
						case RESULT_DB_TYPE::SpatiaLiteDB:
						{
							auto& sldb = dynamic_cast<SpatiaLiteWriter&>(*writer);
							#pragma omp critical
							{
								sldb.SaveNetworkBuilderArc(value.extArcID, value.fromNode, value.toNode, value.cost,
									value.capacity, value.oneway, *(value.geom), resultTableName, *qry);
							}
							break;
						}
						case RESULT_DB_TYPE::ESRI_FileGDB:
						{
							auto& fgdb = dynamic_cast<FGDBWriter&>(*writer);
							#pragma omp critical
							{
								fgdb.SaveNetworkBuilderArc(value.extArcID, value.fromNode, value.toNode, value.cost,
									value.capacity, value.oneway, *(value.geom), resultTableName);
							}
							break;
						}
					}
				}//omp single
			}
		}//omp paralell

		writer->CommitCurrentTransaction();
		writer->CloseConnection();
		LOGGER::LogDebug("Done!");
		return 0; //OK
	}
	catch (exception& ex)
	{
		LOGGER::LogError("NetworkBuilderSimple: Unerwarteter Fehler!");
		LOGGER::LogError(ex.what());
		return 1; //Not OK
	}
}

std::unordered_map<unsigned int, netxpert::NetworkBuilderResultArc> 
netxpert::simple::NetworkBuilder::GetBuiltNetwork()
{
	std::unordered_map<unsigned int, netxpert::NetworkBuilderResultArc> result;
	
	if (this->builder)
		result = this->builder->GetBuiltNetwork();

	return result;
}

netxpert::simple::NetworkBuilder::~NetworkBuilder()
{
}
