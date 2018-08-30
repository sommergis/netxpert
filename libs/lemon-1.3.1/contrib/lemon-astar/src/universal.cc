/* -*- mode: C++; indent-tabs-mode: nil; -*-
 *
 * This file is a part of LEMON, a generic C++ optimization library.
 *
 * Copyright (C) 2003-2009
 * Egervary Jeno Kombinatorikus Optimalizalasi Kutatocsoport
 * (Egervary Research Group on Combinatorial Optimization, EGRES).
 *
 * Permission to use, modify and distribute this software is granted
 * provided that this copyright notice appears in all copies. For
 * precise terms see the accompanying LICENSE file.
 *
 * This software is provided "AS IS" with no warranty of any kind,
 * express or implied, and with no claim as to its suitability for any
 * purpose.
 *
 */

///\ingroup demos
///\file
///\brief Demonstrating graph input and output
///
/// This program gives an example of how to read and write a digraph
/// and additional maps from/to a stream or a file using the
/// \ref lgf-format "LGF" format.
///
/// The \c "digraph.lgf" file:
/// \include digraph.lgf
///
/// And the program which reads it and prints the digraph to the
/// standard output:
/// \include lgf_demo.cc

#include <iostream>
#include <lemon/smart_graph.h>
#include <lemon/list_graph.h>
#include <lemon/lgf_reader.h>
#include <lemon/lgf_writer.h>
#include <sys/timeb.h>
#include <string.h>
#include "dijkstra.h"
#include "bijkstra.h"
#include "astar.h"
#include "abistar.h"
#include <fstream>
#include <unistd.h>
#include <math.h>
#include "estimate_calcs.h"

