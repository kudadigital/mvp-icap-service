using System.Collections.Generic;
using System.Linq;

namespace Glasswall.IcapServer.CloudProxyApp.Configuration
{
    public class CloudProxyApplicationConfigurationChecker : IAppConfigurationChecker
    {
        public void CheckConfiguration(IAppConfiguration configuration)
        {
            var configurationErrors = new List<string>();

            if (string.IsNullOrEmpty(configuration.InputFilepath))
                configurationErrors.Add($"'InputFilepath' configuration is missing");

            if (string.IsNullOrEmpty(configuration.OutputFilepath))
                configurationErrors.Add($"'OutputFilepath' configuration is missing");

            if (configurationErrors.Any())
            {
                throw new InvalidApplicationConfigurationException(string.Join(',', configurationErrors));
            }
        }
    }
}
