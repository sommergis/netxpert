# SPT Tests

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

def test_spt_add_nodes_1_1(cnfg, cmap):

    atblname = cnfg.ArcsTableName
    arcsTable = netx.DBHELPER.LoadNetworkFromDB(atblname, cmap)

    net = netx.Network(arcsTable, cmap, cnfg)
    net.ConvertInputNetwork(cnfg.CleanNetwork)

    solver = netx.ShortestPathTree(cnfg)

    x = 703444
    y = 5364720
    supply = 0
    withCap = False
    startID = net.AddStartNode('start', x, y, supply, cnfg.Treshold,
                                    cmap, withCap)

    x = 703342
    y = 5364710
    endID = net.AddEndNode('end', x, y, supply, cnfg.Treshold,
                                    cmap, withCap)

    solver.SetOrigin(startID)
    dests = netx.Destinations()
    dests.append(endID)
    solver.SetDestinations(dests)

    solver.Solve(net)
    optimum = solver.GetOptimum()
    del net, solver

    return optimum
    #return solver.GetOptimum()

def test_spt_add_nodes_1_n(cnfg, cmap):

    atblname = cnfg.ArcsTableName
    arcsTable = netx.DBHELPER.LoadNetworkFromDB(atblname, cmap)

    net = netx.Network(arcsTable, cmap, cnfg)
    net.ConvertInputNetwork(cnfg.CleanNetwork)

    solver = netx.ShortestPathTree(cnfg)

    x = 703444
    y = 5364720
    supply = 0
    withCap = False
    startID = net.AddStartNode('start', x, y, supply, cnfg.Treshold,
                                    cmap, withCap)

    destIDs = []
    x = 703342
    y = 5364710
    destIDs.append(net.AddEndNode('end1', x, y, supply, cnfg.Treshold,
                                    cmap, withCap))

    x = 703332
    y = 5364720
    destIDs.append(net.AddEndNode('end2', x, y, supply, cnfg.Treshold,
                                    cmap, withCap))

    x = 703434
    y = 5364650
    destIDs.append(net.AddEndNode('end3', x, y, supply, cnfg.Treshold,
                                    cmap, withCap))

    solver.SetOrigin(startID)
    dests = netx.Destinations()
    for d in destIDs:
        dests.append(d)

    solver.SetDestinations(dests)

    solver.Solve(net)
    optimum = solver.GetOptimum()
    del net, solver

    return optimum
    #return solver.GetOptimum()

def test_spt_add_nodes_1_all(cnfg, cmap):

    # netx.LOGGER.Initialize(cnfg)
    # netx.DBHELPER.Initialize(cnfg)

    atblname = cnfg.ArcsTableName
    arcsTable = netx.DBHELPER.LoadNetworkFromDB(atblname, cmap)

    net = netx.Network(arcsTable, cmap, cnfg)
    net.ConvertInputNetwork(cnfg.CleanNetwork)

    solver = netx.ShortestPathTree(cnfg)

    x = 703444
    y = 5364720
    supply = 0
    withCap = False
    startID = net.AddStartNode('start', x, y, supply, cnfg.Treshold,
                                    cmap, withCap)

    solver.SetOrigin(startID)
    # all -> no setting of dests
    solver.Solve(net)
    optimum = solver.GetOptimum()
    del net, solver

    return optimum
    #return solver.GetOptimum()


def test_spt_load_nodes_1_n(cnfg, cmap):

    atblname = cnfg.ArcsTableName
    arcsTable = netx.DBHELPER.LoadNetworkFromDB(atblname, cmap)
    ntblname = cnfg.NodesTableName
    nodesTable = netx.DBHELPER.LoadNodesFromDB(ntblname, cnfg.NodesGeomColumnName, cmap)
    withCapacity = False

    net = netx.Network(arcsTable, cmap, cnfg)
    net.ConvertInputNetwork(cnfg.CleanNetwork)

    startNodes = net.LoadStartNodes(nodesTable, cnfg.Treshold, atblname, cnfg.ArcsGeomColumnName, cmap, withCapacity)
    endNodes = net.LoadEndNodes(nodesTable, cnfg.Treshold, atblname, cnfg.ArcsGeomColumnName, cmap, withCapacity)

    solver = netx.ShortestPathTree(cnfg)

    solver.SetOrigin(startNodes[0][0]) #first start; first item of tuple is the new internal id
    dests = netx.Destinations()
    for d in endNodes:
        dests.append(d[0]) #first item of tuple is the new internal id
    solver.SetDestinations(dests)

    solver.Solve(net)
    optimum = solver.GetOptimum()
    del net, solver

    return optimum
    #return solver.GetOptimum()

