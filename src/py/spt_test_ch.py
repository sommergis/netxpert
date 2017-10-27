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


def test_contraction(cnfg, cmap):

    atblname = cnfg.ArcsTableName
    arcsTable = netx.DBHELPER.LoadNetworkFromDB(atblname, cmap)

    net = netx.Network(arcsTable, cmap, cnfg)

    net.ComputeContraction(100)
    net.ExportContractedNetwork(cnfg.ArcsTableName + "-ch-export")
    #net.ImportContractedNetwork(cnfg.ArcsTableName + "-ch")

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



def test_spt_add_nodes_1_1_ch(cnfg, cmap):

    atblname = cnfg.ArcsTableName
    arcsTable = netx.DBHELPER.LoadNetworkFromDB(atblname, cmap)

    net = netx.Network(arcsTable, cmap, cnfg)

    #net.ComputeContraction(95)
    #net.ExportContractedNetwork(cnfg.ArcsTableName + "-ch-export")
    net.ImportContractedNetwork(cnfg.ArcsTableName + "-ch-export")

    # freising west
    #x = 696169 #696742
    #y = 5364026
    # bayern nw
    x = 514920
    y = 5537949

    # deutschland nw
    #x = 322449
    #y = 5722914
    supply = 0

    withCap = False
    startID = net.AddStartNode('start', x, y, supply, cnfg.Treshold,
                                    cmap, withCap)

    #x = 703342
    #y = 5364710
    # freising ost
    x = 703338
    y = 5364600

    # deutschland suedost
    #x = 746433
    #y = 5298931

    endID = net.AddEndNode('end', x, y, supply, cnfg.Treshold,
                                    cmap, withCap)

    #net.ExportContractedNetwork(cnfg.ArcsTableName + "-ch-loadnodes-export")

#    net.ComputeContraction(100)
#    net.ExportContractedNetwork(cnfg.ArcsTableName + "-loadnodes-ch-export")

    #net.ImportContractedNetwork(cnfg.ArcsTableName + "-ch-export")

    #print(startID)
    solver = netx.ShortestPathTree(cnfg)
    solver.SetOrigin(startID)

    dests = netx.Nodes()
    dests.append(endID)
    solver.SetDestinations(dests)

    solver.Solve(net)

    optimum = solver.GetOptimum()

    solver.SaveResults(cnfg.ArcsTableName + "_ch_20170915", cmap)

    del net, solver

    return optimum

if __name__ == "__main__":

    print(netx.Version())

    if 'linux' in sys.platform:
        print 'Running test on Linux..'
        #path_to_cnfg = r"/home/hahne/dev/netxpert1_0/test/bin/Release/ODMatrixCnfg_Big.json"
        # TEST OK
        #path_to_cnfg = r"/home/hahne/dev/netxpert1_0/test/bin/Release/ODMatrixCnfg_small.json"
        # TEST OK
        path_to_cnfg = r"/home/hahne/dev/netxpert1_0/test/bin/Release/SPTCnfg_small.json"
        # TEST OK
        #path_to_cnfg = r"/home/hahne/dev/netxpert1_0/test/bin/Release/SPTCnfg_Germany_s_t.json"
        # TEST results not exact - precision oder arc ids?
        #path_to_cnfg = r"/home/hahne/dev/netxpert1_0/test/bin/Debug/SPTCnfg_spt_lines_5_points_4.json"
        # TEST OK

    if 'win' in sys.platform:
        print 'Running test on Windows..'
        #path_to_cnfg = "ODMatrixCnfg_Big.json"
        path_to_cnfg = "SPTCnfg_small.json"

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

    active_tests =  ["ch only",
                     "1-1 | add nodes",
                     "1-n | add nodes",
                     "1-n | load nodes",
                     "1-all | add nodes"
                    ]
    active_tests = active_tests[1:2]

    if "ch only" in active_tests:
        print("Testing Contraction..")
        starttime = datetime.datetime.now()
        test_contraction(cnfg, cmap)
        stoptime = datetime.datetime.now()
        print(("Duration: {0}".format(stoptime - starttime)))

    if "1-1 | add nodes" in active_tests:
        print("Testing SPT 1-1 with imported Contraction..")
        starttime = datetime.datetime.now()
        test_spt_add_nodes_1_1_ch(cnfg, cmap)
        stoptime = datetime.datetime.now()
        print(("Duration: {0}".format(stoptime - starttime)))
