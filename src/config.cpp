#include <fstream>
#include "config.h"

using namespace netxpert;
using namespace cereal;
using namespace std;

 Config ConfigReader::GetConfigFromJSON(string jsonString)
{
    stringstream ss (jsonString);
    JSONInputArchive archive ( ss );
    Config outCnfg;
    archive( outCnfg );
    return outCnfg;
}

void ConfigReader::GetConfigFromJSONFile(string fileName, Config& cnfg)
{
    ifstream is(fileName);
    JSONInputArchive archive( is );
    archive( cnfg );
}
