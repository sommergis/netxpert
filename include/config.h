/*
 * This file is a part of netxpert.
 *
 * Copyright (C) 2013-2017
 * Johannes Sommer, Christopher Koller
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

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <string>
#include <stdexcept>
#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/map.hpp>
#include <omp.h>

namespace netxpert {

    ///\brief Current version of netXpert
    static const std::string Version() {
        return "0.9.8a";
    };

    namespace cnfg {

    /**
    * \brief 80% of available CPU Power (=number of threads) is used
    * (can be overridden by setting environment variable
    *  OMP_NUM_THREADS to a higher value)
    */
    #ifdef DEBUG
        static int LOCAL_NUM_THREADS = 1;
    #endif // DEBUG
    #ifndef DEBUG
        static int LOCAL_NUM_THREADS = std::floor(omp_get_max_threads() * 0.8);
    #endif

    /**
    * \brief Geometry Handling.
    **/
    enum GEOMETRY_HANDLING : int16_t {
        NoGeometry = 0,    //!< Output will be attribute tables only - no geometries.
        StraightLines = 1, //!< Output will be straight lines ("as the crow flies") from start to end points.
        RealGeometry = 2   //!< Output will be line geometries.
    };

    /**
    * \brief TestCases, that can be started per entry "TestCase" in Config file.
    **/
    enum TESTCASE : int16_t {
        TestNetworkBuilder = 0,
        ShortestPathTreeCOM = 1,
        ODMatrixCOM = 2,
        TransportationCOM = 3,
        MSTCOM = 4,
        IsolinesCOM = 5,
        DumpInternalArcsToDB = 6,
        NetworkConvert = 7,
        TestFileGDBWriter = 8,
        TestSpatiaLiteWriter = 9,
        TestAddNodes = 10,
        TestCreateRouteGeometries = 11,
        MCFCOM = 12,
        TransportationCOMExt = 13,
        ODMatrixCOM2 = 14
    };

    /**
    * \brief Type of the output DB in which the solver result is been written.
    **/
    enum RESULT_DB_TYPE : int16_t {
        SpatiaLiteDB = 0, //!< SpatiaLite Format > 4.3.0.
        ESRI_FileGDB = 1, //!< ESRI File Geodatabase with FileGDB API 1.4.
        JSON = 2,         //!< JSON
        GooglePolyLine = 3//!< Google Compressed Binary Polyline encoding
    };

    /**
    * \brief Type of the Log Level.
    **/
    enum LOG_LEVEL : int16_t {
       LogAll = -1,   //!< Any message will be logged, also debugging infos.
       LogDebug = 0,  //!< Any message from fatal errors to debug messages will be logged.
       LogInfo = 1,   //!< Any message from fatal errors to info messages will be logged.
       LogWarning = 2,//!< Any message from fatal errors to warning messages will be logged.
       LogError = 3,  //!< Any message from fatal errors to info messages will be logged.
       LogFatal = 4,  //!< Only fatal errors will be logged.
       LogQuiet = 5   //!< No logging at all.
    };
    /**
    * \brief Type of the Shortest Path Tree algorithms.
    **/
    enum SPTAlgorithm : int16_t {
        Dijkstra_2Heap_LEMON = 4, //!< Dijkstra of LEMON with binary heap
        Bijkstra_2Heap_LEMON = 5, //!< Bidirectional Dijkstra of LEMON with binary heap
        Dijkstra_dheap_BOOST = 6, //!< Dijkstra of Boost Graph Library with d-ary heap
        ODM_LEM_2Heap = 7         //!< \warning: was just an experiment!
    };
    /**
    * \brief Type of the Minimum Cost Flow algorithms.
    **/
    enum MCFAlgorithm : int16_t {
        NetworkSimplex_LEMON = 1  //!< NetworkSimplex algorithm of LEMON
    };
    /**
    * \brief Type of the Minimum Spanning Tree algorithms.
    **/
    enum MSTAlgorithm : int16_t {
        Kruskal_LEMON = 2 //!< Kruskal's Minimum Spanning Tree algorithm of LEMON.
    };

    /**
    * \brief Storage for the configuration of NetXpert
    **/
    struct Config
    {
        std::string NetXDBPath; //!< Path for the input database
        std::string ResultDBPath; //!< Path for the result database
        netxpert::cnfg::RESULT_DB_TYPE ResultDBType;//!< Type of the result database type
        std::string ResultTableName; //!< Name of the result table
        bool SPTAllDests;//!< If true, then all shortest paths from the given start point to all reachable destinations in the network will be computed in the shortest path solver
        int SPTHeapCard; //!< Ariety for the heap structure in shortest path algorithms if supported. \warning Unsed at the moment!
        netxpert::cnfg::SPTAlgorithm SptAlgorithm; //!< Shortest path algorithm to use
        netxpert::cnfg::MCFAlgorithm McfAlgorithm; //!< Minimum cost flow algorithm to use
        netxpert::cnfg::MSTAlgorithm MstAlgorithm; //!< Minimum spanning tree algorithm to use
        bool IsDirected; //!< Network is directed or bidirectional
        std::string ArcsTableName; //!< Name of the input arcs table
        std::string ArcsGeomColumnName; //!< Geometry column name of the input arcs table
        std::string ArcIDColumnName; //!< ID column name of the input arcs table
        std::string FromNodeColumnName; //!< From node column name of the input arcs table
        std::string ToNodeColumnName; //!< To node column name of the input arcs table
        std::string CostColumnName; //!<  Cost column name of the input arcs table
        std::string CapColumnName; //!<  Capacity column name of the input arcs table
        std::string OnewayColumnName;//!< Oneway column name of the input arcs table
        std::string NodesTableName; //!< Name of the input nodes table
        std::string NodesGeomColumnName; //!< Geometry column name of the input nodes table
        std::string NodeIDColumnName;//!< ID column name of the input nodes table
        std::string NodeSupplyColumnName;//!< Supply column name of the input nodes table
        std::string BarrierPolyTableName;//!< Name of the barrier polygon table
        std::string BarrierPolyGeomColumnName;//!< Geometry column name of the barrier polygon table
        std::string BarrierLineTableName;//!< Name of the barrier line table
        std::string BarrierLineGeomColumnName;//!< Geometry column name of the barrier line table
        std::string BarrierPointTableName;//!< Name of the barrier point table
        std::string BarrierPointGeomColumnName;//!< Geometry column name of the barrier point table
        int Treshold; //!< Treshold for distance search: closest arc of network to given point; (Snapping tolerance)
        bool UseSpatialIndex;//!< \deprecated Use spatial index or not in input database. Default: true
        bool LoadDBIntoMemory;//!< \deprecated Load input database into memory.
        int NumberOfTests;//!< Number of tests to run
        std::string SpatiaLiteHome;//!< Path to the directory of the SpatiaLite library
        std::string SpatiaLiteCoreName;//!< Name of the SpatiaLite library (without file extension)
        netxpert::cnfg::GEOMETRY_HANDLING GeometryHandling;//!< Geometry handling (no geometry, straight lines, real geometry).
        netxpert::cnfg::TESTCASE TestCase;//!< Test case to run
        netxpert::cnfg::LOG_LEVEL LogLevel;//!< Application log level.
        bool CleanNetwork;//!< Clean input network on load.
        std::string LogFileFullPath; //!< Path to log file.

        ///\brief Serialize struct members to JSON
        template <class Archive>
        void serialize( Archive & ar ){
            ar(
                CEREAL_NVP(NetXDBPath),
                CEREAL_NVP(ResultDBPath),
                CEREAL_NVP(ResultDBType),
                CEREAL_NVP(ResultTableName),
                CEREAL_NVP(SPTAllDests),
                CEREAL_NVP(SPTHeapCard),
                CEREAL_NVP(SptAlgorithm),
                CEREAL_NVP(McfAlgorithm),
                CEREAL_NVP(MstAlgorithm),
                CEREAL_NVP(IsDirected),
                CEREAL_NVP(ArcsTableName),
                CEREAL_NVP(ArcsGeomColumnName),
                CEREAL_NVP(ArcIDColumnName),
                CEREAL_NVP(FromNodeColumnName),
                CEREAL_NVP(ToNodeColumnName),
                CEREAL_NVP(CostColumnName),
                CEREAL_NVP(CapColumnName),
                CEREAL_NVP(NodesTableName),
                CEREAL_NVP(NodesGeomColumnName),
                CEREAL_NVP(NodeIDColumnName),
                CEREAL_NVP(NodeSupplyColumnName),
                CEREAL_NVP(BarrierPolyTableName),
                CEREAL_NVP(BarrierPolyGeomColumnName),
                CEREAL_NVP(BarrierLineTableName),
                CEREAL_NVP(BarrierLineGeomColumnName),
                CEREAL_NVP(BarrierPointTableName),
                CEREAL_NVP(BarrierPointGeomColumnName),
                CEREAL_NVP(Treshold),
                CEREAL_NVP(UseSpatialIndex),
                CEREAL_NVP(CleanNetwork),
                CEREAL_NVP(LoadDBIntoMemory),
                CEREAL_NVP(NumberOfTests),
                CEREAL_NVP(SpatiaLiteHome),
                CEREAL_NVP(SpatiaLiteCoreName),
                CEREAL_NVP(OnewayColumnName),
                CEREAL_NVP(TestCase),
                CEREAL_NVP(GeometryHandling),
                CEREAL_NVP(LogLevel),
                CEREAL_NVP(LogFileFullPath) );
        }
    };


    ///\brief Reads the configuration file for netXpert
    class ConfigReader
    {
        public:
            ///\brief Constructor for ConfigReader
            ConfigReader() {}
            ///\brief Deconstructor for ConfigReader
            ~ConfigReader() {}
            ///\brief Gets the configuration from the given JSON string
            Config GetConfigFromJSON(std::string jsonString);
            ///\brief Gets the configuration from the given JSON file
            void GetConfigFromJSONFile(std::string fileName, netxpert::cnfg::Config& cnfg);
    };
} //namespace cnfg
} //namespace netxpert

#endif // CONFIG_H
