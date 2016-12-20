#include <fstream>
#include "config.h"

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
