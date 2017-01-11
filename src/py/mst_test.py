# MST Tests

import sys, datetime, os
sys.path.append('/usr/local/lib')
sys.path.append('/usr/local/lib/netxpert')

parallelization = True  # True | False
# must be done before module import of pynetxpert
if parallelization:
    import multiprocessing as mp
    os.environ["OMP_NUM_THREADS"] = str(mp.cpu_count())
else:
    os.environ["OMP_NUM_THREADS"] = str(1)

import pynetxpert as netx
import json

def read_config(path_to_cnfg):

    f = open(path_to_cnfg, "r")
    content = f.read()
    f.close()

    #c is root
    config_json = json.loads(content)['c']

    cnfg = netx.Config()
    cnfg.ArcsGeomColumnName = config_json["ArcsGeomColumnName"].encode('ascii', 'ignore')
    cnfg.ArcsTableName = config_json["ArcsTableName"].encode('ascii', 'ignore')
    cnfg.NetXDBPath = config_json["NetXDBPath"].encode('ascii', 'ignore')
    cnfg.TestCase = config_json["TestCase"]
    cnfg.NodesTableName = config_json["NodesTableName"].encode('ascii', 'ignore')
    cnfg.NodesGeomColumnName = config_json["NodesGeomColumnName"].encode('ascii', 'ignore')
    cnfg.NodeIDColumnName = config_json["NodeIDColumnName"].encode('ascii', 'ignore')
    cnfg.NodeSupplyColumnName = config_json["NodeSupplyColumnName"].encode('ascii', 'ignore')
    cnfg.ArcIDColumnName = config_json["ArcIDColumnName"].encode('ascii', 'ignore')
    cnfg.FromNodeColumnName = config_json["FromNodeColumnName"].encode('ascii', 'ignore')
    cnfg.ToNodeColumnName = config_json["ToNodeColumnName"].encode('ascii', 'ignore')
    cnfg.CostColumnName = config_json["CostColumnName"].encode('ascii', 'ignore')
    cnfg.CapColumnName = config_json["CapColumnName"].encode('ascii', 'ignore')
    cnfg.OnewayColumnName = config_json["OnewayColumnName"].encode('ascii', 'ignore')
    cnfg.IsDirected = config_json["IsDirected"]
    cnfg.LogLevel = config_json["LogLevel"]
    cnfg.LogFileFullPath = config_json["LogFileFullPath"].encode('ascii', 'ignore')
    cnfg.SpatiaLiteHome = config_json["SpatiaLiteHome"].encode('ascii', 'ignore')
    cnfg.SpatiaLiteCoreName = config_json["SpatiaLiteCoreName"].encode('ascii', 'ignore')
    cnfg.CleanNetwork = config_json["CleanNetwork"]
    cnfg.ResultDBType = config_json["ResultDBType"]
    cnfg.ResultDBPath = config_json["ResultDBPath"].encode('ascii', 'ignore')
    cnfg.Treshold = config_json["Treshold"]
    cnfg.GeometryHandling = config_json["GeometryHandling"]
    cnfg.UseSpatialIndex = config_json["UseSpatialIndex"]
    cnfg.Treshold = 2500
    cnfg.LogLevel = -1

    cmap = netx.ColumnMap()
    cmap.arcIDColName = cnfg.ArcIDColumnName
    cmap.fromColName = cnfg.FromNodeColumnName
    cmap.toColName = cnfg.ToNodeColumnName
    cmap.costColName = cnfg.CostColumnName
    #cmap.capColName = cnfg.CapColumnName
    #cmap.onewayColName = cnfg.OnewayColumnName
    cmap.nodeIDColName = cnfg.NodeIDColumnName
    cmap.supplyColName = cnfg.NodeSupplyColumnName

    return cnfg, cmap

def test_mst(cnfg, cmap):

    atblname = cnfg.ArcsTableName
    arcsTable = netx.DBHELPER.LoadNetworkFromDB(atblname, cmap)

    net = netx.Network(arcsTable, cmap, cnfg)
    net.ConvertInputNetwork(cnfg.CleanNetwork)

    solver = netx.MinimumSpanningTree(cnfg)
    solver.Solve(net)

    optimum = solver.GetOptimum()
    del net, solver

    return optimum
    #return solver.GetOptimum())

def test_mst_load_nodes(cnfg, cmap):

    atblname = cnfg.ArcsTableName
    arcsTable = netx.DBHELPER.LoadNetworkFromDB(atblname, cmap)
    ntblname = cnfg.NodesTableName
    nodesTable = netx.DBHELPER.LoadNodesFromDB(ntblname, cnfg.NodesGeomColumnName, cmap)

    net = netx.Network(arcsTable, cmap, cnfg)
    net.ConvertInputNetwork(cnfg.CleanNetwork)

    solver = netx.MinimumSpanningTree(cnfg)

    solver.Solve(net)

    optimum = solver.GetOptimum()
    del net, solver

    return optimum
    #return solver.GetOptimum()

if __name__ == "__main__":
    print(netx.Version())
    #path_to_cnfg = r"/home/hahne/dev/netxpert/test/bin/Release/MSTCnfg_Big.json"
    path_to_cnfg = r"/home/hahne/dev/netxpert/test/bin/Release/MSTCnfg_small.json"

    cnfg, cmap = read_config(path_to_cnfg)

    cnfg.SpatiaLiteHome = r'/usr/local/lib'
    cnfg.SpatiaLiteCoreName = './mod_spatialite'

    netx.LOGGER.Initialize(cnfg)
    netx.DBHELPER.Initialize(cnfg)

    alg_dict = {
        0: "",
        1: "",
        2: "Kruskal_LEMON"
        }

    active_tests =  ["mst",
                     "mst | load nodes"
                    ]

    active_tests = active_tests[:1]

    if "mst" in active_tests:
        for i in range(2, 3):
            cnfg.MstAlgorithm = i
            print(("Testing MST with Algorithm {0}..".format(alg_dict[i])))
            starttime = datetime.datetime.now()
            result = test_mst(cnfg, cmap)
            stoptime = datetime.datetime.now()
            print(("Duration: {0}".format(stoptime - starttime)))
            #print(result)
            if "Big" in path_to_cnfg:
                if str(result) != str(10816901.7965):
                    print("test failed!")
                else:
                    print("test succeeded.")
            if "small" in path_to_cnfg:
                if str(result) != str(28596.3859159):
                    print("test failed!")
                else:
                    print("test succeeded.")

    if "mst | load nodes" in active_tests:
        for i in range(2, 3):
            cnfg.MstAlgorithm = i
            print(("Testing MST (load nodes) with Algorithm {0}..".format(alg_dict[i])))
            starttime = datetime.datetime.now()
            result = test_mst_load_nodes(cnfg, cmap)
            stoptime = datetime.datetime.now()
            print(("Duration: {0}".format(stoptime - starttime)))
            #print(result)
            if "Big" in path_to_cnfg:
                if str(result) != str(10816901.7965):
                    print("test failed!")
                else:
                    print("test succeeded.")
            if "small" in path_to_cnfg:
                if str(result) != str(28596.3859159):
                    print("test failed!")
                else:
                    print("test succeeded.")