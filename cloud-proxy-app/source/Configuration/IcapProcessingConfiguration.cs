using System;

namespace Glasswall.IcapServer.CloudProxyApp.Configuration
{
    public class IcapProcessingConfiguration : IProcessingConfiguration
    {
        public TimeSpan ProcessingTimeoutDuration { get; set; }
    }
}
