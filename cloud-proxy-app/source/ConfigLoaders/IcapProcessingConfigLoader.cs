using Glasswall.IcapServer.CloudProxyApp.Configuration;
using System;

namespace Glasswall.IcapServer.CloudProxyApp.ConfigLoaders
{
    public static class IcapProcessingConfigLoader
    {
        public static IProcessingConfiguration SetDefaults(IProcessingConfiguration configuration)
        {
            configuration.ProcessingTimeoutDuration = TimeSpan.FromSeconds(60);
            return configuration;
        }
    }
}
