#include "netbuilder_simple.h"

using namespace netxpert::cnfg;
using namespace netxpert::io;
using namespace netxpert::utils;

netxpert::simple::NetworkBuilder::NetworkBuilder(std::string jsonCnfg)
{
	//Convert JSON Config to real Config Object
	NETXPERT_CNFG = UTILS::DeserializeJSONtoObject<Config>(jsonCnfg);
}

std::string netxpert::simple::NetworkBuilder::GetBuiltNetworkAsJSON()
{
	std::string result;

	return result;
}

int netxpert::simple::NetworkBuilder::Build()
{
    //local scope!
    using namespace std;
    using namespace netxpert;
    using namespace netxpert::data;

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
		catch (std::exception& ex)
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

		unordered_map< uint32_t, NetworkBuilderResultArc> kvArcs = builder->GetBuiltNetwork();
		LOGGER::LogDebug("Size of built network: " + to_string(kvArcs.size()));

		this->builder->SaveResults(resultTableName, cmap);

		return 0; //OK
	}
	catch (exception& ex)
	{
		LOGGER::LogError("NetworkBuilderSimple: Unexpected Error!");
		LOGGER::LogError(ex.what());
		return 1; //Not OK
	}
}

std::unordered_map<uint32_t, netxpert::data::NetworkBuilderResultArc>
netxpert::simple::NetworkBuilder::GetBuiltNetwork()
{
	std::unordered_map<uint32_t, netxpert::data::NetworkBuilderResultArc> result;

	if (this->builder)
		result = this->builder->GetBuiltNetwork();

	return result;
}