using namespace lemon;
  
  //defines for choosing algorithm
  #define DIJKSTRA  0
  #define BIJKSTRA  1
  #define ASTAR     2
  #define ABISTAR    4
  
  //defines for choosing route handling
  #define DEFAULT           0
  #define DEFAULT_WITH_DRAW 1
  #define FROM_FILE         2
  
  //defines for write options
  #define NONE      0
  #define SOURCE    1
  #define TARGET    2
  #define DISTANCE  4
  #define LENGTH    8
  #define TIME      16
  #define SCANNED   32
  #define REACHED   64
  #define INSERTED  128
  #define DECREASED 256
  
    
  int calc_elapsed(const timeb &start, const timeb &finish);
  void write_elapsed (const timeb &start, const timeb &finish, char* what);
  int proc_all(int argc, char** argv);
  void write_help();
  void write_configures(std::string graphPath, int algorithm, int routes, int write_options, std::string routesPath);
  
  int main(int argc, char** argv) {
  
  if (argc < 5 && argc != 2) { write_help(); return 0; }
  std::string graphPath;
  int algorithm;
  int routes;
  int write_options;
  std::string routesPath;
  if (argc != 2)
  {
    graphPath = argv[1];
    algorithm = atoi(argv[2]);
    routes = atoi(argv[3]);
    write_options = atoi(argv[4]);
    if (routes & FROM_FILE)
    {
      if (argc < 6) { std::cout<<"No file specified for routes" << std::endl; write_help(); return 0; }
      routesPath = argv[5];
    }
  
    write_configures(graphPath, algorithm, routes, write_options, routesPath);
  } else {
    graphPath = "USA-road-d.NY.eukl.txt";
    algorithm = atoi(argv[1]);
    routes = FROM_FILE;
    write_options = 511;
    routesPath = "routes.txt";
    write_configures(graphPath, algorithm, routes, write_options, routesPath);
  }
  
  /*std::cout<< (algorithm & DIJKSTRA) << std::endl;
  std::cout<< (algorithm & BIJKSTRA) << std::endl;
  std::cout<< (algorithm & ASTAR) << std::endl;
  std::cout<< (algorithm & ABISTAR) << std::endl;
  switch(algorithm)
  {
  case DIJKSTRA: std::cout<<"dijkstra"<<std::endl; break;
  case BIJKSTRA: std::cout<<"bijkstra"<<std::endl; break;
  case ASTAR: std::cout<<"astar"<<std::endl; break;
  case ABISTAR: std::cout<<"bistar"<<std::endl; break;
  }*/
  typedef ListDigraph Graph;
  typedef Graph::Node Node;
  typedef Graph::Arc Arc;
  typedef Graph::NodeIt NodeIt;
  typedef Graph::ArcIt ArcIt;
  typedef Graph::ArcMap<double> ArcMap;
  typedef EstimateRTCoordCalculator<Graph> EstimateCalculator;
  //typedef EstimatePreciseCalculator<Graph> EstimateCalculator;
  typedef Dijkstra<Graph, ArcMap > Dijkstra;
  typedef Bijkstra<Graph, ArcMap > Bijkstra;
  typedef AStar<Graph, ArcMap, EstimateCalculator> AStar;
  typedef ABiStar<Graph, ArcMap, EstimateCalculator> ABiStar;
  
  ListDigraph g;
  ArcMap cap(g);
  Node s, t;
  struct timeb start, finish;
  struct timeb tsta, tfin;
  
  std::cout << "\nReading the digraph from '" << graphPath << "'..." << std::endl;
  ftime(&start);
  try {
    digraphReader(g, graphPath).arcMap("capacity", cap).node("source", s).node("target", t).run();
  } catch (Exception& error) {
    std::cerr << "Error: " << error.what() << std::endl;
    return -1;
  }
  
  ftime(&finish);
  int nodeCount = countNodes(g), arcCount = countArcs(g);
  
  std::cout << "\nA digraph is read from '" << graphPath << "'." << "\nNumber of nodes: " << nodeCount << "\nNumber of arcs: " << arcCount << std::endl;
  write_elapsed(start,finish,(char*)"read the graph");
  std::cout << "-------------------------------------------" << std::endl;
  
  ///routes from file
  Node *sources = NULL;
  Node *targets = NULL;
  int routeCount;  
  if (routes & FROM_FILE) 
  {
    std::ifstream iFile(routesPath.c_str());
    if (iFile.is_open())
    {
      iFile >> routeCount;
      sources = new ListDigraph::Node[routeCount];
      targets = new ListDigraph::Node[routeCount];
      int temp;
      for (int i=0; i<routeCount; ++i)
      {
        iFile >> temp;
        sources[i] = g.nodeFromId(temp);
      }
      for (int i=0; i<routeCount; ++i)
      {
        iFile >> temp;
        targets[i] = g.nodeFromId(temp);
      }
      iFile.close();
    } else {std::cout<<"couldn't open '" << routesPath << "'" << std::endl; return 1; }
  }
  
  switch (algorithm)
  {
    case BIJKSTRA: std::cout << "Let's begin bidirectional dijsktra...\n" << std::endl; break;
    case ASTAR: std::cout << "Let's begin unidirectional astar...\n" << std::endl; break;
    case ABISTAR: std::cout << "Let's begin bidirectional astar...\n" << std::endl; break;
    default: std::cout << "Let's begin unidirectional dijsktra...\n" << std::endl; break;
  }
  
  ftime(&start);
  ///routes from file///
  if (routes & FROM_FILE) 
  {
    int percentage = 0;
    int block = routeCount/20;
    long int sumtime = 0;
    char* path;
    switch (algorithm)
    {
      case BIJKSTRA: path = (char*)"results_bijkstra.txt"; break;
      case ASTAR: path = (char*)"results_astar.txt"; break;
      case ABISTAR: path = (char*)"results_bistar.txt"; break;
      default: path = (char*)"results_dijkstra.txt"; break;
    
    }
    std::ofstream oFile(path);
    if (oFile.is_open())
    {
      if (write_options & SOURCE) oFile << "source\t";
      if (write_options & TARGET) oFile << "target\t";
      if (write_options & DISTANCE) oFile << "dist" << "\t";
      if (write_options & LENGTH) oFile << "len" << "\t";
      if (write_options & TIME) oFile << "time" << "\t";
      if (write_options & SCANNED) oFile << "scanned" << "\t";
      if (write_options & REACHED) oFile << "reached" << "\t";
      if (write_options & INSERTED) oFile << "inserted" << "\t";
      if (write_options & DECREASED) oFile << "decreased" << "\t";
      if (write_options & SCANNED && algorithm & ABISTAR) oFile << "rejected" << "\t";
      oFile << "\r\n";
      int dist; //it's double, really, but it's easier to handle its output, and in this case it doesn't make any difference
      int length, time;
      long int scanned, reached, inserted, decreased;
      long int rejected = 0;
      EstimateCalculator ec(g); //unused in case of Dijkstras, but it's a lot of time to intialize at every step in case of AStars
      std::cout << "0% done" << std::endl;
      for (int i=0; i<routeCount; ++i)
      {
        switch (algorithm)
        {
        case BIJKSTRA:
          {
          Bijkstra bijkstra(g,cap);
          ftime(&tsta);
          bijkstra.run(sources[i],targets[i]);
          ftime(&tfin);
          
          dist = bijkstra.dist(targets[i]);
          length = bijkstra.getlength(targets[i]);
          time = calc_elapsed(tsta,tfin);
          sumtime += time;
          scanned = bijkstra.getscannedcounter(); reached = bijkstra.getreachedcounter();
          inserted = bijkstra.getinsertedcounter(); decreased = bijkstra.getdecreasedcounter();
          
          break;
          }
        case ASTAR:
          {
          AStar astar(g,cap);
          astar.setEstimateCalculator(ec);
          ftime(&tsta);
          astar.run(sources[i],targets[i]);
          ftime(&tfin);
          
          dist = astar.dist(targets[i]);
          length = astar.getlength(targets[i]);
          time = calc_elapsed(tsta,tfin);
          sumtime += time;
          scanned = astar.getscannedcounter(); reached = astar.getreachedcounter();
          inserted = astar.getinsertedcounter(); decreased = astar.getdecreasedcounter();
          break;
          }
        case ABISTAR:
          {
          ABiStar bistar(g,cap);
          bistar.setEstimateCalculator(ec);
          ftime(&tsta);
          bistar.run(sources[i],targets[i]);
          ftime(&tfin);
          
          dist = bistar.dist(targets[i]);
          length = bistar.getlength(targets[i]);
          time = calc_elapsed(tsta,tfin);
          sumtime += time;
          scanned = bistar.getscannedcounter(); reached = bistar.getreachedcounter();
          inserted = bistar.getinsertedcounter(); decreased = bistar.getdecreasedcounter();
          rejected = bistar.getrejectedcounter();
          break;
          }
        default:
          {
          Dijkstra dijkstra(g,cap);
          ftime(&tsta);
          dijkstra.run(sources[i],targets[i]);
          ftime(&tfin);
          
          dist = dijkstra.dist(targets[i]);
          length = dijkstra.getlength(targets[i]);
          time = calc_elapsed(tsta,tfin);
          sumtime += time;
          scanned = dijkstra.getscannedcounter(); reached = dijkstra.getreachedcounter();
          inserted = dijkstra.getinsertedcounter(); decreased = dijkstra.getdecreasedcounter();
          break;
          }
        }
        
        if (write_options & SOURCE) oFile << g.id(sources[i]) << "\t";
        if (write_options & TARGET) oFile << g.id(targets[i]) << "\t";
        if (write_options & DISTANCE) oFile << dist << "\t";
        if (write_options & LENGTH) oFile << length << "\t";
        if (write_options & TIME) oFile << time / 1000 << "." << ((time % 1000) < 100 ? "0" : "") << ((time % 1000) < 10 ? "0" : "") << time % 1000 << "\t";
        if (write_options & SCANNED) oFile << scanned << "\t";
        if (write_options & REACHED) oFile << reached << "\t";
        if (write_options & INSERTED) oFile << inserted << "\t";
        if (write_options & DECREASED) oFile << decreased << "\t";
        if (write_options & SCANNED && algorithm & ABISTAR) oFile << rejected << "\t";
        oFile << "\r\n";
        
        if (i % block == 0) { 
          if (i) { percentage += 5; std::cout << percentage << "% done" << std::endl; }
        }
      }
      if (percentage != 100) std::cout << "100% done" << std::endl;
    }
    oFile.close();
    std::cout<<"Net time: " << sumtime / 1000 << "." << ((sumtime % 1000) < 100 ? "0" : "") << ((sumtime % 1000) < 10 ? "0" : "") << sumtime % 1000 << std::endl;
  }
  else
  {
    if (write_options & SOURCE) std::cout << "source\t";
    if (write_options & TARGET) std::cout << "target\t";
    if (write_options & DISTANCE) std::cout << "dist" << "\t";
    if (write_options & LENGTH) std::cout << "len" << "\t";
    if (write_options & TIME) std::cout << "time" << "\t";
    if (write_options & SCANNED) std::cout << "scanned" << "\t";
    if (write_options & REACHED) std::cout << "reached" << "\t";
    if (write_options & INSERTED) std::cout << "inserted" << "\t";
    if (write_options & DECREASED) std::cout << "decreased" << "\t";
    if (write_options & SCANNED && algorithm & ABISTAR) std::cout << "rejected" << "\t";
    std::cout<<""<<std::endl;
    int dist; //it's double, really, but it's easier to handle its output, and in this case it doesn't make any difference
    int length, time;
    long int scanned, reached, inserted, decreased;
    long int rejected = 0;
    switch(algorithm)
    {
    case BIJKSTRA:
      {
      Bijkstra bijkstra(g,cap);
      ftime(&tsta);
      bijkstra.run(s,t);
      ftime(&tfin);
      dist = bijkstra.dist(t);
      length = bijkstra.getlength(t);
      time = calc_elapsed(tsta,tfin);
      scanned = bijkstra.getscannedcounter(); reached = bijkstra.getreachedcounter();
      inserted = bijkstra.getinsertedcounter(); decreased = bijkstra.getdecreasedcounter();
  
      if (routes & DEFAULT_WITH_DRAW)
      {
        {
          int color;
          std::ofstream oFile("ny_bid_colors.txt");
          if (oFile.is_open()) {
            for (NodeIt it(g); it != INVALID; ++it)
            {
              if (bijkstra.processed(it))
              {
                if (bijkstra.revProcessed(it)) color = 2;
                else color = 4;
              } else if (bijkstra.revProcessed(it)) color = 3;
              else continue;
              oFile << g.id(it) << " " << color << std::endl;
            }
            oFile.close();
          } else std::cout<<"error while opening file"<<std::endl;  
        }
        {
          int color;
          std::ofstream oFile("ny_bid_arcColors.txt");
          if (oFile.is_open()) {
            for (ArcIt it(g); it != INVALID; ++it)
            {
              if (bijkstra.processed(g.target(it)) && bijkstra.processed(g.source(it)))
              {
                if (bijkstra.revProcessed(g.target(it)) && bijkstra.revProcessed(g.source(it))) color = 2;
                else color = 4;
              } else if (bijkstra.revProcessed(g.target(it)) && bijkstra.revProcessed(g.source(it))) color = 3;
              else continue;
              oFile << g.id(it) << " " << color << std::endl;
            }
            oFile.close();
          } else std::cout<<"error while opening file"<<std::endl;  
        }     
    
        {
          int routeColor = 8, routeWidth = 6000;
          std::ofstream oFile("ny_bid_routeArcColors.txt");
          if (oFile.is_open()) {
            Node *n = new Node(t);
            Node *temp = n;
            Arc e;
            while (g.id(*n) != g.id(s))
            {
              e = bijkstra.predArc(*n);
              oFile << g.id(e) << " " << routeColor << " " << routeWidth << std::endl;
              Node src = g.source(e);
              n = &src;
            }
            oFile.close();
            delete temp;
          } else std::cout<<"error while opening file"<<std::endl;  
        }
      }
      break;
      }
    case ASTAR:
      {
      AStar astar(g,cap);
      EstimateCalculator ec(g);
      astar.setEstimateCalculator(ec);
      ftime(&tsta);
      astar.run(s,t);
      ftime(&tfin);
      dist = astar.dist(t);
      length = astar.getlength(t);
      time = calc_elapsed(tsta,tfin);
      scanned = astar.getscannedcounter(); reached = astar.getreachedcounter();
      inserted = astar.getinsertedcounter(); decreased = astar.getdecreasedcounter();
  
      if (routes & DEFAULT_WITH_DRAW)
      {
        {
          std::ofstream oFile("ny_una_colors.txt");
          if (oFile.is_open()) {
            for (NodeIt it(g); it != INVALID; ++it)
            {
              if (astar.processed(it)) oFile << g.id(it) << " 3" << std::endl;
            }
            oFile.close();
          } else std::cout<<"error while opening file"<<std::endl;  
        }
        {
          std::ofstream oFile("ny_una_arcColors.txt");
          if (oFile.is_open()) {
            for (ArcIt it(g); it != INVALID; ++it)
            {
              if (astar.processed(g.target(it)) && astar.processed(g.source(it))) oFile << g.id(it) << " 3" << std::endl;
            }
            oFile.close();
          } else std::cout<<"error while opening file"<<std::endl;  
        }
        {
          int routeColor = 8, routeWidth = 6000;
          std::ofstream oFile("ny_una_routeArcColors.txt");
          if (oFile.is_open()) {
            Node *n = new Node(t);
            Node *temp = n;
            Arc e;
            while (g.id(*n) != g.id(s))
            {
              e = astar.predArc(*n);
              oFile << g.id(e) << " " << routeColor << " " << routeWidth << std::endl;
              Node src = g.source(e);
              n = &src;
            }
            oFile.close();
            delete temp;
          } else std::cout<<"error while opening file"<<std::endl;  
       }
      }
      break;
      }
    case ABISTAR:
      {
      ABiStar abistar(g,cap);
      EstimateCalculator ec(g);
      abistar.setEstimateCalculator(ec);
      ftime(&tsta);
      abistar.run(s,t);
      ftime(&tfin);
      dist = abistar.dist(t);
      length = abistar.getlength(t);
      time = calc_elapsed(tsta,tfin);
      scanned = abistar.getscannedcounter(); reached = abistar.getreachedcounter();
      inserted = abistar.getinsertedcounter(); decreased = abistar.getdecreasedcounter();
  
      if (routes & DEFAULT_WITH_DRAW)
      {
        {
          int color;
          std::ofstream oFile("ny_bia_colors.txt");
          if (oFile.is_open()) {
            for (NodeIt it(g); it != INVALID; ++it)
            {
              if (abistar.processed(it))
              {
                if (abistar.revProcessed(it)) color = 2;
                else color = 4;
              } else if (abistar.revProcessed(it)) color = 3;
              else continue;
              oFile << g.id(it) << " " << color << std::endl;
            }
            oFile.close();
          } else std::cout<<"error while opening file"<<std::endl;  
        }
        {
          int color;
          std::ofstream oFile("ny_bia_arcColors.txt");
          if (oFile.is_open()) {
            for (ArcIt it(g); it != INVALID; ++it)
            {
              if (abistar.processed(g.target(it)) && abistar.processed(g.source(it)))
              {
                if (abistar.revProcessed(g.target(it)) && abistar.revProcessed(g.source(it))) color = 2;
                else color = 4;
              } else if (abistar.revProcessed(g.target(it)) && abistar.revProcessed(g.source(it))) color = 3;
              else continue;
              oFile << g.id(it) << " " << color << std::endl;
            }
            oFile.close();
          } else std::cout<<"error while opening file"<<std::endl;  
        }     
    
        {
          int routeColor = 8, routeWidth = 6000;
          std::ofstream oFile("ny_bia_routeArcColors.txt");
          if (oFile.is_open()) {
            Node *n = new Node(t);
            Node *temp = n;
            Arc e;
            while (g.id(*n) != g.id(s))
            {
              e = abistar.predArc(*n);
              oFile << g.id(e) << " " << routeColor << " " << routeWidth << std::endl;
              Node src = g.source(e);
              n = &src;
            }
            oFile.close();
            delete temp;
          } else std::cout<<"error while opening file"<<std::endl;  
        }
      }
      break;
      }
    default:
      {
      Dijkstra dijkstra(g,cap);
      ftime(&tsta);
      dijkstra.run(s,t);
      ftime(&tfin);
      dist = dijkstra.dist(t);
      length = dijkstra.getlength(t);
      time = calc_elapsed(tsta,tfin);
      scanned = dijkstra.getscannedcounter(); reached = dijkstra.getreachedcounter();
      inserted = dijkstra.getinsertedcounter(); decreased = dijkstra.getdecreasedcounter();
   
      if (routes & DEFAULT_WITH_DRAW)
      {
        {
          std::ofstream oFile("ny_unid_colors.txt");
          if (oFile.is_open()) {
            for (NodeIt it(g); it != INVALID; ++it)
            {
              if (dijkstra.processed(it)) oFile << g.id(it) << " 3" << std::endl;
            }
            oFile.close();
          } else std::cout<<"error while opening file"<<std::endl;  
        }
        {
          std::ofstream oFile("ny_unid_arcColors.txt");
          if (oFile.is_open()) {
            for (ArcIt it(g); it != INVALID; ++it)
            {
              if (dijkstra.processed(g.target(it)) && dijkstra.processed(g.source(it))) oFile << g.id(it) << " 3" << std::endl;
            }
            oFile.close();
          } else std::cout<<"error while opening file"<<std::endl;  
        }
        {
          int routeColor = 8, routeWidth = 6000;
          std::ofstream oFile("ny_unid_routeArcColors.txt");
          if (oFile.is_open()) {
            Node *n = new Node(t);
            Node *temp = n;
            Arc e;
            while (g.id(*n) != g.id(s))
            {
              e = dijkstra.predArc(*n);
              oFile << g.id(e) << " " << routeColor << " " << routeWidth << std::endl;
              Node src = g.source(e);
              n = &src;
            }
            oFile.close();
            delete temp;
          } else std::cout<<"error while opening file"<<std::endl;  
       }
      }
      break;
      }
    }
    if (write_options & SOURCE) std::cout << g.id(s) << "\t";
    if (write_options & TARGET) std::cout << g.id(t) << "\t";
    if (write_options & DISTANCE) std::cout << dist << "\t";
    if (write_options & LENGTH) std::cout << length << "\t";
    if (write_options & TIME) std::cout << time / 1000 << "." << ((time % 1000) < 100 ? "0" : "") << ((time % 1000) < 10 ? "0" : "") << time % 1000 << "\t";
    if (write_options & SCANNED) std::cout << scanned << "\t";
    if (write_options & REACHED) std::cout << reached << "\t";
    if (write_options & INSERTED) std::cout << inserted << "\t";
    if (write_options & DECREASED) std::cout << decreased << "\t";
    if (write_options & SCANNED && algorithm & ABISTAR) std::cout << rejected << "\t";
    std::cout<<""<<std::endl;
  
  }
  
  ftime(&finish);
  switch (algorithm)
  {
    case BIJKSTRA: write_elapsed(start,finish,(char*)"run bidirectional dijkstra"); break;
    case ASTAR: write_elapsed(start,finish,(char*)"run unidirectional astar"); break;
    case ABISTAR: write_elapsed(start,finish,(char*)"run bidirectional astar"); break;
    default: write_elapsed(start,finish,(char*)"run unidirectional dijkstra"); break;
  }


  if (sources != NULL) delete[] sources;
  if (targets != NULL) delete[] targets;
    
  return 0;
}

