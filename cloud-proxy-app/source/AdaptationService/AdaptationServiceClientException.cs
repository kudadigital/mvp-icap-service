using System;

namespace Glasswall.IcapServer.CloudProxyApp.AdaptationService
{
    public class AdaptationServiceClientException : ApplicationException
    {
        public AdaptationServiceClientException()
        {

        }

        public AdaptationServiceClientException(string message) : base(message)
        {
        }
    }
}
