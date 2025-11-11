#include "MarginCallPluginInterface.hpp"

extern "C" void AboutReport(Value& request,
                            Value& response,
                            Document::AllocatorType& allocator,
                            CServerInterface* server) {
    response.AddMember("version", 123, allocator);
}

extern "C" void DestroyReport() {}

extern "C" void CreateReport(Value& request,
                             Value& response,
                             Document::AllocatorType& allocator,
                             CServerInterface* server) {
}