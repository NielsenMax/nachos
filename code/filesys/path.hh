#ifndef PATH__HH
#define PATH__HH

#include <string>
#include <list>

class Path {
public:
    
    void Merge(const char* subpath);

    std::string GetPath();

    std::string Split();

    std::list<std::string> path;
};

#endif