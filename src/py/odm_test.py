# ODM Tests

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
    cnfg.SPTHeapCard = 2
    cnfg.LogLevel = 5

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

def test_odm_add_nodes(cnfg, cmap):

    atblname = cnfg.ArcsTableName
    arcsTable = netx.DBHELPER.LoadNetworkFromDB(atblname, cmap)

    net = netx.Network(arcsTable, cmap, cnfg)
    net.ConvertInputNetwork(cnfg.CleanNetwork)

    solver = netx.OriginDestinationMatrix(cnfg)

    startIDs = []
    x = 703444
    y = 5364720
    supply = 0
    withCap = False
    startIDs.append(net.AddStartNode('start1', x, y, supply, cnfg.Treshold,
                                    cmap, withCap))

    x = 703434
    y = 5364710
    startIDs.append(net.AddStartNode('start2', x, y, supply, cnfg.Treshold,
                                    cmap, withCap))

    destIDs = []
    x = 703342
    y = 5364710
    destIDs.append(net.AddEndNode('end1', x, y, supply, cnfg.Treshold,
                                    cmap, withCap))

    x = 703332
    y = 5364720
    destIDs.append(net.AddEndNode('end2', x, y, supply, cnfg.Treshold,
                                    cmap, withCap))

    starts = netx.ODNodes()
    for s in startIDs:
        starts.append((s, str(s)))
    solver.SetOrigins(starts)
    dests = netx.ODNodes()

    for d in destIDs:
        dests.append((d, str(d)))

    solver.SetDestinations(dests)

    solver.Solve(net)

    optimum = solver.GetOptimum()
    del net, solver

    return optimum
    #return solver.GetOptimum())

def test_odm_load_nodes(cnfg, cmap):

    atblname = cnfg.ArcsTableName
    arcsTable = netx.DBHELPER.LoadNetworkFromDB(atblname, cmap)
    ntblname = cnfg.NodesTableName
    nodesTable = netx.DBHELPER.LoadNodesFromDB(ntblname, cnfg.NodesGeomColumnName, cmap)
    withCapacity = False

    net = netx.Network(arcsTable, cmap, cnfg)
    net.ConvertInputNetwork(cnfg.CleanNetwork)

    startNodes = net.LoadStartNodes(nodesTable, cnfg.Treshold, atblname, cnfg.ArcsGeomColumnName, cmap, withCapacity)
    endNodes = net.LoadEndNodes(nodesTable, cnfg.Treshold, atblname, cnfg.ArcsGeomColumnName, cmap, withCapacity)

    solver = netx.OriginDestinationMatrix(cnfg)

    starts = netx.ODNodes()
    for s in startNodes:
        starts.append(s)

    dests = netx.ODNodes()
    for d in endNodes:
        dests.append(d)

    solver.SetOrigins(starts)
    solver.SetDestinations(dests)

    solver.Solve(net)

    optimum = solver.GetOptimum()
    del net, solver

    return optimum
    #return solver.GetOptimum()

if __name__ == "__main__":
    print(netx.Version())
    path_to_cnfg = r"/home/hahne/dev/netxpert/test/bin/Release/ODMatrixCnfg_Big.json"
    #path_to_cnfg = r"/home/hahne/dev/netxpert/test/bin/Release/ODMatrixCnfg_small.json"

    cnfg, cmap = read_config(path_to_cnfg)

    netx.LOGGER.Initialize(cnfg)
    netx.DBHELPER.Initialize(cnfg)

    alg_dict = {
        0: "Dijkstra_MCFClass",
        1: "LQueue_MCFClass",
        2: "LDeque_MCFClass",
        3: "Dijkstra_Heap_MCFClass",
        4: "Dijkstra_2Heap_LEMON",
        5: "Bijkstra_2Heap_LEMON",
        6: "Dijkstra_dheap_BOOST"
        }

    active_tests =  ["2-2 | add nodes",
                     "n-n | load nodes"
                    ]

    active_tests = active_tests

    if "2-2 | add nodes" in active_tests:
        for i in range(1, 7):
            cnfg.SptAlgorithm = i
            print(("Testing ODM (add nodes) with Algorithm {0}..".format(alg_dict[i])))
            starttime = datetime.datetime.now()
            result = test_odm_add_nodes(cnfg, cmap)
            stoptime = datetime.datetime.now()
            print(("Duration: {0}".format(stoptime - starttime)))
            #print(result)
            if "Big" in path_to_cnfg:
                if str(result) != str(69.7588758141):
                    print("test failed!")
                else:
                    print("test succeeded.")
            if "small" in path_to_cnfg:
                if str(result) != str(407.956086602):
                    print("test failed!")
                else:
                    print("test succeeded.")

    if "n-n | load nodes" in active_tests:
        for i in range(1, 7):
            cnfg.SptAlgorithm = i
            print(("Testing ODM (load nodes) with Algorithm {0}..".format(alg_dict[i])))
            starttime = datetime.datetime.now()
            result = test_odm_load_nodes(cnfg, cmap)
            stoptime = datetime.datetime.now()
            print(("Duration: {0}".format(stoptime - starttime)))
            #print(result)
            if "Big" in path_to_cnfg:
                if str(result) != str(15156313.8398):
                    print("test failed!")
                else:
                    print("test succeeded.")
            if "small" in path_to_cnfg:
                if str(result) != str(63661.0253598):
                    print("test failed!")
                else:
                    print("test succeeded.")