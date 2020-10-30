namespace Glasswall.IcapServer.CloudProxyApp.Configuration
{
    public class AdaptationStoreConfiguration : IStoreConfiguration
    {
        public string OriginalStorePath { get; set; }
        public string RebuiltStorePath { get; set; }
    }
}