int calc_elapsed(const timeb &start, const timeb &finish)
{
  return (int)(1000*(finish.time - start.time) + (finish.millitm - start.millitm));
}

void write_elapsed (const timeb &start, const timeb &finish, char* what)
{
    int elapsed = calc_elapsed(start,finish);
    printf("It took %d.%03d seconds to %s\n",elapsed / 1000, elapsed % 1000, what);
}

int proc_all(int argc, char** argv)
{
    if (argc < 3) return 0;
    for (int i=2; i < argc; ++i)
    {
        if (strstr(argv[i],"-all") != 0) 
            return strcmp(argv[i],"-allF") == 0 ? 2 : 1;
    }
    return 0;
}

void write_help()
{
  std::cout << "\nUSAGE\n"<< std::endl;
  
  std::cout << "Parameters:"<< std::endl;
  std::cout << "GRAPHPATH ALGORITHM ROUTEHANDLING WRITEOPTIONS ROUTEPATH\n" << std::endl;
  
  std::cout << "\nGRAPHPATH:     The path where the graph will be read from" << std::endl;
  std::cout << "\nALGORITHM:     Which of the algorithms listed below will run (only one)" << std::endl;
  std::cout << "  0  Dijkstra" << std::endl;
  std::cout << "  1  Bijkstra" << std::endl;
  std::cout << "  2  AStar" << std::endl;
  std::cout << "  4  ABiStar" << std::endl;
  
  std::cout << "\nROUTEHANDLING: How to handle the route(s) (only one)" << std::endl;
  std::cout << "  0  Default           - The source and target nodes will be read from the graph" << std::endl;
  std::cout << "  1  Default with draw - Same as the default, but the result can be drawed" << std::endl;
  std::cout << "  2  From file         - The routes will be read from the file ROUTEPATH" << std::endl;
  
  std::cout << "\nWRITEOPTIONS:  What will the output include for each route (even more)" << std::endl;
  std::cout << "  0    Nothing         - Nothing will be written" << std::endl;
  std::cout << "  1    Source          - The source node will be written" << std::endl;
  std::cout << "  2    Target          - The target node will be written" << std::endl;
  std::cout << "  4    Distance        - The distance will be written" << std::endl;
  std::cout << "  8    Length          - The length will be written" << std::endl;
  std::cout << "  16   Time            - The time will be written" << std::endl;
  std::cout << "  32   Scanned         - The number of scanned nodes will be written" << std::endl;
  std::cout << "  64   Reached         - The number of reached nodes will be written" << std::endl;
  std::cout << "  128  Inserted        - The number of inserted nodes will be written" << std::endl;
  std::cout << "  256  Decreased       - The number of decreased nodes will be written" << std::endl;
  
  std::cout << "\nROUTEPATH:     The path where the routes will be read from (only in case of ROUTEHANDLING = From file)";
  std::cout << "\n" << std::endl;
  
  std::cout << "there is a shortcut with only one parameter: ALGORITHM" << std::endl;
  std::cout << "it is equal to calling \"USA-road-d.NY.eukl.txt ALGORITHM 2 511 routes.txt\"\n" << std::endl;
}

