#include "Environment.h"


class NetworkEnvironment : IEnvironment {

    public:
        NetworkEnvironment(const int threadCount) : m_threadCount(threadCount) { }
        bool submitPortForScan(const uint16_t port, NetworkProtocols requestedProtocols) {

            return true;
        }

    private:

        uint16_t m_threadCount; 
};


std::shared_ptr<IEnvironment> EnvironmentFactory::defaultGenerator(const int threadCount) {

    return std::make_shared<IEnvironment>(NetworkEnvironment{threadCount});
}
