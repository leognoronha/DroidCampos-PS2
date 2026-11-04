#pragma once
#include <string>
namespace Tyra {struct FileUtils {
 static std::string getPathFromFilename(const std::string& p){auto n=p.find_last_of("/\\");return n==std::string::npos?"":p.substr(0,n+1);}
 static std::string getFilenameWithoutExtension(const std::string& p){return p.substr(0,p.find_last_of('.'));}
 static std::string getExtensionOfFilename(const std::string& p){return p.substr(p.find_last_of('.')+1);}
};}