void write_configures(std::string graphPath, int algorithm, int routes, int write_options, std::string routesPath)
{
  std::cout << "\n----------------------options----------------------" << std::endl;
  std::cout << "The program will run with the following options:\n" << std::endl;
  std::cout << "GRAPHPATH:     " << graphPath << std::endl;
  std::cout << "ALGORITHM:     ";
  switch (algorithm)
  {
    case (BIJKSTRA): std::cout << "Bijkstra"; break;
    case (ASTAR):    std::cout << "AStar"; break;
    case (ABISTAR):   std::cout << "ABiStar"; break;
    default:         std::cout << "Dijkstra"; break;
  }
  std::cout << "\nROUTEHANDLING: ";
  switch (routes)
  {
    case (DEFAULT_WITH_DRAW): std::cout << "Default with draw"; break;
    case (FROM_FILE):         std::cout << "From file"; break;
    default:                  std::cout << "Default"; break;
  }
  std::cout << "\nWRITEOPTIONS: ";
  if (write_options == NONE)       std::cout << " Nothing";
  else {
    if (write_options & SOURCE)    std::cout << " Source";
    if (write_options & TARGET)    std::cout << " Target";
    if (write_options & DISTANCE)  std::cout << " Distance";
    if (write_options & LENGTH)    std::cout << " Length";
    if (write_options & TIME)      std::cout << " Time";
    if (write_options & SCANNED)   std::cout << " Scanned";
    if (write_options & REACHED)   std::cout << " Reached";
    if (write_options & INSERTED)  std::cout << " Inserted";
    if (write_options & DECREASED) std::cout << " Decreased";
  }
  if (routes & FROM_FILE) std::cout << "\nROUTEPATH:     " << routesPath << std::endl;
  else std::cout<<""<<std::endl;
  std::cout << "----------------------options----------------------\n" << std::endl;
}
