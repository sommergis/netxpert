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

#include <fstream>
#include "config.hpp"

using namespace std;
using namespace cereal;
using namespace netxpert::cnfg;

Config ConfigReader::GetConfigFromJSON(string jsonString)
{
    stringstream ss (jsonString);
    try
    {
        JSONInputArchive archive ( ss );
        Config outCnfg;
        archive( outCnfg );
        return outCnfg;
    }
    catch (cereal::RapidJSONException& ex)
    {
        cout <<"Error in Config String!"<<endl;
        throw ex;
    }
}

void ConfigReader::GetConfigFromJSONFile(string fileName, Config& cnfg)
{
    ifstream is(fileName);
    try
    {
        JSONInputArchive archive( is );
        archive( cnfg );
    }
    catch (cereal::RapidJSONException& ex)
    {
        cout <<"Error in Config File: "<<fileName <<"!"<<endl;
        throw ex;
    }
}
