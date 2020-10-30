namespace Glasswall.IcapServer.CloudProxyApp.Configuration
{
    public interface IStoreConfiguration
    {
        public string OriginalStorePath { get; set; }
        public string RebuiltStorePath { get; set; }
    }
}
