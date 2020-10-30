using System.Collections.Generic;

namespace Glasswall.IcapServer.CloudProxyApp
{
    internal static class CommandLineSwitchMapping
    {
        const string InputConfigurationKey = "InputFilepath";
        const string OutputConfigurationKey = "OutputFilepath";
        const string ConfigurationConfigurationKey = "Configuration";

        public static IDictionary<string, string> Mapping { get; } = new Dictionary<string, string>
        {
                { "-i", InputConfigurationKey },
                { "-o", OutputConfigurationKey },
                { "-c", ConfigurationConfigurationKey },
        };
    }
}