if __name__ == "__main__":
    print(netx.Version())
    #path_to_cnfg = r"/home/hahne/dev/netxpert/test/bin/Release/ODMatrixCnfg_Big.json"
    path_to_cnfg = r"/home/hahne/dev/netxpert/test/bin/Release/ODMatrixCnfg_small.json"

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

    active_tests =  ["1-1 | add nodes",
                     "1-n | add nodes",
                     "1-n | load nodes",
                     "1-all | add nodes"
                    ]

    active_tests = active_tests[0:2]

    if "1-1 | add nodes" in active_tests:
        for i in range(1, 7):
            cnfg.SptAlgorithm = i
            print(("Testing SPT (1-1 | add nodes) with Algorithm {0}..".format(alg_dict[i])))
            starttime = datetime.datetime.now()
            result = test_spt_add_nodes_1_1(cnfg, cmap)
            stoptime = datetime.datetime.now()
            print(("Duration: {0}".format(stoptime - starttime)))
            print(result)
            if "Big" in path_to_cnfg:
                if str(result) != str(18.3440133391):
                    print("test failed!")
                else:
                    print("test succeeded.")
            if "small" in path_to_cnfg:
                if str(result) != str(102.0257824):
                    print("test failed!")
                else:
                    print("test succeeded.")

    if "1-n | add nodes" in active_tests:
        # Dijkstra_dheap_BOOST fails
        for i in range(1, 7):
            cnfg.SptAlgorithm = i
            print(("Testing SPT (1-n | add nodes) with Algorithm {0}..".format(alg_dict[i])))
            starttime = datetime.datetime.now()
            result = test_spt_add_nodes_1_n(cnfg, cmap)
            stoptime = datetime.datetime.now()
            print(("Duration: {0}".format(stoptime - starttime)))
            #print(result)
            if "Big" in path_to_cnfg:
                if str(result) != str(18.3440133391):
                    print("test failed!")
                else:
                    print("test succeeded.")
            if "small" in path_to_cnfg:
                if str(result) != str(224.269976681):
                    print("test failed!")
                else:
                    print("test succeeded.")

    if "1-n | load nodes" in active_tests:
        for i in range(1, 7):
            cnfg.SptAlgorithm = i
            print(("Testing SPT (1-n | load nodes) with Algorithm {0}..".format(alg_dict[i])))
            starttime = datetime.datetime.now()
            result = test_spt_load_nodes_1_n(cnfg, cmap)
            stoptime = datetime.datetime.now()
            print(("Duration: {0}".format(stoptime - starttime)))
            print(result)
            if "Big" in path_to_cnfg:
                if str(result) != str(407788.849797):
                    print("test failed!")
                else:
                    print("test succeeded.")
            if "small" in path_to_cnfg:
                if str(result) != str(14525.2353775):
                    print("test failed!")
                else:
                    print("test succeeded.")

    if "1-all | add nodes" in active_tests:
        # Dijkstra_dheap_BOOST fails
        for i in range(1, 7):
            cnfg.SptAlgorithm = i
            print(("Testing SPT (1-all | add nodes) with Algorithm {0}..".format(alg_dict[i])))
            starttime = datetime.datetime.now()
            result = test_spt_add_nodes_1_all(cnfg, cmap)
            stoptime = datetime.datetime.now()
            print(("Duration: {0}".format(stoptime - starttime)))
            #print(result)
            if "Big" in path_to_cnfg:
                if str(result) != str(18.3440133391):
                    print("test failed!")
                else:
                    print("test succeeded.")
            if "small" in path_to_cnfg:
                if str(result) != str(36547.4605421):
                    print("test failed!")
                else:
                    print("test succeeded.")

