#include <stdio.h>
#include <jsoncpp/json/json.h>

int main()
{
    Json::Value root;
    root["name"] = "John";
    root["age"] = 30;
    root["married"] = true;
    Json::Reader gh;
    return 0;
    
}