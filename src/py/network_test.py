# Network Tests

import sys, datetime
sys.path.append('/usr/local/lib')
sys.path.append('/usr/local/lib/netxpert')

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
    #cnfg.SpatiaLiteHome = r"/usr/local/lib"
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

def test_add_nodes(cnfg, cmap):

    atblname = cnfg.ArcsTableName
    arcsTable = netx.DBHELPER.LoadNetworkFromDB(atblname, cmap)

    net = netx.Network(arcsTable, cmap, cnfg)
    net.ConvertInputNetwork(cnfg.CleanNetwork)

    print(("Original Node ID of 1: " + net.GetOriginalNodeID(1)))
    x = 703444
    y = 5364720
    supply = 1
    withCap = False
    newNodeID = net.AddStartNode('1', x, y, supply, cnfg.Treshold, cmap, withCap)
    print(("Original Node ID of {0}: {1}".format(newNodeID,
                                            net.GetOriginalNodeID(newNodeID))))
    x = 703342
    y = 5364710
    supply = -1
    newNodeID = net.AddStartNode('2', x, y, supply, cnfg.Treshold, cmap, withCap)
    print(("Original Node ID of {0}: {1}".format(newNodeID,
                                            net.GetOriginalNodeID(newNodeID))))

    del net

def test_load_nodes(cnfg, cmap):

    atblname = cnfg.ArcsTableName
    ntblname = cnfg.NodesTableName

    arcsTable = netx.DBHELPER.LoadNetworkFromDB(atblname, cmap)
    nodesTable = netx.DBHELPER.LoadNodesFromDB(ntblname,
                                                cnfg.NodesGeomColumnName,
                                                cmap)
    # NewNode to InputNode
    nodes = netx.InputNodes()
    for newNode in nodesTable:
        inputNode = netx.InputNode()
        inputNode.extNodeID = newNode.extNodeID
        inputNode.nodeSupply = newNode.supply
        nodes.append(inputNode)

    net = netx.Network(arcsTable, nodes, cmap, cnfg)
    net.ConvertInputNetwork(cnfg.CleanNetwork)

    del net

def test_load_nodes_2(cnfg, cmap):

    atblname = cnfg.ArcsTableName
    arcsTable = netx.DBHELPER.LoadNetworkFromDB(atblname, cmap)
    ntblname = cnfg.NodesTableName
    nodesTable = netx.DBHELPER.LoadNodesFromDB(ntblname, cnfg.NodesGeomColumnName, cmap)
    withCapacity = False

    net = netx.Network(arcsTable, cmap, cnfg)
    net.ConvertInputNetwork(cnfg.CleanNetwork)

    startNodes = net.LoadStartNodes(nodesTable, cnfg.Treshold, atblname, cnfg.ArcsGeomColumnName, cmap, withCapacity)
    endNodes = net.LoadEndNodes(nodesTable, cnfg.Treshold, atblname, cnfg.ArcsGeomColumnName, cmap, withCapacity)

    del net

if __name__ == "__main__":

    print(netx.Version())
    path_to_cnfg = r"/home/hahne/dev/netxpert/test/bin/Release/ODMatrixCnfg_Big.json"
    #path_to_cnfg = r"/home/hahne/dev/netxpert/test/bin/Release/ODMatrixCnfg_small.json"

    cnfg, cmap = read_config(path_to_cnfg)

    netx.LOGGER.Initialize(cnfg)
    netx.DBHELPER.Initialize(cnfg)

    active_tests = ["add nodes",
                    "load nodes",
                    "load start & end nodes",
                    "reset network"
                    ]

    active_tests = active_tests

    if "add nodes" in active_tests:
        print("Testing Network (add nodes with XY coords)..")
        starttime = datetime.datetime.now()
        test_add_nodes(cnfg, cmap)
        stoptime = datetime.datetime.now()
        print(("Duration: {0}".format(stoptime - starttime)))

    if "load nodes" in active_tests:
        print("Testing Network (load nodes from spatialite table)..")
        starttime = datetime.datetime.now()
        test_load_nodes(cnfg, cmap)
        stoptime = datetime.datetime.now()
        print(("Duration: {0}".format(stoptime - starttime)))

    if "load start & end nodes" in active_tests:
        print("Testing Network (load start & end nodes from spatialite table)..")
        starttime = datetime.datetime.now()
        test_load_nodes_2(cnfg, cmap)
        stoptime = datetime.datetime.now()
        print(("Duration: {0}".format(stoptime - starttime)))
