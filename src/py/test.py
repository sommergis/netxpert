# Python test

import pynetxpert as netx
import json

def main():
  f = open(r"/home/hahne/dev/netxpert/bin/Release/ODMatrixCnfg.json", "r")
  content = f.read()
  f.close()

  config_json = json.loads(content)['c'] #c is root
  
  cnfg = netx.Config()

  cnfg.ArcsGeomColumnName = config_json["ArcsGeomColumnName"].encode('ascii', 'ignore')
  cnfg.ArcsTableName = config_json["ArcsTableName"].encode('ascii', 'ignore')
  cnfg.SQLiteDBPath = config_json["SQLiteDBPath"].encode('ascii', 'ignore')
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

  #print netx.LOGGER.IsInitialized

  #if not netx.LOGGER.IsInitialized:
  print "init logger"
  netx.LOGGER.Initialize(cnfg)

  cmap = netx.ColumnMap()
  cmap.arcIDColName = cnfg.ArcIDColumnName
  cmap.fromColName = cnfg.FromNodeColumnName
  cmap.toColName = cnfg.ToNodeColumnName
  cmap.costColName = cnfg.CostColumnName
  #cmap.capColName = cnfg.CapColumnName
  #cmap.onewayColName = cnfg.OnewayColumnName
  cmap.nodeIDColName = cnfg.NodeIDColumnName
  cmap.supplyColName = cnfg.NodeSupplyColumnName

  #if not netx.DBHELPER.IsInitialized:
  print "init dbhelper"
  netx.DBHELPER.Initialize(cnfg)

  atblname = cnfg.ArcsTableName
  ntblname = cnfg.NodesTableName

  arcsTable = netx.DBHELPER.LoadNetworkFromDB(atblname, cmap)
  nodesTable = netx.DBHELPER.LoadNodesFromDB(ntblname, cmap)

  #net = netx.Network(arcsTable, cmap, cnfg)
  net = netx.Network(arcsTable, nodesTable, cmap, cnfg)
  net.ConvertInputNetwork(cnfg.CleanNetwork)

  print "Original Node ID of 1: " + net.GetOriginalNodeID(1)

if __name__ == "__main__":
  main()
