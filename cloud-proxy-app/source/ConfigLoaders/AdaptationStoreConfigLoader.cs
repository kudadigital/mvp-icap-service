using Glasswall.IcapServer.CloudProxyApp.Configuration;

namespace Glasswall.IcapServer.CloudProxyApp.ConfigLoaders
{
    public static class AdaptationStoreConfigLoader
    {
        public static IStoreConfiguration SetDefaults(IStoreConfiguration configuration)
        {
            configuration.OriginalStorePath = "/var/source";
            configuration.RebuiltStorePath = "/var/target";

            return configuration;
        }
    }
}
