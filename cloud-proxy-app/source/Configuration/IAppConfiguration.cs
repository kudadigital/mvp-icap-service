namespace Glasswall.IcapServer.CloudProxyApp.Configuration
{
    public interface IAppConfiguration
    {
        string InputFilepath { get; }
        string OutputFilepath { get; }
    }
}
