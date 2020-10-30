namespace Glasswall.IcapServer.CloudProxyApp.Configuration
{
    public interface IAppConfigurationChecker
    {
        void CheckConfiguration(IAppConfiguration configuration);
    }
}
