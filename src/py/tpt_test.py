#
# This file is a part of netxpert.
#
# Copyright (C) 2013-2017
# Johannes Sommer, Christopher Koller
#
# Permission to use, modify and distribute this software is granted
# provided that this copyright notice appears in all copies. For
# precise terms see the accompanying LICENSE file.
#
# This software is provided "AS IS" with no warranty of any kind,
# express or implied, and with no claim as to its suitability for any
# purpose.
#
# MCF Tests

import sys, datetime, os

if 'linux' in sys.platform:
    sys.path.append('/usr/local/lib')
    sys.path.append('/usr/local/lib/netxpert')

if 'win' in sys.platform:
    pass

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

    #config is root
    config_json = json.loads(content)['config']

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
    cnfg.Threshold = config_json["Threshold"]
    cnfg.GeometryHandling = config_json["GeometryHandling"]
    cnfg.UseSpatialIndex = config_json["UseSpatialIndex"]
    cnfg.Threshold = 2500
    cnfg.LogLevel = -1

    cmap = netx.ColumnMap()
    cmap.arcIDColName = cnfg.ArcIDColumnName
    cmap.fromColName = cnfg.FromNodeColumnName
    cmap.toColName = cnfg.ToNodeColumnName
    cmap.costColName = cnfg.CostColumnName
    cmap.capColName = cnfg.CapColumnName
    #cmap.onewayColName = cnfg.OnewayColumnName
    cmap.nodeIDColName = cnfg.NodeIDColumnName
    cmap.supplyColName = cnfg.NodeSupplyColumnName

    return cnfg, cmap

def test_tpt(cnfg, cmap):

    atblname = cnfg.ArcsTableName
    arcsTable = netx.DBHELPER.LoadNetworkFromDB(atblname, cmap)

    net = netx.Network(arcsTable, cmap, cnfg)

    startIDs = []
    x = 702370
    y = 5352540
    supply = 5
    withCap = True
    startIDs.append(net.AddStartNode('start1', x, y, supply, cnfg.Threshold,
                                     cmap, withCap))

    x = 701360
    y = 5352530
    supply = 5
    startIDs.append(net.AddStartNode('start2', x, y, supply, cnfg.Threshold,
                                     cmap, withCap))

    destIDs = []
    x = 699022
    y = 5355445
    supply = -5
    destIDs.append(net.AddEndNode('end1', x, y, supply, cnfg.Threshold,
                                  cmap, withCap))

    x = 702237
    y = 5358572
    supply = -3
    destIDs.append(net.AddEndNode('end2', x, y, supply, cnfg.Threshold,
                                  cmap, withCap))

    starts = netx.Nodes()
    dests = netx.Nodes()

    for s in startIDs:
        starts.append(s)
    for d in destIDs:
        dests.append(d)

    solver = netx.Transportation(cnfg)

    solver.SetOrigins(starts)
    solver.SetDestinations(dests)

    solver.Solve(net)

    optimum = solver.GetOptimum()
    del net, solver

    return optimum
    #return solver.GetOptimum())

def test_tpt_load_nodes(cnfg, cmap):

    atblname = cnfg.ArcsTableName
    arcsTable = netx.DBHELPER.LoadNetworkFromDB(atblname, cmap)
    ntblname = cnfg.NodesTableName
    nodesTable = netx.DBHELPER.LoadNodesFromDB(ntblname, cnfg.NodesGeomColumnName, cmap)
    withCapacity = True

    net = netx.Network(arcsTable, cmap, cnfg)

    startNodes = net.LoadStartNodes(nodesTable, cnfg.Threshold, atblname, cnfg.ArcsGeomColumnName, cmap, withCapacity)
    endNodes = net.LoadEndNodes(nodesTable, cnfg.Threshold, atblname, cnfg.ArcsGeomColumnName, cmap, withCapacity)

    solver = netx.Transportation(cnfg)

    starts = netx.Nodes()
    dests = netx.Nodes()

    for s in startNodes:
        starts.append(s[0])
    for d in endNodes:
        dests.append(d[0])


    solver.SetOrigins(starts)
    solver.SetDestinations(dests)

    solver.Solve(net)

    optimum = solver.GetOptimum()
    del net, solver

    return optimum
    #return solver.GetOptimum()

if __name__ == "__main__":

    print(netx.Version())

    if 'linux' in sys.platform:
        print 'Running test on Linux..'
        path_to_cnfg = r"/home/hahne/dev/netxpert1_0/test/bin/Debug/TranspCnfg_small.json"
        #path_to_cnfg = r"/home/hahne/dev/netxpert1_0/test/bin/Release/TransCnfg_med_2_baysf.json"
        #path_to_cnfg = r"/home/hahne/dev/netxpert1_0/test/bin/Release/TransCnfg_med_baysf.json"

    if 'win' in sys.platform:
        print 'Running test on Windows..'
        path_to_cnfg = "TransCnfg_small.json"

    cnfg, cmap = read_config(path_to_cnfg)

    if 'linux' in sys.platform:
        cnfg.SpatiaLiteHome = r"/home/hahne/dev/netx"
        cnfg.SpatiaLiteCoreName = './libspatialite'

    if 'win' in sys.platform:
        cnfg.SpatiaLiteHome = r'C:/Users/johannes/Desktop/netxpert_release_deploy_1_0'
        cnfg.SpatiaLiteCoreName = 'spatialite430'

    print cnfg.SpatiaLiteHome

    netx.LOGGER.Initialize(cnfg)
    netx.DBHELPER.Initialize(cnfg)

    spt_alg_dict = {
        0: "Dijkstra_MCFClass",
        1: "LQueue_MCFClass",
        2: "LDeque_MCFClass",
        3: "Dijkstra_Heap_MCFClass",
        4: "Dijkstra_2Heap_LEMON",
        5: "Bijkstra_2Heap_LEMON",
        6: "Dijkstra_dheap_BOOST"
        }

    mcf_alg_dict = {
        0: "NetworkSimplex_MCF",
        1: "NetworkSimplex_LEMON"
        }

    active_tests =  ["tp | add nodes",
                     "tp | load nodes"
                    ]

    active_tests = active_tests[:2]

    if "tp | add nodes" in active_tests:
        for i in range(1, 2):
            cnfg.McfAlgorithm = i
            print(("Testing TPT with MCF Algorithm {0}..".format(mcf_alg_dict[i])))
            starttime = datetime.datetime.now()
            result = test_tpt(cnfg, cmap)
            stoptime = datetime.datetime.now()
            print(("Duration: {0}".format(stoptime - starttime)))
            print(result)
            if "Big" in path_to_cnfg:
                if str(result) != str(0):
                    print("test failed!")
                else:
                    print("test succeeded.")
            if "small" in path_to_cnfg:
                if str(result) != str(20.4702320011):
                    print("test failed!")
                else:
                    print("test succeeded.")

    if "tp | load nodes" in active_tests:
        for i in range(1, 2):
            cnfg.McfAlgorithm = i
            print(("Testing TPT (load nodes) with MCF Algorithm {0}..".format(mcf_alg_dict[i])))
            starttime = datetime.datetime.now()
            result = test_tpt_load_nodes(cnfg, cmap)
            stoptime = datetime.datetime.now()
            print(("Duration: {0}".format(stoptime - starttime)))
            print(result)
            if "Big" in path_to_cnfg:
                if str(result) != str(0):
                    print("test failed!")
                else:
                    print("test succeeded.")
            if "small" in path_to_cnfg:
                if str(result) != str(16.0):
                    print("test failed!")
                else:
                    print("test succeeded.")
