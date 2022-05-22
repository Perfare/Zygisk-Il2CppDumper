#include <iostream>
using namespace std;
#include <list>  
#include <string>
struct Field {
    string name;
    string offset;
};
struct Property {
    string name;
};
struct Method {
    string name;
    string offset;
    string sigNature;
};
struct Dump {
    string dllName;
    string nameSpace;
    string clsName;
    list<Field> fields;
    list<Property> properties;
    list<Method> methods;

};
string toHexString(int i){
    stringstream ss;
    string s_temp;
    ss << "0x" << std::hex << i;
    ss >> s_temp;
    return s_temp;
}
void Replace(string& s1, const string& s2, const string& s3)
{
    string::size_type pos = 0;
    string::size_type a = s2.size();
    string::size_type b = s3.size();
    while ((pos = s1.find(s2, pos)) != string::npos)
    {
        s1.erase(pos, a);
        s1.insert(pos, s3);
        pos += b;
    }
}

list<string> SerializeDumps(list<Dump> dumps) {
    list<string> allStr;
    string result;
    allStr.push_back("{\"Dumps\":[");
    for (Dump dump : dumps) {
        result = "{\"dllName\":\"" + dump.dllName + "\","
                  + "\"nameSpace\":\"" + dump.nameSpace + "\","
                  + "\"clsName\":\"" + dump.clsName + "\",\"fields\": [";
        for (Field field : dump.fields)
        {
            result = result + "{"
                     + "\"name\":\"" + field.name + "\","
                     + "\"offset\":\"" + field.offset + "\"},";
        }
        //遍历完fields，开始遍历properties
        result += "],\"properties\": [";
        for (Property property : dump.properties)
        {
            result = result + "{"
                     + "\"name\":\"" + property.name + "\"},";
        }
        //遍历完properties开始遍历methods
        result += "],\"methods\": [";
        for (Method method : dump.methods)
        {
            result = result + "{"
                     + "\"name\":\"" + method.name + "\","
                     + "\"offset\":\"" + method.offset + "\","
                     + "\"sigNature\":\"" + method.sigNature + "\"},";
        }
        result += "]},";
        Replace(result, ", )", ")");
        Replace(result, ",]", "]");
        allStr.push_back(result);
        result = "";
    }
    allStr.push_back("]}");
    return allStr;
}

