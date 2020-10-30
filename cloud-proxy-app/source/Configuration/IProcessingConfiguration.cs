using System;

namespace Glasswall.IcapServer.CloudProxyApp.Configuration
{
    public interface IProcessingConfiguration
    {
        TimeSpan ProcessingTimeoutDuration { get; set; }
    }
}
