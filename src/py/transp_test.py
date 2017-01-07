# Simple (JSON) Transportation Solver Test

import sys, datetime
sys.path.append('/usr/local/lib')
sys.path.append('/usr/local/lib/netxpert')

import pynetxpert as netx

def test_data():
  odmatrix = [
                                 {"arcid": str("5"),"fromNode": str("4"),"toNode": str("2"),"cost": 2 },
                                 {"arcid": str("4"),"fromNode": str("3"),"toNode": str("4"),"cost": 1 },
                                 {"arcid": str("3"),"fromNode": str("3"),"toNode": str("1"),"cost": 3 },
                                 {"arcid": str("2"),"fromNode": str("4"),"toNode": str("1"),"cost": 1 },
                                 {"arcid": str("1"),"fromNode": str("1"),"toNode": str("2"),"cost": 2 },
                                 {"arcid": str("6"),"fromNode": str("5"),"toNode": str("4"),"cost": 3 },
                                 {"arcid": str("7"),"fromNode": str("5"),"toNode": str("2"),"cost": 6 }
                                ]
  nodeSupply = [  { "nodeid": str("3"),"supply": 5 },
                                                { "nodeid": str("4"),"supply": 0 },
                                                { "nodeid": str("1"),"supply": -2},
                                                { "nodeid": str("2"),"supply": -4},
                                                { "nodeid": str("5"),"supply": 1 }
                                             ]
  return odmatrix, nodeSupply

def convert_test_data(odmatrix, _supply):

  odm = netx.ExtSPTArcs()
  supply = netx.ExtNodeSupplies()

  for item in _odmatrix:
    o = netx.ExtSPTreeArc()
    #print item
    o.extArcID = item["arcid"]
    e = netx.ExternalArc()
    e.extFromNode = item["fromNode"]
    e.extToNode = item["toNode"]
    o.extArc = e
    o.cost = item["cost"]
    odm.append(o)

  for item in _supply:
    n = netx.ExtNodeSupply()
    n.extNodeID = item["nodeid"]
    n.supply = item["supply"]
    supply.append(n)

  return odm, supply

def tpsolve(odmatrix, nodeSupply):

  cnfg = netx.Config()

  cnfg.LogLevel = -1
  cnfg.LogFileFullPath = "/var/www/apps/netxpert/netXpert.log"
  cnfg.SpatiaLiteHome = "/usr/local/libs"
  cnfg.SpatiaLiteCoreName = "./libspatialite"
  cnfg.CleanNetwork = False
  cnfg.McfAlgorithm = 1

  netx.LOGGER.Initialize(cnfg)

  solver = netx.Transportation(cnfg);

  data = netx.ExtTransportationData()

  data.odm = odmatrix
  data.supply = nodeSupply

  solver.SetExtODMatrix(data.odm)
  solver.SetExtNodeSupply(data.supply)

  solver.Solve()

  optimum = solver.GetOptimum()
  dist = solver.GetExtDistribution()
  result = solver.GetSolverJSONResult()

  del net, solver
  #print "Optimum:",optimum

  return result

def check_result(json_result):

  if json_result.replace(r'\n', '') == '''{
    "result": {
        "optimum": 18,
        "distribution": [
            {
                "arcid": "6",
                "fromNode": "5",
                "toNode": "4",
                "cost": 3,
                "flow": 1
            },
            {
                "arcid": "4",
                "fromNode": "3",
                "toNode": "4",
                "cost": 1,
                "flow": 5
            },
            {
                "arcid": "2",
                "fromNode": "4",
                "toNode": "1",
                "cost": 1,
                "flow": 2
            },
            {
                "arcid": "5",
                "fromNode": "4",
                "toNode": "2",
                "cost": 2,
                "flow": 4
            }
        ]
    }
 }'''.replace(r'\n',''):
    return True

  else:
    return False

if __name__ == '__main__':
    print(netx.Version())
    o, s = test_data()
    #print o
    #print s
    odm, supply = convert_test_data(o, s)

    #print odm, supply
    result = tpsolve(odm, supply)

    if check_result(result):
        print("test succeeded.")
    else:
        print("test failed!")